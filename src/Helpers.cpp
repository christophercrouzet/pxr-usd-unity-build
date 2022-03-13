#include "Helpers.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/StringRef.h>

#include <cassert>

clang::DiagnosticBuilder
pxr::
diag(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    llvm::StringRef Description
)
{
    assert(Loc.isValid());
    clang::DiagnosticsEngine &DiagEngine = Result.Context->getDiagnostics();
    unsigned ID
        = DiagEngine.getDiagnosticIDs()->getCustomDiagID(
            clang::DiagnosticIDs::Warning, Description.str()
        );
    return DiagEngine.Report(Loc, ID);
}

clang::StringRef
pxr::
getSourceChars(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Begin,
    clang::SourceLocation End
)
{
    return clang::Lexer::getSourceText(
        clang::CharSourceRange::getCharRange(Begin, End),
        *Result.SourceManager,
        Result.Context->getLangOpts()
    );
}

clang::StringRef
pxr::
getSourceTokens(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Begin,
    clang::SourceLocation End
)
{
    return clang::Lexer::getSourceText(
        clang::CharSourceRange::getTokenRange(Begin, End),
        *Result.SourceManager,
        Result.Context->getLangOpts()
    );
}
