#include "../DisambiguateSymbols.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/Refactoring.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/raw_ostream.h>

#include <memory>
#include <string>

namespace {

llvm::cl::OptionCategory DisambiguateSymbolsCategory("Disambiguate Symbols");

llvm::cl::opt<std::string> Root(
    "root",
    llvm::cl::desc("Path to USD's root directory."),
    llvm::cl::cat(DisambiguateSymbolsCategory)
);

llvm::cl::opt<bool> Overwrite(
    "overwrite",
    llvm::cl::desc("Overwrite the files."),
    llvm::cl::cat(DisambiguateSymbolsCategory)
);

llvm::cl::opt<bool> Dump(
    "dump",
    llvm::cl::desc("Dump the result of the changes to stdout."),
    llvm::cl::cat(DisambiguateSymbolsCategory)
);

} // anonymous namespace

int
main(
    int argc,
    const char **argv
)
{
    llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);

    auto ExpectedParser = clang::tooling::CommonOptionsParser::create(
        argc, argv, DisambiguateSymbolsCategory
    );
    if (!ExpectedParser)
    {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }

    clang::tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();

    clang::tooling::RefactoringTool Tool(
        OptionsParser.getCompilations(), OptionsParser.getSourcePathList()
    );

    pxr::disambiguate_symbols::DisambiguateSymbolsTool PxrTool(
        &Tool.getReplacements(), Root
    );

    clang::ast_matchers::MatchFinder Finder;
    PxrTool.registerMatchers(&Finder);
    std::unique_ptr<clang::tooling::FrontendActionFactory> Factory =
        clang::tooling::newFrontendActionFactory(&Finder);

    if (int Result = Tool.run(Factory.get()))
    {
        return Result;
    }

    clang::LangOptions DefaultLangOptions;
    llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> DiagOpts
        = new clang::DiagnosticOptions();
    clang::TextDiagnosticPrinter DiagnosticPrinter(llvm::errs(), &*DiagOpts);
    clang::DiagnosticsEngine Diagnostics(
        llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs>(
            new clang::DiagnosticIDs()
        ),
        &*DiagOpts,
        &DiagnosticPrinter,
        false
    );

    clang::FileManager &FileMgr(Tool.getFiles());
    clang::SourceManager SourceMgr(Diagnostics, FileMgr);
    clang::Rewriter Rewrite(SourceMgr, DefaultLangOptions);

    std::map<std::string, clang::tooling::Replacements> GroupedReplacements
        = clang::tooling::groupReplacementsByFile(
            Rewrite.getSourceMgr().getFileManager(), Tool.getReplacements()
        );
    for (const auto &FileAndReplaces: GroupedReplacements)
    {
        const clang::tooling::Replacements &Replaces = FileAndReplaces.second;
        if (!clang::tooling::applyAllReplacements(Replaces, Rewrite))
        {
            const std::string &FilePath = FileAndReplaces.first;
            llvm::errs()
                << "Failed applying replacements for file "
                << FilePath.c_str()
                << ".\n";
            return 1;
        }
    }

    if (Dump)
    {
        for (const auto &it : Tool.getReplacements())
        {
            llvm::StringRef File = it.first;
            const llvm::ErrorOr<const clang::FileEntry *> Entry
                = FileMgr.getFile(File);

            clang::FileID ID
                = SourceMgr.getOrCreateFileID(*Entry, clang::SrcMgr::C_User);
            llvm::outs() << "============== " << File << " ==============\n";
            Rewrite.getEditBuffer(ID).write(llvm::outs());
            llvm::outs() << "\n============================================\n";
        }
    }

    if (Overwrite && Rewrite.overwriteChangedFiles())
    {
        llvm::errs()
            << "Failed writing the changes to the files.\n";
        return 1;
    }

    return 0;
}
