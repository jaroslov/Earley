#ifndef MORNING_H
#define MORNING_H

#ifdef __cplusplus
extern "C"
{
#endif

/*

    A grammar is defined like so:

        1. There is a termination value 0.
        2. There are non-terminals, with values in the range 1 <= nt <= N.
        3. There are terminals, with values in the range 0 < N < t.
        4. A sequence is an array of terminals and non-terminals, ending with the termination value.
        5. An alternation is an array of sequences, ending with the termination value.
        6. A grammar is an array of alternations, ending with the termination value.

    Example:

        enum { END, SUM, PROD, PLUS, NUM };

        int G[] =
        {
            SUM, PLUS, PROD, END,
            PROD, END,
            END,

            PROD, MUL, NUM, END,
            NUM, END,
            END,

            END,
        };

    Which corresponds to the grammar:

        START ::= SUM
          SUM ::= SUM `+` PROD
               |  PROD
         PROD ::= PROD `*` <num>
               |  <num>

    If the user wants to use epsilon (null) terminals, then the user must announce
    to the system a non-zero value for the null terminal. (See the API, below).

    There are three phases to the Earley parser generator:

        1. Random-access and alternate random-access table;
        2. Optionally, null-set discovery;
        3. Parsing.

    The first two can be done ahead of time by the user, so that the cost can
    be amortized.

    Random-access table
    -------------------
    Generates a fast-lookup table that resolves the location of the sequences
    in the grammar. This allows O(1) lookup of grammar rules. There are two
    tables:
        int RAT[][2]    range of alternates
        int *ARAT[]     points into the grammar
    The RAT[][2] points to a subset of the ARAT table.
    The *ARAT[] points into the grammar

    The user is responsible for sizing both of these arrays.

    Null-set discovery
    ------------------
    Determines all of those rules in the null-kernel. This speeds up certain
    phases of the Earley parser, considerably. This phase is only optional if
    the user doesn't define a null terminal.

*/

typedef int   (*morningGetNextLex)(void* cb, int* nextLex);
typedef int   (*morningAddItem)(void* cb, int index, int rule, int alt, int pos);
typedef void* (*morningSemanticAction)(void* cb, int rule, int subrule, void* previous);
typedef int   (*morningBadParse)(void* cb, int rule, int subrule, int stop);

typedef void* MorningParseOpts;

int morningParseOptsSize();
MorningParseOpts morningMakeParseOpts(void* scratch);

int morningAddGrammar(MorningParseOpts, int* Grammar, int NumRules);
int morningAddRandomAccessTable(MorningParseOpts, int (*RAT)[2], int** ARAT);
int morningAddNullKernel(MorningParseOpts, int nullTerminal, int* nullSet);
int morningAddGetNextLex(MorningParseOpts, morningGetNextLex);
int morningAddAddItem(MorningParseOpts, morningAddItem);
int morningAddSemanticActions(MorningParseOpts, morningSemanticAction);
int morningAddBadParse(MorningParseOpts, morningBadParse);

int morningBuildRandomAccessTable(MorningParseOpts);
int morningBuildNullKernel(MorningParseOpts);
int morningParse(MorningParseOpts, void* cb);

#ifdef __cplusplus
} // extern "C"
#endif

#endif//MORNING_H

#ifdef  MORNING_CPP_IMPL

#ifdef  MORNING_TESTING
#include <stdio.h>
#endif//MORNING_TESTING

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct MorningParseOptsT
{
    int*                    Grammar;
    int                     NumRules;
    int                   (*RAT)[2];
    int**                   ARAT;
    int                     NullTerminal;
    int*                    NullSet;
    morningGetNextLex       GetNextLex;
    morningAddItem          AddItem;
    morningSemanticAction   SemanticAction;
    morningBadParse         BadParse;
} MorningParseOptsT;

int morningParseOptsSize()
{
    return sizeof(MorningParseOptsT);
}

MorningParseOpts morningMakeParseOpts(void* scratch)
{
    for (int II = 0; II < sizeof(MorningParseOptsT); ++II)
    {
        ((unsigned char*)scratch)[II]   = 0;
    }
    return (MorningParseOpts)scratch;
}

int morningAddGrammar(MorningParseOpts mp, int* Grammar, int NumRules)
{
    MorningParseOptsT* mpo  = (MorningParseOptsT*)mp;
    if (!mpo) return 0;
    if (!Grammar) return 0;
    if (NumRules < 1) return 0;
    mpo->Grammar    = Grammar;
    mpo->NumRules   = NumRules;
    return 1;
}

int morningAddRandomAccessTable(MorningParseOpts mp, int (*RAT)[2], int** ARAT)
{
    MorningParseOptsT* mpo  = (MorningParseOptsT*)mp;
    if (!mpo) return 0;
    if (!RAT) return 0;
    mpo->RAT    = RAT;
    mpo->ARAT   = ARAT;
    return 1;
}

int morningAddNullKernel(MorningParseOpts mp, int nullTerminal, int* nullSet)
{
    MorningParseOptsT* mpo  = (MorningParseOptsT*)mp;
    if (!mpo) return 0;
    if (!nullSet) return 0;
    mpo->NullTerminal   = nullTerminal;
    mpo->NullSet        = nullSet;
    return 1;
}

int morningAddGetNextLex(MorningParseOpts mp, morningGetNextLex GetNextLex)
{
    MorningParseOptsT* mpo  = (MorningParseOptsT*)mp;
    if (!mpo) return 0;
    if (!GetNextLex) return 0;
    mpo->GetNextLex     = GetNextLex;
    return 1;
}

int morningAddAddItem(MorningParseOpts mp, morningAddItem AddItem)
{
    MorningParseOptsT* mpo  = (MorningParseOptsT*)mp;
    if (!mpo) return 0;
    if (!AddItem) return 0;
    mpo->AddItem        = AddItem;
    return 1;
}

int morningAddSemanticActions(MorningParseOpts mp, morningSemanticAction SemanticAction)
{
    MorningParseOptsT* mpo  = (MorningParseOptsT*)mp;
    if (!mpo) return 0;
    if (!SemanticAction) return 0;
    mpo->SemanticAction = SemanticAction;
    return 1;
}

int morningAddBadParse(MorningParseOpts mp, morningBadParse BadParse)
{
    MorningParseOptsT* mpo  = (MorningParseOptsT*)mp;
    if (!mpo) return 0;
    if (!BadParse) return 0;
    mpo->BadParse       = BadParse;
    return 1;
}

int morningBuildRandomAccessTable(MorningParseOpts mp)
{
    MorningParseOptsT* mpo  = (MorningParseOptsT*)mp;
    if (!mpo) return 0;
    if (!mpo->Grammar) return 0;
    if (mpo->NumRules < 1) return 0;
    int* G  = mpo->Grammar;
    for (int RR = 0, II = 0; RR < mpo->NumRules; ++RR, ++G)
    {
        mpo->RAT[RR][0]     = II;
        for (int* AA = G; *AA; ++II)
        {
            mpo->ARAT[II]   = AA;
            for (int* SS = AA; *SS; ++SS, ++AA)
            {
            }
            ++AA;
            G   = AA;
        }
        mpo->RAT[RR][1]     = II;
    }
    return 1;
}

int morningBuildNullKernel(MorningParseOpts mp)
{
    MorningParseOptsT* mpo  = (MorningParseOptsT*)mp;
    if (!mpo) return 0;
    if (mpo->NullTerminal == 0)
    {
        return 1;
    }
    for (int RR = 0; RR < mpo->NumRules; ++RR)
    {
        mpo->NullSet[RR] = 0;
    }
    int anyNewSet   = 0;
    do
    {
        anyNewSet   = 0;
        for (int RR = 0; RR < mpo->NumRules; ++RR)
        {
            for (int AA = mpo->RAT[RR][0]; AA < mpo->RAT[RR][1]; ++AA)
            {
                int allNullTerminal = 1;
                for (int* SS = mpo->ARAT[AA]; *SS; ++SS)
                {
                    allNullTerminal &= ((*SS >= mpo->NumRules) && (*SS == mpo->NullTerminal)) || ((*SS < mpo->NumRules) && mpo->NullSet[*SS]);
                }
                if (allNullTerminal)
                {
                    anyNewSet   = !mpo->NullSet[RR];
                    mpo->NullSet[RR] = 1;
                }
            }
        }
    } while (anyNewSet);
    return 1;
}

int morningParse(MorningParseOpts mp, void* cb)
{
    MorningParseOptsT* mpo  = (MorningParseOptsT*)mp;

    for (int AA = mpo->RAT[0][0]; AA < mpo->RAT[0][1]; ++AA)
    {
        if (mpo->AddItem(cb, 0, 0, AA - mpo->RAT[0][0], 0) < 0)
        {
            return 0; // Something went really wrong.
        }
    }

    for (int index = 0, lex = 0; mpo->GetNextLex(cb, &lex); ++index, lex = 0)
    {
        // Prediction, Scanning, Completion.
        // AddItem()
        //  > 0 : success, new item added
        //  = 0 : success, no item added
        //  < 0 : something went wrong
    }

    return 1;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif//MORNING_CPP_IMPL

#ifdef  MORNING_TESTING

#include <stdio.h>

int test();

int main(int argc, char *argv[])
{
    return test();
}

typedef struct Gcb
{
    int*    begin;
    int*    cursor;
    int*    end;
} Gcb;

int gcbGetNextLex(void* cb, int* nextLex)
{
    if (!cb) return 0;
    Gcb& gcb    = *(Gcb*)cb;

    if (gcb.begin > gcb.cursor)
    {
        return 0;
    }

    if (gcb.cursor >= gcb.end)
    {
        return 0;
    }

    *nextLex    = *gcb.cursor;
    ++gcb.cursor;

    return 1;
}

int test()
{
    enum { END, SUM, PROD, PLUS, MUL, NUM };

    int G[] =
    {
        /*  SUM ::= */ SUM, PLUS, PROD, END,
        /*       |  */ PROD, END,
        END,

        /* PROD ::= */ PROD, MUL, NUM, END,
        /*       |  */ NUM, END,
        END,

        END,
    };

    unsigned char cmpo[morningParseOptsSize()];
    MorningParseOpts mpo    = morningMakeParseOpts(&cmpo[0]);
    morningAddGrammar(mpo, &G[0], PROD);

    int RAT[PROD][2]        = { };
    int* ARAT[4]            = { };
    morningAddRandomAccessTable(mpo, RAT, ARAT);

    if (!morningBuildRandomAccessTable(mpo))
    {
        return 1;
    }

    for (int RR = 0; RR < PROD; ++RR)
    {
        fprintf(stdout, "%d == %d\n", *RAT[RR], RR+1);
    }

    int nullSet[PROD]   = { };
    morningAddNullKernel(mpo, END, &nullSet[0]);

    if (!morningBuildNullKernel(mpo))
    {
        return 1;
    }
    for (int RR = 0; RR < PROD; ++RR)
    {
        fprintf(stdout, "%d%s\n", (RR+1), nullSet[RR] ? " is null." : " is not null.");
    }

    int lexemes[]   = { NUM };
    Gcb gcb =
    {
        .begin  = &lexemes[0],
        .cursor = &lexemes[0],
        .end    = &lexemes[0] + sizeof(lexemes) / sizeof(lexemes[0]),
    };

    morningAddGetNextLex(mpo, gcbGetNextLex);
    //morningAddSemanticActions(mpo, morningSemanticAction);
    //morningAddBadParse(mpo, morningBadParse);

    if (!morningParse(mpo, (void*)&gcb))
    {
        return 1;
    }

    return 0;
}

#endif//MORNING_TESTING
