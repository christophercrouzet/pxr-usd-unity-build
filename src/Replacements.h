#ifndef REPLACEMENTS_H
#define REPLACEMENTS_H

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Tooling/Core/Replacement.h>

#include <map>
#include <string>

namespace pxr {

void
registerReplacement(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    clang::tooling::Replacement Replacement,
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    const char *Label,
    const clang::FixItHint &Fix
);

void
createInsertion(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    llvm::StringRef Value,
    const char *Label
);

void
createReplacement(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    clang::SourceRange Range,
    llvm::StringRef Value,
    const char *Label
);

void
createRemoval(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    clang::SourceRange Range,
    const char *Label
);

} // namespace pxr

#endif // REPLACEMENTS_H
