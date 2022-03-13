#ifndef INLINE_NAMESPACES_H
#define INLINE_NAMESPACES_H

#include <clang/AST/DeclCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Tooling/Core/Replacement.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>

#include <map>
#include <string>

namespace pxr {
namespace inline_namespaces {

class InlineNamespacesTool
    : public clang::ast_matchers::MatchFinder::MatchCallback
{
public:
    InlineNamespacesTool(
        std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
        llvm::StringRef FilePattern
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
    llvm::StringRef FilePattern;
    llvm::SmallVector<const clang::NamespaceAliasDecl *> NamespaceAliasDeps;
    llvm::SmallVector<const clang::UsingDecl *> UsingDeps;
    llvm::SmallVector<const clang::UsingDirectiveDecl *> UsingNamespaceDeps;
};

} // namespace inline_namespaces
} // namespace pxr

#endif // INLINE_NAMESPACES_H
