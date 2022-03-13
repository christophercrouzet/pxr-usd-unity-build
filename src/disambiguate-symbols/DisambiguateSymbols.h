#ifndef DISAMBIGUATE_SYMBOLS_H
#define DISAMBIGUATE_SYMBOLS_H

#include <clang/AST/DeclCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Tooling/Core/Replacement.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>

#include <map>
#include <string>

namespace pxr {
namespace disambiguate_symbols {

class DisambiguateSymbolsTool
    : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    DisambiguateSymbolsTool(
        std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
        llvm::StringRef RootPath
    );

    void
    registerMatchers(
        clang::ast_matchers::MatchFinder *Finder
    );

    void
    run(
        const clang::ast_matchers::MatchFinder::MatchResult &Result
    ) override;

private:
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements;
    llvm::StringRef RootPath;
};

} // namespace disambiguate_symbols
} // namespace pxr

#endif // DISAMBIGUATE_SYMBOLS_H
