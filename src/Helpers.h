#ifndef HELPERS_H
#define HELPERS_H

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/StringRef.h>

namespace pxr {

clang::DiagnosticBuilder
diag(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    llvm::StringRef Description
);

clang::StringRef
getSourceChars(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Begin,
    clang::SourceLocation End
);

clang::StringRef
getSourceTokens(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Begin,
    clang::SourceLocation End
);

} // namespace pxr

#endif // HELPERS_H
