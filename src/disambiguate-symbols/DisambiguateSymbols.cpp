// Give a name to anonymous namespaces in order to avoid name clashes
// with others source files.

#define DEBUG 0

#include "DisambiguateSymbols.h"
#include "../Helpers.h"
#include "../Locations.h"
#include "../Replacements.h"

#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/NestedNameSpecifier.h>
#include <clang/AST/TypeLoc.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/CharInfo.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TokenKinds.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/ErrorHandling.h>

#include <algorithm>
#include <cassert>
#include <map>
#include <string>

using namespace clang::ast_matchers;

namespace pxr {
namespace disambiguate_symbols {

namespace {

/* Modules                                                         O-(''Q)
   -------------------------------------------------------------------------- */

std::string
getModuleName(
    clang::StringRef FilePath,
    clang::StringRef RootPath
)
{
    assert(FilePath.startswith(RootPath));
    FilePath = FilePath.substr(
        RootPath.size() + 1,
        FilePath.find('.') - RootPath.size() - 1
    );

    llvm::SmallVector<llvm::StringRef> Parts;
    FilePath.split(Parts, '/', -1, false);

    std::string Out(Parts[0].str());
    for (size_t I = 1; I < Parts.size(); ++I)
    {
        Out += clang::toUppercase(Parts[I][0]);
        Out += Parts[I].substr(1);
    }

    return Out;
}


/* Fixers                                                          O-(''Q)
   -------------------------------------------------------------------------- */

void
fixNameAnonNamespace(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    const MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    clang::SourceLocation Begin,
    clang::SourceLocation End,
    clang::StringRef ModuleName,
    const clang::Decl *MatchedAnonNamespace
)
{
    clang::SourceManager *SourceMgr = Result.SourceManager;

    if (!Loc.isValid())
    {
        return;
    }

    // Ensure that the locations refer to where they were spelled in the source.

    Begin = SourceMgr->getSpellingLoc(Begin);
    End = SourceMgr->getSpellingLoc(End);

    // Check whether the location is defined within the current file.

    if (SourceMgr->getFileID(Loc) != SourceMgr->getMainFileID())
    {
        return;
    }

    pxr::createInsertion(
        FileToReplacements,
        Result,
        End,
        " " + ModuleName.str(),
        "anon namespace"
    );
}

void
fixInlineNamespace(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    const MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    clang::SourceLocation Begin,
    clang::SourceLocation End,
    clang::StringRef ModuleName
)
{
    clang::SourceManager *SourceMgr = Result.SourceManager;

    if (!Loc.isValid())
    {
        return;
    }

    // Ensure that the locations refer to where they were spelled in the source.

    Loc = SourceMgr->getSpellingLoc(Loc);
    Begin = SourceMgr->getSpellingLoc(Begin);
    End = SourceMgr->getSpellingLoc(End);

    // Check whether the location is defined within the current file.

    if (SourceMgr->getFileID(Loc) != SourceMgr->getMainFileID())
    {
        return;
    }

    // Use the module name as namespace and add the `::` delimiter if it is not
    // already present.
    const char *Buf = SourceMgr->getCharacterData(Begin);
    if (Buf[0] == ':' && Buf[1] == ':')
    {
        createInsertion(
            FileToReplacements,
            Result,
            Begin,
            ModuleName,
            "inline namespace"
        );
        return;
    }

    createInsertion(
        FileToReplacements,
        Result,
        Begin,
        ModuleName.str() + "::",
        "inline namespace"
    );
}

} // anonymous namespace

/* Class Implementation                                            O-(''Q)
   -------------------------------------------------------------------------- */

DisambiguateSymbolsTool::
DisambiguateSymbolsTool(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    llvm::StringRef RootPath
) :
    FileToReplacements(FileToReplacements),
    RootPath(RootPath)
{
}

void
DisambiguateSymbolsTool::
registerMatchers(
    MatchFinder *Finder
)
{
    // Anonymous namespaces.

    auto AnonNamespace
        = namespaceDecl(
            isExpansionInMainFile(),
            isAnonymous()
        );

    // Declarations belonging in anonymous namespaces.

    auto Decl
        = namedDecl(
            anyOf(
                hasParent(
                    AnonNamespace
                ),
                hasParent(
                    enumDecl(
                        hasParent(
                            AnonNamespace
                        )
                    )
                ),
                hasParent(
                    classTemplateDecl(
                        hasParent(
                            AnonNamespace
                        )
                    )
                ),
                hasParent(
                    functionTemplateDecl(
                        hasParent(
                            AnonNamespace
                        )
                    )
                )
            )
        );

    // There are two main categories of AST nodes that we need to match when it
    // comes to finding symbols that reference a declaration belonging into a
    // namespace that we want to inline: expressions and types.

    // Expressions are found using `declRefExpr()`, which references declared
    // variables, functions, enums, and whatnot, or overload expressions, which
    // is for expressions that could not have their declaration resolved.

    // The resulting objects allow accessing the nested name qualifier, which
    // is what we need to assess whether an expression has its referring
    // namespace already inlined or not.

    auto Expr
        = expr(
            isExpansionInMainFile(),
            anyOf(
                declRefExpr(
                    hasDeclaration(Decl)
                ),
                unresolvedLookupExpr(
                    hasAnyDeclaration(Decl)
                )
            ),
            unless(
                hasAncestor(AnonNamespace)
            )
        );

    // A single type declaration can be referenced in multiple locations,
    // so we find such references using `typeLoc()` rather than `type()`.
    // However, these type references are sometimes matched without their nested
    // name qualifier so we only match bare types here that are not prefixed
    // with any namespace, and we then handle the ones having qualifiers in
    // a dedicated matcher.

    auto Type
        = typeLoc(
            isExpansionInMainFile(),
            loc(
                qualType(
                    hasDeclaration(Decl)
                )
            ),
            unless(
                hasAncestor(AnonNamespace)
            )
        );

    // All the types with nested name qualifiers that were not matched in
    // the type matcher are taken case of here.

    auto Nested
        = nestedNameSpecifierLoc(
            hasAncestor(
                decl(
                    isExpansionInMainFile()
                )
            ),
            specifiesTypeLoc(Type),
            unless(
                hasAncestor(AnonNamespace)
            )
        );

    // Declarations having a type that is within an anonymous namespace.

    auto DeclType
        = decl(
            isExpansionInMainFile(),
            has(
                typeLoc(
                    has(
                        loc(
                            qualType(
                                hasDeclaration(Decl)
                            )
                        )
                    )
                ).bind("type")
            ),
            unless(
                hasAncestor(AnonNamespace)
            )
        );

    // Matchers.

    Finder->addMatcher(
        traverse(
            clang::TK_IgnoreUnlessSpelledInSource,
            AnonNamespace.bind("anon_namespace")
        ),
        this
    );

    Finder->addMatcher(
        traverse(
            clang::TK_IgnoreUnlessSpelledInSource,
            expr(
                Expr,
                unless(

                    // Only keep the matches that have no nested name.

                    has(Nested)
                )
            ).bind("expr")
        ),
        this
    );

    Finder->addMatcher(
        traverse(
            clang::TK_IgnoreUnlessSpelledInSource,
            typeLoc(
                Type,
                unless(
                    allOf(

                        // Only keep the top-level matches that have no nested name.

                        anyOf(
                            has(Nested),
                            hasParent(Nested),
                            hasParent(Type)
                        ),

                        // But do not discard template parameters.

                        unless(
                            hasParent(
                                loc(
                                    qualType(
                                        anyOf(
                                            templateSpecializationType(),
                                            templateTypeParmType()
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            ).bind("type")
        ),
        this
    );

    Finder->addMatcher(
        traverse(
            clang::TK_IgnoreUnlessSpelledInSource,
            nestedNameSpecifierLoc(
                Nested,
                unless(

                    // Only keep the top-level matches that are not part of
                    // a using declaration.

                    hasParent(Nested)
                )
            ).bind("nested")
        ),
        this
    );

    Finder->addMatcher(
        traverse(
            clang::TK_IgnoreUnlessSpelledInSource,
            decl(DeclType)
        ),
        this
    );
}

void
DisambiguateSymbolsTool::
run(
    const MatchFinder::MatchResult &Result
)
{
    clang::FileID FileID = Result.SourceManager->getMainFileID();
    llvm::Optional<clang::StringRef> FilePath
        = Result.SourceManager->getNonBuiltinFilenameForID(FileID);
    std::string ModuleName = getModuleName(*FilePath, this->RootPath);

    if (
        const auto *MatchedAnonNamespace
            = Result.Nodes.getNodeAs<clang::NamespaceDecl>(
                "anon_namespace"
            )
        )
    {
        clang::SourceLocation Begin
            = getBeginLocationForAnonNamespace(Result, MatchedAnonNamespace);
        clang::SourceLocation End
            = getEndLocationForAnonNamespace(Result, MatchedAnonNamespace);

#if DEBUG
        fprintf(
            stderr,
            "----------------------------------------"
            "----------------------------------------\n"
        );
        diag(Result, MatchedAnonNamespace->getBeginLoc(), "[Anon Namespace]");
#endif

        fixNameAnonNamespace(
            this->FileToReplacements,
            Result,
            MatchedAnonNamespace->getLocation(),
            Begin,
            End,
            ModuleName,
            MatchedAnonNamespace
        );
    }
    else if (
        const auto *MatchedExpr
            = Result.Nodes.getNodeAs<clang::Expr>("expr")
    )
    {
        clang::NestedNameSpecifierLoc Nested;
        clang::DeclarationNameInfo DeclNameInfo;
        switch (MatchedExpr->getStmtClass())
        {
            case clang::Stmt::StmtClass::DeclRefExprClass:
            {
                auto Expr
                    = llvm::cast<clang::DeclRefExpr>(MatchedExpr);
                Nested = Expr->getQualifierLoc();
                DeclNameInfo = Expr->getNameInfo();
                break;
            }
            case clang::Stmt::StmtClass::UnresolvedLookupExprClass:
            {
                auto Expr
                    = llvm::cast<clang::UnresolvedLookupExpr>(MatchedExpr);
                Nested = Expr->getQualifierLoc();
                DeclNameInfo = Expr->getNameInfo();
                break;
            }
            default:
            {
                return;
            }
        }

        if (
            DeclNameInfo.getName().getNameKind()
            == clang::DeclarationName::NameKind::CXXOperatorName
        )
        {
            return;
        }

        clang::SourceLocation Begin
            = getBeginLocationForExpr(Result, MatchedExpr, Nested);
        clang::SourceLocation End
            = getEndLocationForExpr(Result, MatchedExpr, Nested);

#if DEBUG
        fprintf(
            stderr,
            "----------------------------------------"
            "----------------------------------------\n"
        );
        diag(Result, MatchedExpr->getBeginLoc(), "matched: ‘%0’ [Expr]")
            << getSourceChars(Result, Begin, End);
#endif

        fixInlineNamespace(
            this->FileToReplacements,
            Result,
            MatchedExpr->getBeginLoc(),
            Begin,
            End,
            ModuleName
        );
    }
    else if (
        const auto *MatchedType
            = Result.Nodes.getNodeAs<clang::TypeLoc>("type")
    )
    {
        clang::SourceLocation Begin
            = getBeginLocationForType(Result, *MatchedType);
        clang::SourceLocation End
            = getEndLocationForType(Result, *MatchedType);

#if DEBUG
        fprintf(
            stderr,
            "----------------------------------------"
            "----------------------------------------\n"
        );
        diag(Result, MatchedType->getBeginLoc(), "matched: ‘%0’ [Type]")
            << getSourceChars(Result, Begin, End);
#endif

        fixInlineNamespace(
            this->FileToReplacements,
            Result,
            MatchedType->getBeginLoc(),
            Begin,
            End,
            ModuleName
        );
    }
    else if (
        const auto *MatchedNested
            = Result.Nodes.getNodeAs<clang::NestedNameSpecifierLoc>("nested")
    )
    {
        clang::SourceLocation Begin
            = getBeginLocationForNested(Result, *MatchedNested);
        clang::SourceLocation End
            = getEndLocationForNested(Result, *MatchedNested);

#if DEBUG
        fprintf(
            stderr,
            "----------------------------------------"
            "----------------------------------------\n"
        );
        diag(Result, MatchedNested->getBeginLoc(), "matched: ‘%0’ [Nested]")
            << getSourceChars(Result, Begin, End);
#endif

        fixInlineNamespace(
            this->FileToReplacements,
            Result,
            MatchedNested->getBeginLoc(),
            Begin,
            End,
            ModuleName
        );
    }
}

} // namespace disambiguate_symbols
} // namespace pxr
