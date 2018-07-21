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

        1. Random-access table;
        2. Optionally, null-set discovery;
        3. Parsing.

    The first two can be done ahead of time by the user, so that the cost can
    be amortized.

    Random-access table
    -------------------
    Generates a fast-lookup table that resolves the location of the sequences
    in the grammar. This allows O(1) lookup of grammar rules.

    Null-set discovery
    ------------------
    Determines all of those rules in the null-kernel. This speeds up certain
    phases of the Earley parser, considerably. This phase is only optional if
    the user doesn't define a null terminal.

*/

typedef int   (*morningGetNextLex)(void* cb, int* nextLex);
typedef void* (*morningSemanticAction)(void* cb, int rule, int subrule, void* previous);
typedef int   (*morningBadParse)(void* cb, int rule, int subrule, int stop);

int morningBuildRandomAccessTable(int* Grammar, int NumRules, int** RAT);
int morningBuildNullKernel(int NumRules, int** RAT, int nullTerminal, int* nullSet);
int morningParse(int NumRules, int** RAT, int nullTerminal, int* nullSet, void* cb, morningGetNextLex GetNextLex, morningSemanticAction Action, morningBadParse badParse);

#ifdef __cplusplus
} // extern "C"
#endif

#endif//MORNING_H

#ifdef  MORNING_CPP_IMPL

#ifdef __cplusplus
extern "C"
{
#endif

int morningBuildRandomAccessTable(int* Grammar, int NumRules, int** RAT)
{
    int* G  = Grammar;
    for (int RR = 0; RR < NumRules; ++RR)
    {
        RAT[RR] = G;
        for (int* AA = G; *AA; ++AA)
        {
            for (int* SS = AA; *SS; ++SS, ++G, ++AA)
            {
            }
        }
    }
    return 1;
}

int morningBuildNullKernel(int NumRules, int** RAT, int nullTerminal, int* nullSet)
{
    if (nullTerminal == 0)
    {
        return 1;
    }
    for (int RR = 0; RR < NumRules; ++RR)
    {
        nullSet[RR] = 0;
    }
    int anyNewSet   = 0;
    do
    {
        anyNewSet   = 0;
        for (int RR = 0; RR < NumRules; ++RR)
        {
            int* Rule   = RAT[RR];
            for (int* AA = Rule; *AA; ++AA)
            {
                int allNullTerminal = 1;
                for (int* SS = AA; *SS; ++SS, ++AA)
                {
                    allNullTerminal &= ((*SS >= NumRules) && (*SS == nullTerminal)) || ((*SS < NumRules) && nullSet[*SS]);
                }
                if (allNullTerminal)
                {
                    anyNewSet   = !nullSet[RR];
                    nullSet[RR] = 1;
                }
            }
        }
    } while (anyNewSet);
    return 1;
}

int morningParse(int NumRules, int** RAT, int nullTerminal, int* nullSet, void* cb, morningGetNextLex GetNextLex, morningSemanticAction Action, morningBadParse badParse)
{
    return 0;
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

    int *RAT[PROD]      = { };
    if (!morningBuildRandomAccessTable(&G[0], PROD, &RAT[0]))
    {
        return 1;
    }

    for (int RR = 0; RR < PROD; ++RR)
    {
        fprintf(stdout, "%d == %d\n", *RAT[RR], RR+1);
    }

    int nullSet[PROD]   = { };
    if (!morningBuildNullKernel(PROD, &RAT[0], END, &nullSet[0]))
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
    if (!morningParse(PROD, &RAT[0], END, &nullSet[0], &gcb, gcbGetNextLex, 0, 0))
    {
        return 1;
    }

    return 0;
}

#endif//MORNING_TESTING
