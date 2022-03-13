#include "Locations.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/NestedNameSpecifier.h>
#include <clang/AST/TypeLoc.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/TokenKinds.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Token.h>

clang::SourceLocation
pxr::
getBeginLocationForAnonNamespace(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::NamespaceDecl *Decl
)
{
    return Decl->getBeginLoc();
}

clang::SourceLocation
pxr::
getEndLocationForAnonNamespace(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::NamespaceDecl *Decl
)
{
    return clang::Lexer::getLocForEndOfToken(
        Decl->getBeginLoc(),
        0,
        *Result.SourceManager,
        Result.Context->getLangOpts()
    );
}

clang::SourceLocation
pxr::
getBeginLocationForUsing(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::Decl *Decl
)
{
    return Decl->getBeginLoc();
}

clang::SourceLocation
pxr::
getEndLocationForUsing(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::Decl *Decl
)
{
    return clang::Lexer::findLocationAfterToken(
        Decl->getEndLoc(),
        clang::tok::semi,
        *Result.SourceManager,
        Result.Context->getLangOpts(),
        false
    );
}

clang::SourceLocation
pxr::
getBeginLocationForExpr(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::Expr *Expr,
    clang::NestedNameSpecifierLoc Nested
)
{
    if (Nested)
    {
        return Nested.getBeginLoc();
    }

    return Expr->getBeginLoc();
}

clang::SourceLocation
pxr::
getEndLocationForExpr(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::Expr *Expr,
    clang::NestedNameSpecifierLoc Nested
)
{
    if (Nested)
    {
        clang::TypeLoc NestedType = Nested.getTypeLoc();
        if (NestedType)
        {
            clang::NestedNameSpecifierLoc Prefix = Nested.getPrefix();
            return Prefix.getEndLoc();
        }

        return Nested.getEndLoc();
    }

    return Expr->getBeginLoc();
}

clang::SourceLocation
pxr::
getBeginLocationForType(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::TypeLoc Type
)
{
    if (Type.getTypeLocClass() == clang::TypeLoc::Elaborated)
    {
        clang::NestedNameSpecifierLoc NestedNameSpecifier =
            Type.castAs<clang::ElaboratedTypeLoc>().getQualifierLoc();
        if (NestedNameSpecifier.getNestedNameSpecifier())
        {
            return NestedNameSpecifier.getBeginLoc();
        }

        Type = Type.getNextTypeLoc();
    }

    return Type.getBeginLoc();
}

clang::SourceLocation
pxr::
getEndLocationForType(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::TypeLoc Type
)
{
    while (
        Type.getTypeLocClass() == clang::TypeLoc::Qualified)
    {
        Type = Type.getNextTypeLoc();
    }

    if (Type.getTypeLocClass() == clang::TypeLoc::Elaborated)
    {
        clang::ElaboratedTypeLoc Elaborated
            = Type.castAs<clang::ElaboratedTypeLoc>();
        clang::NestedNameSpecifierLoc Nested = Elaborated.getQualifierLoc();

        if (Nested)
        {
            clang::TypeLoc NestedType = Nested.getTypeLoc();
            if (NestedType)
            {
                return NestedType.getBeginLoc().getLocWithOffset(-2);
            }

            return Nested.getEndLoc();
        }
    }

    return Type.getBeginLoc();
}

clang::SourceLocation
pxr::
getBeginLocationForNested(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::NestedNameSpecifierLoc Nested
)
{
    return Nested.getBeginLoc();
}

clang::SourceLocation
pxr::
getEndLocationForNested(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    const clang::NestedNameSpecifierLoc Nested
)
{
    clang::TypeLoc NestedType = Nested.getTypeLoc();
    if (NestedType)
    {
        clang::NestedNameSpecifierLoc Prefix = Nested.getPrefix();
        return Prefix.getEndLoc();
    }

    return Nested.getEndLoc();
}
