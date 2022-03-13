// Remove all ‘using’ declarations and make sure that all symbols referencing
// a declaration within the given namespaces are fully qualified.
//
// The code is loosely based on the ‘clang-tools-extra/clang-change-namespace’
// tool from the llvm project.

#define DEBUG 0

#include "InlineNamespaces.h"
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
#include <map>
#include <string>

using namespace clang::ast_matchers;

namespace pxr {
namespace inline_namespaces {

namespace {

/* Namespaces                                                      O-(''Q)
   -------------------------------------------------------------------------- */

llvm::SmallVector<llvm::StringRef>
buildNamespace(
    const clang::NamespaceDecl *Namespace
)
{
    llvm::SmallVector<llvm::StringRef> Out;

    const auto *Ctx = llvm::cast<clang::DeclContext>(Namespace);
    for (; Ctx; Ctx = Ctx->getParent())
    {
        if (!llvm::isa<clang::NamespaceDecl>(Ctx))
        {
            continue;
        }

        const clang::NamespaceDecl *Decl
            = llvm::cast<clang::NamespaceDecl>(Ctx);

        if (Decl->isInline())
        {
            continue;
        }


        Out.push_back(Decl->getName());
    }

    std::reverse(Out.begin(), Out.end());;
    return Out;
}

llvm::SmallVector<llvm::StringRef>
splitNamespace(
    llvm::StringRef Namespace
)
{
    llvm::SmallVector<llvm::StringRef> Out;
    Namespace.split(Out, "::", -1, false);
    return Out;
}

std::string
joinNamespace(
    llvm::SmallVector<llvm::StringRef> Namespace
)
{
    return llvm::join(Namespace.begin(), Namespace.end(), "::");
}

std::string
joinPartialNamespace(
    llvm::SmallVector<llvm::StringRef>::const_iterator Begin,
    llvm::SmallVector<llvm::StringRef>::const_iterator End
)
{
    return llvm::join(Begin, End, "::");
}

bool
filterNamespace(
    llvm::SmallVector<llvm::StringRef> Namespace
)
{
    return (

        // Only consider inlining some specific namespaces...

        (
            Namespace[0] != "boost"
            && Namespace[0] != "std"
        )

        // ... with a few exceptions.

        || (
            Namespace[0] == "boost"
            && Namespace.size() > 1
            && (
                Namespace[1] == "hash_value"
            )
        )
        || (
            Namespace[0] == "std"
            && Namespace.size() > 1
            && (
                Namespace[1] == "chrono_literals"
                || Namespace[1] == "swap"
            )
        )
    );
}

bool
filterSymbol(
    llvm::SmallVector<llvm::StringRef> Namespace,
    llvm::StringRef SymbolName
)
{
    return (
        Namespace[0] == "std"
        && SymbolName == "swap"
    );
}

void
shortenDeclNamespace(
    llvm::SmallVector<llvm::StringRef> &Namespace,
    const MatchFinder::MatchResult &Result,
    const clang::DeclContext *Context,
    clang::SourceLocation Loc,
    const llvm::SmallVector<const clang::NamespaceAliasDecl *> &NamespaceAliasDeps,
    const llvm::SmallVector<const clang::UsingDecl *> &UsingDeps,
    const llvm::SmallVector<const clang::UsingDirectiveDecl *> &UsingNamespaceDeps
)
{
    // TODO: avoid hardcoding workarounds for ‘boost::python::object’,
    //       ‘boost::iterators’, ‘boost::operators_impl’, and others.
}

/* Fixers                                                          O-(''Q)
   -------------------------------------------------------------------------- */

void
fixRemoveUsingNamespace(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    const MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    clang::SourceLocation Begin,
    clang::SourceLocation End,
    const clang::Decl *MatchedUsing,
    clang::NestedNameSpecifierLoc Nested
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

    // Retrieve the source range matching the full namespace.

    llvm::Optional<clang::Token> EndToken
        = clang::Lexer::findNextToken(
            MatchedUsing->getEndLoc(),
            *Result.SourceManager,
            Result.Context->getLangOpts()
        );

    if (!EndToken)
    {
        return;
    }

    clang::SourceLocation NamespaceBegin = Nested.getBeginLoc();
    clang::SourceLocation NamespaceEnd = EndToken->getLocation();

    if (!NamespaceBegin.isValid())
    {
        NamespaceBegin = MatchedUsing->getLocation();
    }

    // Retrieve the actual namespace value.

    llvm::SmallVector<llvm::StringRef> Namespace
        = splitNamespace(getSourceChars(Result, NamespaceBegin, NamespaceEnd));

    // Filter it.

    if (filterNamespace(Namespace))
    {
        return;
    }

    // Apply the changes.

    createRemoval(
        FileToReplacements,
        Result,
        Begin,
        clang::SourceRange(Begin, End),
        "remove using"
    );
}

void
fixInlineNamespace(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    const llvm::SmallVector<const clang::NamespaceAliasDecl *> &NamespaceAliasDeps,
    const llvm::SmallVector<const clang::UsingDecl *> &UsingDeps,
    const llvm::SmallVector<const clang::UsingDirectiveDecl *> &UsingNamespaceDeps,
    const MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    clang::SourceLocation Begin,
    clang::SourceLocation End,
    llvm::StringRef SymbolName,
    const clang::NamespaceDecl *MatchedNamespace,
    const clang::Decl *MatchedContext
)
{
    clang::SourceManager *SourceMgr = Result.SourceManager;

    if (!Loc.isValid() || !MatchedNamespace->getSourceRange().isValid())
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

    // Retrieve the reference's context.

    const clang::DeclContext *RefContext
        = MatchedContext ? MatchedContext->getDeclContext() : nullptr;

    // Retrieve the declaration's namespace.

    llvm::SmallVector<llvm::StringRef> DeclNamespace
        = buildNamespace(MatchedNamespace);
    shortenDeclNamespace(
        DeclNamespace,
        Result,
        RefContext,
        Loc,
        NamespaceAliasDeps,
        UsingDeps,
        UsingNamespaceDeps
    );
    assert(!DeclNamespace.empty());

    // Filter namespaces.

    if (
        filterNamespace(DeclNamespace)
        || filterSymbol(DeclNamespace, SymbolName)
    )
    {
        return;
    }

    // Read the reference's namespace.

    llvm::SmallVector<llvm::StringRef> RefNamespace;
    if (Begin.isValid() && End.isValid() && Begin != End)
    {
        RefNamespace = splitNamespace(getSourceChars(Result, Begin, End));
    }

    // Exit early if both namespaces match.

    if (RefNamespace == DeclNamespace)
    {
        return;
    }

#if DEBUG
        diag(Result, Begin, "‘%0’ -> ‘%1’")
            << joinNamespace(RefNamespace)
            << joinNamespace(DeclNamespace);
#endif

    size_t DeclBeginPos = 0;
    size_t DeclEndPos = DeclNamespace.size();

    // Strip to the right any undesired namespace.
    // TODO: instead of hardcoding some namespaces to lookup, implement this
    //       more procedurally in `shortenDeclNamespace()`.

    for (size_t I = DeclEndPos; I > DeclBeginPos; --I)
    {
        if (
            DeclNamespace[I - 1].startswith("__")
            || (
                DeclNamespace[I - 1] == "operators_impl"
                && I > 1
                && DeclNamespace[I - 2] == "boost"
            )
            || (
                DeclNamespace[I - 1] == "iterators"
                && I > 1
                && DeclNamespace[I - 2] == "boost"
            )
            || (
                DeclNamespace[I - 1] == "api"
                && I > 2
                && DeclNamespace[I - 2] == "python"
                && DeclNamespace[I - 3] == "boost"
                && SymbolName == "object"
            )
            || (
                DeclNamespace[I - 1] == "self_ns"
                && I > 2
                && DeclNamespace[I - 2] == "python"
                && DeclNamespace[I - 3] == "boost"
                && (
                    SymbolName == "self"
                    || SymbolName == "self_t"
                )
            )
        )
        {
            DeclEndPos = I - 1;
            break;
        }
    }

    // Figure out which parts are needed for the referenced namespace to match
    // the one that is declared.

    size_t RefEndPos = RefNamespace.size();

    while (RefEndPos > 0 && DeclEndPos > 0)
    {
        if (RefNamespace[RefEndPos - 1] != DeclNamespace[DeclEndPos - 1])
        {
            break;
        }

        --RefEndPos;
        --DeclEndPos;
    }

    // See if we can shorten the parts needed by using the enclosing namespace.

    if (RefContext)
    {
        const clang::DeclContext *Enclosing
            = RefContext->getEnclosingNamespaceContext();
        if (Enclosing && llvm::isa<clang::NamespaceDecl>(Enclosing))
        {
            const auto *RefContextDecl
                = llvm::cast<clang::NamespaceDecl>(Enclosing);
            if (!RefContextDecl->isAnonymousNamespace())
            {
                llvm::SmallVector<llvm::StringRef> RefContextParts
                    = splitNamespace(RefContextDecl->getQualifiedNameAsString());

                while (
                    DeclBeginPos < RefContextParts.size()
                    && DeclBeginPos < DeclNamespace.size()
                )
                {
                    if (
                        RefContextParts[DeclBeginPos]
                        != DeclNamespace[DeclBeginPos]
                    )
                    {
                        break;
                    }

                    ++DeclBeginPos;
                }
            }
        }
    }

    if (DeclBeginPos >= DeclEndPos)
    {
        return;
    }

    // Build the namespace from the declaration that needs to be inserted.

    std::string Namespace = joinPartialNamespace(
        DeclNamespace.begin() + DeclBeginPos, DeclNamespace.begin() + DeclEndPos
    );
    Namespace += "::";

    // Prepend the new namespace onto the one already existing.

    if (RefEndPos == 0)
    {
        createInsertion(
            FileToReplacements, Result, Begin, Namespace, "inline namespace"
        );
        return;
    }

    // Replace part of the existing namespace with the new one.

    size_t Length = 0;
    for (
        auto It = RefNamespace.begin();
        It != RefNamespace.begin() + RefEndPos;
        ++It
    )
    {
        Length += (*It).size() + 2;
    }

    assert(Length > 0);
    createReplacement(
        FileToReplacements,
        Result,
        Begin,
        clang::SourceRange(Begin, Begin.getLocWithOffset(Length - 1)),
        Namespace,
        "inline namespace"
    );
}

} // anonymous namespace

/* Class Implementation                                            O-(''Q)
   -------------------------------------------------------------------------- */

InlineNamespacesTool::
InlineNamespacesTool(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    llvm::StringRef FilePattern
) :
    FileToReplacements(FileToReplacements),
    FilePattern(FilePattern)
{
}

void
InlineNamespacesTool::
registerMatchers(
    MatchFinder *Finder
)
{
    // Match all the ‘using’ directives to be removed.

    auto Using
        = decl(
            isExpansionInMainFile(),
            anyOf(
                namespaceAliasDecl(),
                usingDecl(),
                usingDirectiveDecl()
            )
        );

    // Declarations referred by expressions and types.

    auto Decl
        = namedDecl(
            hasAncestor(
                namespaceDecl().bind("namespace")
            ),
            unless(
                typedefNameDecl(
                    unless(
                        hasDeclContext(
                            equalsBoundNode("namespace")
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
                    hasDeclaration(Decl.bind("decl"))
                ),
                unresolvedLookupExpr(
                    hasAnyDeclaration(Decl.bind("decl"))
                )
            ),
            unless(
                // Ignore literals such as the ones from chrono.

                hasAncestor(
                    userDefinedLiteral()
                )
            ),
            optionally(
                hasAncestor(
                    decl().bind("context")
                )
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
                    hasDeclaration(Decl.bind("decl"))
                )
            ),
            unless(

                // Ignore types referring to template parameters.

                loc(
                    substTemplateTypeParmType()
                )
            ),
            optionally(
                hasAncestor(
                    decl().bind("context")
                )
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
            specifiesTypeLoc(
                Type
            ),
            optionally(
                hasAncestor(
                    decl().bind("context")
                )
            )
        );

    // Matchers to record ‘using’, ‘namespace’, and ‘namespace alias’
    // declarations that can be used to shorten some namespace specifiers.

    Finder->addMatcher(
        traverse(
            clang::TK_IgnoreUnlessSpelledInSource,
            namespaceAliasDecl().bind("namespace_alias_dep")
        ),
        this
    );

    Finder->addMatcher(
        traverse(
            clang::TK_IgnoreUnlessSpelledInSource,
            usingDecl().bind("using_dep")
        ),
        this
    );

    Finder->addMatcher(
        traverse(
            clang::TK_IgnoreUnlessSpelledInSource,
            usingDirectiveDecl().bind("using_namespace_dep")
        ),
        this
    );

    // Matchers.

    Finder->addMatcher(
        traverse(
            clang::TK_IgnoreUnlessSpelledInSource,
            Using.bind("using")
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

                    anyOf(
                        hasParent(Nested),
                        hasParent(Using)
                    )
                )
            ).bind("nested")
        ),
        this
    );
}

void
InlineNamespacesTool::
run(
    const MatchFinder::MatchResult &Result
)
{
    if (
        const auto *MatchedNamespaceAliasDep
            = Result.Nodes.getNodeAs<clang::NamespaceAliasDecl>(
                "namespace_alias_dep"
            )
        )
    {
        this->NamespaceAliasDeps.push_back(MatchedNamespaceAliasDep);
    }
    else if (
        const auto *MatchedUsingDep
            = Result.Nodes.getNodeAs<clang::UsingDecl>(
                "using_dep"
            )
        )
    {
        this->UsingDeps.push_back(MatchedUsingDep);
    }
    else if (
        const auto *MatchedUsingNamespaceDep
            = Result.Nodes.getNodeAs<clang::UsingDirectiveDecl>(
                "using_namespace_dep"
            )
        )
    {
        this->UsingNamespaceDeps.push_back(MatchedUsingNamespaceDep);
    }
    else if (
        const auto *MatchedUsing
            = Result.Nodes.getNodeAs<clang::Decl>("using")
    )
    {
        clang::NestedNameSpecifierLoc Nested;
        switch (MatchedUsing->getKind())
        {
            case clang::Decl::Kind::NamespaceAlias:
            {
                const auto *Using
                    = llvm::cast<clang::NamespaceAliasDecl>(MatchedUsing);
                Nested = Using->getQualifierLoc();
                break;
            }
            case clang::Decl::Kind::Using:
            {
                const auto *Using
                    = llvm::cast<clang::UsingDecl>(MatchedUsing);
                Nested = Using->getQualifierLoc();
                break;
            }
            case clang::Decl::Kind::UsingDirective:
            {
                const auto *Using
                    = llvm::cast<clang::UsingDirectiveDecl>(MatchedUsing);
                Nested = Using->getQualifierLoc();
                break;
            }
            default:
            {
                return;
            }
        }

        clang::SourceLocation Begin
            = getBeginLocationForUsing(Result, MatchedUsing);
        clang::SourceLocation End
            = getEndLocationForUsing(Result, MatchedUsing);

#if DEBUG
        fprintf(
            stderr,
            "----------------------------------------"
            "----------------------------------------\n"
        );
        diag(Result, MatchedUsing->getBeginLoc(), "matched: ‘%0’ [Using]")
            << getSourceChars(Result, Begin, End);
#endif

        fixRemoveUsingNamespace(
            this->FileToReplacements,
            Result,
            MatchedUsing->getLocation(),
            Begin,
            End,
            MatchedUsing,
            Nested
        );
    }
    else if (
        const auto *MatchedExpr
            = Result.Nodes.getNodeAs<clang::Expr>("expr")
    )
    {
        clang::NestedNameSpecifierLoc Nested;
        clang::StringRef SymbolName;
        clang::DeclarationNameInfo DeclNameInfo;
        switch (MatchedExpr->getStmtClass())
        {
            case clang::Stmt::StmtClass::DeclRefExprClass:
            {
                auto Expr
                    = llvm::cast<clang::DeclRefExpr>(MatchedExpr);
                Nested = Expr->getQualifierLoc();
                SymbolName
                    = getSourceTokens(
                        Result, Expr->getLocation(), Expr->getLocation()
                    );
                DeclNameInfo = Expr->getNameInfo();
                break;
            }
            case clang::Stmt::StmtClass::UnresolvedLookupExprClass:
            {
                auto Expr
                    = llvm::cast<clang::UnresolvedLookupExpr>(MatchedExpr);
                Nested = Expr->getQualifierLoc();
                SymbolName
                    = getSourceTokens(
                        Result, Expr->getNameLoc(), Expr->getNameLoc()
                    );
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

        const auto *MatchedNamespace
            = Result.Nodes.getNodeAs<clang::NamespaceDecl>("namespace");
        const auto *MatchedContext
            = Result.Nodes.getNodeAs<clang::Decl>("context");

        fixInlineNamespace(
            this->FileToReplacements,
            this->NamespaceAliasDeps,
            this->UsingDeps,
            this->UsingNamespaceDeps,
            Result,
            MatchedExpr->getBeginLoc(),
            Begin,
            End,
            SymbolName,
            MatchedNamespace,
            MatchedContext
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

        clang::TypeLoc Symbol = *MatchedType;
        while (Symbol.getNextTypeLoc())
        {
            Symbol = Symbol.getNextTypeLoc();
        }

        clang::StringRef SymbolName
            = getSourceTokens(
                Result, Symbol.getBeginLoc(), Symbol.getBeginLoc()
            );

#if DEBUG
        fprintf(
            stderr,
            "----------------------------------------"
            "----------------------------------------\n"
        );
        diag(Result, MatchedType->getBeginLoc(), "matched: ‘%0’ [Type]")
            << getSourceChars(Result, Begin, End);
#endif

        const auto *MatchedNamespace
            = Result.Nodes.getNodeAs<clang::NamespaceDecl>("namespace");
        const auto *MatchedContext
            = Result.Nodes.getNodeAs<clang::Decl>("context");

        fixInlineNamespace(
            this->FileToReplacements,
            this->NamespaceAliasDeps,
            this->UsingDeps,
            this->UsingNamespaceDeps,
            Result,
            MatchedType->getBeginLoc(),
            Begin,
            End,
            SymbolName,
            MatchedNamespace,
            MatchedContext
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

        clang::StringRef SymbolName
            = getSourceTokens(
                Result,
                MatchedNested->getEndLoc().getLocWithOffset(2),
                MatchedNested->getEndLoc().getLocWithOffset(2)
            );

#if DEBUG
        fprintf(
            stderr,
            "----------------------------------------"
            "----------------------------------------\n"
        );
        diag(Result, MatchedNested->getBeginLoc(), "matched: ‘%0’ [Nested]")
            << getSourceChars(Result, Begin, End);
#endif

        const auto *MatchedNamespace
            = Result.Nodes.getNodeAs<clang::NamespaceDecl>("namespace");
        const auto *MatchedContext
            = Result.Nodes.getNodeAs<clang::Decl>("context");

        fixInlineNamespace(
            this->FileToReplacements,
            this->NamespaceAliasDeps,
            this->UsingDeps,
            this->UsingNamespaceDeps,
            Result,
            MatchedNested->getBeginLoc(),
            Begin,
            End,
            SymbolName,
            MatchedNamespace,
            MatchedContext
        );
    }
}

} // namespace inline_namespaces
} // namespace pxr
