#include "Helpers.h"
#include "Replacements.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/CharInfo.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Tooling/Core/Replacement.h>
#include <llvm/Support/ErrorHandling.h>

#include <cstdint>
#include <map>
#include <string>

namespace {

clang::SourceRange
expandRangeWithBlanks(
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceRange Range
)
{
    clang::SourceLocation BeginLoc = Range.getBegin();
    clang::SourceLocation EndLoc = Range.getEnd();

    const char *C;

    // Find the beginning of the line.

    const char *Begin = Result.SourceManager->getCharacterData(BeginLoc);
    C = Begin;
    while (clang::isHorizontalWhitespace(*(C - 1)))
    {
        --C;
    }

    if (*(C - 1) == '\n')
    {
        int Offset = uintptr_t(Begin) - uintptr_t(C);
        BeginLoc = BeginLoc.getLocWithOffset(-Offset);
    }

    // Find the end of the line.

    const char *End = Result.SourceManager->getCharacterData(EndLoc);
    C = End;
    while (clang::isHorizontalWhitespace(*C))
    {
        ++C;
    }

    if (*C == '\n')
    {
        int Offset = uintptr_t(C) - uintptr_t(End);
        EndLoc = EndLoc.getLocWithOffset(Offset);
    }

    return clang::SourceRange(BeginLoc, EndLoc);
}

} // anonymous namespace

void
pxr::
registerReplacement(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    clang::tooling::Replacement Replacement,
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    const char *Label,
    const clang::FixItHint &Fix
)
{
    // Precautions were taken to avoid overlapping matchers but here's an
    // extra safety to make sure that duplicated work won't be applied.

    clang::tooling::Replacements &Replacements
        = (*FileToReplacements)[std::string(Replacement.getFilePath())];
    for (auto It = Replacements.begin(); It != Replacements.end(); ++It)
    {
        if (
            (*It).getFilePath() == Replacement.getFilePath()
            && (*It).getOffset() == Replacement.getOffset()
        )
        {
            if (
                (*It).getLength() != Replacement.getLength()
                || (*It).getReplacementText() != Replacement.getReplacementText()
            )
            {
                llvm_unreachable(
                    "Already existing replacement with different content.\n"
                );
            }

            return;
        }
    }

    // Register the replacement within the referenced map object.

    auto Error = Replacements.add(Replacement);
    if (Error)
    {
        std::string Msg = llvm::toString(std::move(Error));
        llvm_unreachable(Msg.c_str());
    }

    // Print a user-friendly diagnostic to inform about what to expect.

    pxr::diag(Result, Loc, Label) << Fix;
}

void
pxr::
createInsertion(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    llvm::StringRef Value,
    const char *Label
)
{
    auto Replacement
        = clang::tooling::Replacement(*Result.SourceManager, Loc, 0, Value);
    clang::FixItHint Fix
        = clang::FixItHint::CreateInsertion(Loc, Value);
    pxr::registerReplacement(
        FileToReplacements, Replacement, Result, Loc, Label, Fix
    );
}

void
pxr::
createReplacement(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    clang::SourceRange Range,
    llvm::StringRef Value,
    const char *Label
)
{
    auto Replacement
        = clang::tooling::Replacement(
            *Result.SourceManager,
            clang::CharSourceRange::getTokenRange(Range),
            Value
        );
    clang::FixItHint Fix
        = clang::FixItHint::CreateReplacement(Range, Value);
    pxr::registerReplacement(
        FileToReplacements, Replacement, Result, Loc, Label, Fix
    );
}

void
pxr::
createRemoval(
    std::map<std::string, clang::tooling::Replacements> *FileToReplacements,
    const clang::ast_matchers::MatchFinder::MatchResult &Result,
    clang::SourceLocation Loc,
    clang::SourceRange Range,
    const char *Label
)
{
    Range = expandRangeWithBlanks(Result, Range);

    // Locations were treated as tokens until now but they need to be converted
    // to characters instead to correctly remove the newline character, hence
    // we pick the next location before converting it to a character range.

    Range.setEnd(Range.getEnd().getLocWithOffset(1));

    auto Replacement
        = clang::tooling::Replacement(
            *Result.SourceManager,
            clang::CharSourceRange::getCharRange(Range),
            ""
        );
    clang::FixItHint Fix
        = clang::FixItHint::CreateRemoval(Range);
    pxr::registerReplacement(
        FileToReplacements, Replacement, Result, Loc, Label, Fix
    );
}
