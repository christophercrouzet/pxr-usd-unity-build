#ifndef LOCATIONS_H
#define LOCATIONS_H

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/NestedNameSpecifier.h>
#include <clang/AST/TypeLoc.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceLocation.h>

namespace pxr {

clang::SourceLocation
getBeginLocationForAnonNamespace(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::NamespaceDecl *Decl
);

clang::SourceLocation
getEndLocationForAnonNamespace(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::NamespaceDecl *Decl
);

clang::SourceLocation
getBeginLocationForUsing(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::Decl *Decl
);

clang::SourceLocation
getEndLocationForUsing(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::Decl *Decl
);

clang::SourceLocation
getBeginLocationForExpr(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::Expr *Expr,
    clang::NestedNameSpecifierLoc Nested
);

clang::SourceLocation
getEndLocationForExpr(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::Expr *Expr,
    clang::NestedNameSpecifierLoc Nested
);

clang::SourceLocation
getBeginLocationForType(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::TypeLoc Type
);

clang::SourceLocation
getEndLocationForType(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::TypeLoc Type
);

clang::SourceLocation
getBeginLocationForNested(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::NestedNameSpecifierLoc Nested
);

clang::SourceLocation
getEndLocationForNested(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::NestedNameSpecifierLoc Nested
);

} // namespace pxr

#endif // LOCATIONS_H
