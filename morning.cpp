#ifndef MORNING_H
#define MORNING_H

#ifdef __cplusplus
extern "C"
{
#endif

/*

    A grammar is defined like so:

        1. There is a termination value 0 (which counts as a rule).
        2. There are non-terminals (rules), with values in the range 0 <= nt < N.
        3. There are terminals, with values in the range N < t.
        4. A sequence is an array of terminals and non-terminals, ending with the termination value.
        5. An alternation is an array of sequences, ending with the termination value; called a rule.
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

    If the user wants to use epsilon (null) terminals, then the they must announce
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
        int ARAT[]      points into the grammar
    The RAT[][2] points to a subset of the ARAT table.
    The ARAT[] points into the grammar

    The user is responsible for sizing both of these arrays. The RAT
    array should be equal to the number of rules (including the mandatory
    end-rule). The ARAT should be equal to the total number of sequences
    in all the rules.

    Null-set discovery
    ------------------
    Determines all of those rules in the null-kernel. This speeds up certain
    phases of the Earley parser, considerably. This phase is only optional if
    the user doesn't define a null terminal.

*/

#define MORNING_PSTATE_TABLE(X) \
    X(ERROR)                    \
    X(INIT)                     \
    X(INIT_ITEMS)               \
    X(SCANNING)                 \
    X(COMPLETION)               \
    X(PREDICTION)               \
    X(LEX_NEXT)                 \
    X(ANALYZE_ITEM)             \
    X(GET_NEXT_ITEM)            \
    X(GET_NEXT_PARENT_ITEM)     \
    X(ADD_PARENT_ITEM)          \
    X(NONE)

typedef enum MORNING_PSTATE
{
#undef MORNING_ENTRY
#define MORNING_ENTRY(E) MORNING_PS_ ## E,
MORNING_PSTATE_TABLE(MORNING_ENTRY)
#undef MORNING_ENTRY
} MORNING_PSTATE;

#define MORNING_EVENT_TABLE(X)  \
    X(ERROR)                    \
    X(GET_LEXEME)               \
    X(ADD_ITEM)                 \
    X(GET_NEXT_ITEM)            \
    X(INIT_PARENT_LIST)         \
    X(GET_NEXT_PARENT_ITEM)     \
    X(NONE)

typedef enum MORNING_EVENT
{
#undef MORNING_ENTRY
#define MORNING_ENTRY(E) MORNING_EVT_ ## E,
MORNING_EVENT_TABLE(MORNING_ENTRY)
#undef MORNING_ENTRY
} MORNING_EVENT;

typedef struct MorningItem
{
    int                     Index;
    int                     Rule;
    int                     Alt;
    int                     Dot;
    int                     Source;
} MorningItem;

typedef struct MorningParseState MorningParseState;

int morningParseStateSize();
int morningInitParseState(MorningParseState*);

int morningIsTerminal(MorningParseState*, int NTN);
int morningIsNonterminal(MorningParseState*, int NTN);
int morningIsNull(MorningParseState*, int NTN);
int morningIsInNullKernel(MorningParseState*, int NTN);
int morningSequenceLength(MorningParseState*, int AltStart);
int morningRuleLength(MorningParseState*, int RuleStart);
int morningEndOfGrammar(MorningParseState*, int RuleStart);
int morningRuleBase(MorningParseState*, MorningItem*);
int morningAltBase(MorningParseState*, MorningItem*);
int morningGetNTN(MorningParseState*, MorningItem*);
int morningNumAlternates(MorningParseState*, MorningItem*);

int morningAddGrammar(MorningParseState*, int* Grammar, int NumRules);
int morningAddRandomAccessTable(MorningParseState*, int (*RAT)[2], int* ARAT);
int morningAddNullKernel(MorningParseState*, int nullTerminal, int* nullSet);
int morningSetStartRule(MorningParseState*, int StartRule);
int morningSetLexeme(MorningParseState*, int Lexeme);
int morningSetNewItem(MorningParseState*, MorningItem* NewItem);

int* morningGetGrammar(MorningParseState*);
int morningGetIndex(MorningParseState*);
MORNING_PSTATE morningGetState(MorningParseState*);
MORNING_EVENT morningGetEvent(MorningParseState*);
int morningGetWorkItem(MorningParseState*, MorningItem* WorkItem);

int morningBuildRandomAccessTable(MorningParseState*);
int morningBuildNullKernel(MorningParseState*);
int morningParse(MorningParseState*, void* cb);

int morningParseStep(MorningParseState* mps);
int morningParentTrigger(MorningParseState* mps, MorningItem* item, int WhichRule);

#ifdef __cplusplus
} // extern "C"
#endif

#endif//MORNING_H

#ifdef  MORNING_TESTING

#include <stdio.h>

#include <algorithm>
#include <list>
#include <map>
#include <set>

int test();

int main(int argc, char *argv[])
{
    return test();
}

bool operator< (MorningItem const& left, MorningItem const& right)
{
    int L[5] = { left.Index,  left.Rule,  left.Alt,  left.Dot,  left.Source  };
    int R[5] = { right.Index, right.Rule, right.Alt, right.Dot, right.Source };
    return std::lexicographical_compare(&L[0], &L[0] + sizeof(L)/sizeof(L[0]),
                                        &R[0], &R[0] + sizeof(R)/sizeof(R[0]));
}

typedef struct Gcb
{
    std::map<int, std::set<MorningItem>>    items;
    std::map<int, std::set<MorningItem>>    unused;
    std::list<const MorningItem*>           parents;
} Gcb;

#define G_TABLE(X)  \
    X(END)          \
    X(SUM)          \
    X(PROD)         \
    X(FAC)          \
    X(NUM)          \
    X(LRULE)        \
    X(PLUS)         \
    X(MUL)          \
    X(DIG)          \
    X(LPAR)         \
    X(RPAR)         \
    X(MAX)

enum
{
#undef G_TABLE_X
#define G_TABLE_X(ENUM) ENUM,
    G_TABLE(G_TABLE_X)
#undef G_TABLE_X
};
const char* GS[]    =
{
#undef G_TABLE_X
#define G_TABLE_X(ENUM) #ENUM,
    G_TABLE(G_TABLE_X)
#undef G_TABLE_X
};

void printItem(MorningParseState* mps, MorningItem* item)
{
    fprintf(stdout, "[%8d] %6s ::=", item->Index, GS[item->Rule]);
    int AltStart    = morningAltBase(mps, item);
    int AltLen      = morningSequenceLength(mps, AltStart);
    int prLen       = 0;
    for (int DD = 0; DD < AltLen; ++DD)
    {
        prLen       += fprintf(stdout, " %s%-6s", (item->Dot == DD) ? "*" : " ", GS[morningGetGrammar(mps)[AltStart + DD]]);
    }
    prLen           += fprintf(stdout, "%s", (item->Dot == AltLen) ? "*" : " ");
    fprintf(stdout, "%*.*s (%d)", (28 - prLen), (28 - prLen), "", item->Source);
    //fprintf(stdout, " : %d%d%d%d%d", item->Index, item->Rule, item->Alt, item->Dot, item->Source);
    fprintf(stdout, "%s", "\n");
}

int test()
{
    int G[] =
    {
        /*  SUM ::= */ SUM, PLUS, PROD, END,
        /*       |  */ PROD, END,
        END,

        /* PROD ::= */ PROD, MUL, FAC, END,
        /*       |  */ FAC, END,
        END,

        /*  FAC ::= */ LPAR, SUM, RPAR, END,
        /*       |  */ NUM, END,
        END,

        /*  NUM ::= */ DIG, NUM, END,
        /*       |  */ DIG, END,

        END,
    };

    #define NUM_RULES       (LRULE)
    #define NUM_PRODS       (8)

    unsigned char mpStore[morningParseStateSize()];
    MorningParseState* mps  = (MorningParseState*)&mpStore[0];
    morningInitParseState(mps);

    morningAddGrammar(mps, &G[0], NUM_RULES);

    int RAT[NUM_RULES][2]   = { };
    int ARAT[NUM_PRODS]     = { };
    morningAddRandomAccessTable(mps, RAT, ARAT);

    if (!morningBuildRandomAccessTable(mps))
    {
        return 1;
    }

    for (int RR = 0; RR < PROD; ++RR)
    {
        fprintf(stdout, "%d == %d\n", *RAT[RR], RR+1);
    }

    int nullSet[NUM_RULES]  = { };
    morningAddNullKernel(mps, END, &nullSet[0]);

    if (!morningBuildNullKernel(mps))
    {
        return 1;
    }

    morningSetStartRule(mps, SUM);

    for (int GG = 0; GG < MAX; ++GG)
    {
        fprintf(stdout, "%8s : is terminal?    %d\n", GS[GG], morningIsTerminal(mps, GG));
        fprintf(stdout, "%8s : is nonterminal? %d\n", GS[GG], morningIsNonterminal(mps, GG));
        fprintf(stdout, "%8s : is null?        %d\n", GS[GG], morningIsInNullKernel(mps, GG));
    }

    fprintf(stdout, "LEN R0A0: %d\n", morningSequenceLength(mps, 0));
    int RuleStart   = 0;
    while (!morningEndOfGrammar(mps, RuleStart))
    {
        int Len     = morningRuleLength(mps, RuleStart);
        if (!Len) break;
        fprintf(stdout, "LEN %d\n", Len);
        RuleStart   += Len + 1;
    }

    MorningItem item    = { 0, 1, 0, 0, 0 };
    printItem(mps, &item);
    fprintf(stdout, "What? %s\n", GS[morningGetNTN(mps, &item)]);
    item.Alt            = 1;
    printItem(mps, &item);
    fprintf(stdout, "What? %s\n", GS[morningGetNTN(mps, &item)]);
    item.Rule           = 1;
    item.Alt            = 0;
    printItem(mps, &item);
    fprintf(stdout, "What? %s\n", GS[morningGetNTN(mps, &item)]);
    item.Alt            = 1;
    printItem(mps, &item);
    fprintf(stdout, "What? %s\n", GS[morningGetNTN(mps, &item)]);

    int lexemes[]   = { DIG, PLUS, LPAR, DIG, MUL, DIG, PLUS, DIG, RPAR, END };
    Gcb gcb =
    {
    };

    #undef MORNING_ENTRY
    #define MORNING_ENTRY(X) # X,
    static const char* STATES[] =
    {
        MORNING_PSTATE_TABLE(MORNING_ENTRY)
    };
    #undef MORNING_ENTRY
    #undef MORNING_ACTION
    #define MORNING_ACTION(X) # X,
    static const char* EVENTS[] =
    {
        MORNING_EVENT_TABLE(MORNING_ACTION)
    };
    #undef MORNING_ACTION

    MorningItem NewItem                         = { };
    while (1)
    {
        int result  = morningParseStep(mps);
        if (result != 1)
        {
            break;
        }
        fprintf(stdout, "[%4d] STATE: %s x %s\n", morningGetIndex(mps), STATES[morningGetState(mps)], EVENTS[morningGetEvent(mps)]);
        MorningItem WorkItem                    = { };
        morningGetWorkItem(mps, &WorkItem);
        switch (morningGetEvent(mps))
        {
        case MORNING_EVT_ERROR                  :
            break;
        case MORNING_EVT_GET_LEXEME             :
            fprintf(stdout, "       --> %s.\n", "GET LEXEME");
            morningSetLexeme(mps, lexemes[morningGetIndex(mps)]);
            break;
        case MORNING_EVT_ADD_ITEM               :
            {
                auto iitr       = gcb.items[WorkItem.Index].find(WorkItem);
                if (iitr != gcb.items[WorkItem.Index].end())
                {
                    break;
                }
                fprintf(stdout, "       --> ITEM ADDED to idx %d.\n", WorkItem.Index);
                fprintf(stdout, "%s", "           ");
                printItem(mps, &WorkItem);
                gcb.items[WorkItem.Index].insert(WorkItem);
                gcb.unused[WorkItem.Index].insert(WorkItem);
            }
            break;
        case MORNING_EVT_GET_NEXT_ITEM          :
            {
                fprintf(stdout, "       NUM ITEMS LEFT: %d\n", (int)gcb.unused[morningGetIndex(mps)].size());
                if (gcb.unused[morningGetIndex(mps)].empty())
                {
                    break;
                }
                fprintf(stdout, "       --> %s.\n", "ITEM GOTTEN");
                NewItem                 = *gcb.unused[morningGetIndex(mps)].begin();
                gcb.unused[morningGetIndex(mps)].erase(gcb.unused[morningGetIndex(mps)].begin());
                morningSetNewItem(mps, &NewItem);
                fprintf(stdout, "%s", "           ");
                printItem(mps, &NewItem);
            }
            break;
        case MORNING_EVT_INIT_PARENT_LIST       :
            gcb.parents.clear();
            fprintf(stdout, "       --> %s.\n", "INIT'D PARENT LIST");
            {
                for (auto& item : gcb.items[WorkItem.Source])
                {
                    fprintf(stdout, "%s", "       FILTERING:\n       ");
                    printItem(mps, (MorningItem*)&item);
                    if (morningParentTrigger(mps, (MorningItem*)&item, WorkItem.Rule))
                    {
                        fprintf(stdout, "%s", "       ADDING.\n");
                        gcb.parents.push_back(&item);
                    }
                }
            }
            break;
        case MORNING_EVT_GET_NEXT_PARENT_ITEM   :
            if (!gcb.parents.empty())
            {
                morningSetNewItem(mps, (MorningItem*)gcb.parents.front());
                gcb.parents.pop_front();
            }
            break;
        case MORNING_EVT_NONE                   :
            break;
        }
    }

    fprintf(stdout, "%s", "\n");
    int Index = 0;
    for (auto& items : gcb.items)
    {
        int len     = snprintf(0, 0, "%d", Index);
        int rem     = 8 - len;
        int pos     = rem / 2;
        int pre     = rem - pos;
        fprintf(stdout, "===%*.*s%d%*.*s===\n", pre, pre, "", Index, pos, pos, "");
        for (auto& item : items.second)
        {
            printItem(mps, (MorningItem*)&item);
        }
        fprintf(stdout, "%s", "\n");
        ++Index;
    }

    return 0;
}

#endif//MORNING_TESTING

#ifdef  MORNING_CPP_IMPL

#ifdef  MORNING_TESTING
#include <stdio.h>
#endif//MORNING_TESTING

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct MorningParseState
{
    int*                    Grammar;
    int                     NumRules;
    int                   (*RAT)[2];
    int*                    ARAT;
    int                     NullTerminal;
    int*                    NullSet;

    int                     StartRule;
    int                     Index;
    int                     LastToken;
    MORNING_PSTATE          State;
    MORNING_EVENT           Event;
    MorningItem             WorkItem;
    MorningItem            *NewItem;
    int                     Lexeme;
} MorningParseState;

int morningAddGrammar(MorningParseState* mps, int* Grammar, int NumRules)
{
    if (!mps) return 0;
    if (!Grammar) return 0;
    if (NumRules < 1) return 0;
    mps->Grammar    = Grammar;
    mps->NumRules   = NumRules;
    return 1;
}

int morningAddRandomAccessTable(MorningParseState* mps, int (*RAT)[2], int* ARAT)
{
    if (!mps) return 0;
    if (!RAT) return 0;
    mps->RAT    = RAT;
    mps->ARAT   = ARAT;
    return 1;
}

int morningAddNullKernel(MorningParseState* mps, int nullTerminal, int* nullSet)
{
    if (!mps) return 0;
    if (!nullSet) return 0;
    mps->NullTerminal   = nullTerminal;
    mps->NullSet        = nullSet;
    return 1;
}

int morningSetStartRule(MorningParseState* mps, int StartRule)
{
    if (!mps) return 0;
    if (!StartRule) return 0;
    if (StartRule > mps->NumRules) return 0;
    mps->StartRule  = StartRule;
    return 1;
}

int morningSetLexeme(MorningParseState* mps, int Lexeme)
{
    if (!mps) return 0;
    mps->Lexeme = Lexeme;
    return 1;
}

int morningSetNewItem(MorningParseState* mps, MorningItem* NewItem)
{
    if (!mps) return 0;
    mps->NewItem    = NewItem;
    return 1;
}

int* morningGetGrammar(MorningParseState* mps)
{
    if (!mps) return 0;
    return mps->Grammar;
}

int morningGetIndex(MorningParseState* mps)
{
    if (!mps) return -1;
    return mps->Index;
}

MORNING_PSTATE morningGetState(MorningParseState* mps)
{
    if (!mps) return MORNING_PS_ERROR;
    return mps->State;
}

MORNING_EVENT morningGetEvent(MorningParseState* mps)
{
    if (!mps) return MORNING_EVT_ERROR;
    return mps->Event;
}

int morningGetWorkItem(MorningParseState* mps, MorningItem* WorkItem)
{
    if (!mps) return 0;
    if (!WorkItem) return 0;
    *WorkItem   = mps->WorkItem;
    return 1;
}

int morningParseStateSize()
{
    return sizeof(MorningParseState);
}

int morningInitParseState(MorningParseState* mps)
{
    if (!mps) return 0;
    for (int BB = 0; BB < sizeof(*mps); ++BB)
    {
        ((unsigned char*)mps)[BB] = 0;
    }
    mps->State      = MORNING_PS_INIT;
    return 1;
}

int morningIsTerminal(MorningParseState* mps, int NTN)
{
    if (!mps) return 0;
    return NTN >= mps->NumRules;
}

int morningIsNonterminal(MorningParseState* mps, int NTN)
{
    if (!mps) return 0;
    return NTN && (NTN < mps->NumRules);
}

int morningIsNull(MorningParseState* mps, int NTN)
{
    if (!mps) return 0;
    return NTN && (NTN == mps->NullTerminal);
}

int morningIsInNullKernel(MorningParseState* mps, int NTN)
{
    if (!morningIsNonterminal(mps, NTN)) return 0;
    if (!mps->NullSet) return 0;
    return mps->NullSet[NTN];
}

int morningSequenceLength(MorningParseState* mps, int AltStart)
{
    if (!mps) return 0;
    int AA  = AltStart;
    while (mps->Grammar[AA])
    {
        ++AA;
    }
    return AA - AltStart;
}

int morningRuleLength(MorningParseState* mps, int RuleStart)
{
    if (!mps) return 0;
    int SeqLen  = 0;
    do
    {
        int AltLen  = morningSequenceLength(mps, RuleStart + SeqLen);
        SeqLen      += AltLen + 1;
    }
    while (mps->Grammar[RuleStart + SeqLen]);
    return SeqLen;
}

int morningEndOfGrammar(MorningParseState* mps, int RuleStart)
{
    if (!mps) return 0;
    return !!!mps->Grammar[RuleStart];
}

int morningRuleBase(MorningParseState* mps, MorningItem* item)
{
    if (!mps) return 0;
    if (!item) return 0;
    if (item->Rule >= mps->NumRules) return 0;
    int RuleBase        = mps->RAT[item->Rule][0];
    int AltBase         = mps->ARAT[RuleBase + 0];
    return AltBase;
}

int morningAltBase(MorningParseState* mps, MorningItem* item)
{
    if (!mps) return 0;
    if (!item) return 0;
    if (item->Rule >= mps->NumRules) return 0;
    int RuleBase        = mps->RAT[item->Rule][0];
    int NumAlts         = mps->RAT[item->Rule][1] - RuleBase;
    if (item->Alt >= NumAlts) return 0;
    int AltBase         = mps->ARAT[RuleBase + item->Alt];
    return AltBase;
}

int morningGetNTN(MorningParseState* mps, MorningItem* item)
{
    if (!mps) return 0;
    if (!item) return 0;
    if (item->Rule >= mps->NumRules) return 0;
    int RuleBase        = mps->RAT[item->Rule][0];
    int NumAlts         = mps->RAT[item->Rule][1] - RuleBase;
    if (item->Alt >= NumAlts) return 0;
    int AltBase         = mps->ARAT[RuleBase + item->Alt];
    // XXX: We can't check for shenanigans, here, sorry.
    int NTN             = mps->Grammar[AltBase + item->Dot];
    return NTN;
}

int morningNumAlternates(MorningParseState* mps, MorningItem* item)
{
    if (!mps) return 0;
    if (!item) return 0;
    if (item->Rule >= mps->NumRules) return 0;
    return mps->RAT[item->Rule][1] - mps->RAT[item->Rule][0];
}

int morningBuildRandomAccessTable(MorningParseState* mps)
{
    if (!mps) return 0;
    if (!mps->Grammar) return 0;
    if (mps->NumRules < 1) return 0;
    for (int RR = 1, PP = 0, AR = 0; RR <= mps->NumRules; ++RR)
    {
        mps->RAT[RR][0]     = AR;
        fprintf(stdout, "RAT[%d][%d] = %d\n", RR, 0, AR);
        for (int AA = PP; mps->Grammar[AA]; ++AR)
        {
            mps->ARAT[AR]   = AA;
            fprintf(stdout, "    ARAT[%d] = %d\n", AR, AA);
            for (int SS = AA; mps->Grammar[SS]; ++SS, ++AA, ++PP)
            {
            }
            ++AA;
            ++PP;
        }
        mps->RAT[RR][1]     = AR;
        fprintf(stdout, "RAT[%d][%d] = %d\n", RR, 1, AR);
        ++PP;
    }
    return 1;
}

int morningBuildNullKernel(MorningParseState* mps)
{
    if (!mps) return 0;
    if (mps->NullTerminal == 0)
    {
        return 1;
    }
    for (int RR = 0; RR < mps->NumRules; ++RR)
    {
        mps->NullSet[RR] = 0;
    }
    int anyNewSet   = 0;
    do
    {
        anyNewSet   = 0;
        for (int RR = 0; RR < mps->NumRules; ++RR)
        {
            for (int AA = mps->RAT[RR][0]; AA < mps->RAT[RR][1]; ++AA)
            {
                int allNullTerminal = 1;
                for (int SS = mps->ARAT[AA]; mps->Grammar[SS]; ++SS)
                {
                    int seq = mps->Grammar[SS];
                    allNullTerminal &= ((seq >= mps->NumRules) && (seq == mps->NullTerminal)) || ((seq < mps->NumRules) && mps->NullSet[seq]);
                }
                if (allNullTerminal)
                {
                    anyNewSet   = !mps->NullSet[RR];
                    mps->NullSet[RR] = 1;
                }
            }
        }
    } while (anyNewSet);
    return 1;
}

int morningParentTrigger(MorningParseState* mps, MorningItem* item, int WhichRule)
{
    if (!mps) return 0;
    if (!item) return 0;
    if (WhichRule == 0) return 0;
    if (WhichRule >= mps->NumRules) return 0;

    int RuleIndex       = mps->RAT[item->Rule][0];
    int NumAlts         = mps->RAT[item->Rule][1] - RuleIndex;
    if (item->Alt >= NumAlts)
    {
        return 0;
    }
    int AltIndex        = RuleIndex + item->Alt;
    int TheRule         = mps->ARAT[AltIndex];
    int NTN             = mps->Grammar[TheRule + item->Dot];
    return NTN == WhichRule;
}

int morningParseStep(MorningParseState* mps)
{
    if (!mps) return -1;

    mps->Event                  = MORNING_EVT_NONE;
    int result                  = 0;

    switch (mps->State)
    {
    case MORNING_PS_ERROR                   :
        result                  = -1;
        break;
    case MORNING_PS_NONE                    :
        result                  = 0;
        break;
    case MORNING_PS_INIT                    :
        mps->State              = MORNING_PS_INIT_ITEMS;
        mps->Event              = MORNING_EVT_ADD_ITEM;
        mps->Index              = 0;
        mps->WorkItem.Index     = 0;
        mps->WorkItem.Rule      = mps->StartRule;
        mps->WorkItem.Alt       = 0;
        mps->WorkItem.Dot       = 0;
        mps->WorkItem.Source    = 0;
        result                  = mps->StartRule && (mps->StartRule <= mps->NumRules);
        break;
    case MORNING_PS_INIT_ITEMS              :
        {
            int NumAlts         = morningNumAlternates(mps, &mps->WorkItem);
            ++mps->WorkItem.Alt;
            if (mps->WorkItem.Alt < NumAlts)
            {
                mps->Event      = MORNING_EVT_ADD_ITEM;
            }
            else
            {
                mps->State      = MORNING_PS_ANALYZE_ITEM;
                mps->Event      = MORNING_EVT_GET_NEXT_ITEM;
            }
            result              = 1;
        }
        break;
    case MORNING_PS_LEX_NEXT                :
        mps->State                      = MORNING_PS_ANALYZE_ITEM;
        mps->Event                      = MORNING_EVT_GET_NEXT_ITEM;
        result                          = 1;
        break;
    case MORNING_PS_SCANNING                :
        //
        // SCANNER. If [A -> ... * a .., j] is in S_i and a = x_i+1,
        //          add [ A -> ... a * ..., j] to S_i+1
        //
        {
            int NTN                     = morningGetNTN(mps, &mps->WorkItem);
            if (NTN == mps->Lexeme)
            {
                mps->WorkItem.Dot       += 1;
                mps->WorkItem.Index     = mps->Index + 1;
                mps->State              = MORNING_PS_GET_NEXT_ITEM;
                mps->Event              = MORNING_EVT_ADD_ITEM;
                result                  = 1;
            }
            else
            {
                mps->State              = MORNING_PS_ANALYZE_ITEM;
                mps->Event              = MORNING_EVT_GET_NEXT_ITEM;
                result                  = 1;
            }
        }
        break;
    case MORNING_PS_COMPLETION              :
        //
        // COMPLETER. If [A -> ... *, j] is in S_i, add
        //          [B -> ... A * ..., k] to S_i for all items
        //          [B -> ... * A ..., k] in S_j
        //
        {
            mps->State                  = MORNING_PS_GET_NEXT_PARENT_ITEM;
            mps->Event                  = MORNING_EVT_GET_NEXT_PARENT_ITEM;
            result                      = 1;
        }
        break;
    case MORNING_PS_GET_NEXT_PARENT_ITEM    :
        if (mps->NewItem)
        {
            mps->State                  = MORNING_PS_ADD_PARENT_ITEM;
            mps->Event                  = MORNING_EVT_ADD_ITEM;
            mps->WorkItem               = *mps->NewItem;
            mps->WorkItem.Index         = mps->Index;
            mps->WorkItem.Dot           += 1;
            result                      = 1;
        }
        else
        {
            mps->State                  = MORNING_PS_ANALYZE_ITEM;
            mps->Event                  = MORNING_EVT_GET_NEXT_ITEM;
            result                      = 1;
        }
        break;
    case MORNING_PS_ADD_PARENT_ITEM         :
        mps->State                      = MORNING_PS_GET_NEXT_PARENT_ITEM;
        mps->Event                      = MORNING_EVT_GET_NEXT_PARENT_ITEM;
        result                          = 1;
        break;
    case MORNING_PS_PREDICTION              :
        //
        // PREDICTOR. If [A -> ... *  B ..., j] is in S_i, add
        //          [B -> * C ..., i] to S_i for all rules B -> C
        //
        {
            int NumAlts                 = morningNumAlternates(mps, &mps->WorkItem);
            ++mps->WorkItem.Alt;
            if (mps->WorkItem.Alt < NumAlts)
            {
                mps->Event              = MORNING_EVT_ADD_ITEM;
            }
            else
            {
                mps->State              = MORNING_PS_ANALYZE_ITEM;
                mps->Event              = MORNING_EVT_GET_NEXT_ITEM;
            }
            result                      = 1;
        }
        break;
    case MORNING_PS_GET_NEXT_ITEM           :
        mps->State                      = MORNING_PS_ANALYZE_ITEM;
        mps->Event                      = MORNING_EVT_GET_NEXT_ITEM;
        result                          = 1;
        break;
    case MORNING_PS_ANALYZE_ITEM            :
        if (mps->NewItem)
        {
            mps->WorkItem               = *mps->NewItem;
            int NTN                     = morningGetNTN(mps, &mps->WorkItem);
            if (NTN == 0)
            {
                mps->State              = MORNING_PS_COMPLETION;
                mps->Event              = MORNING_EVT_INIT_PARENT_LIST;
                result                  = 1;
            }
            else if (NTN <= mps->NumRules)
            {
                mps->State              = MORNING_PS_PREDICTION;
                mps->Event              = MORNING_EVT_ADD_ITEM;
                mps->WorkItem.Index     = mps->Index;
                mps->WorkItem.Rule      = NTN;
                mps->WorkItem.Alt       = 0;
                mps->WorkItem.Dot       = 0;
                mps->WorkItem.Source    = mps->Index;
                result                  = 1;
            }
            else
            {
                mps->State              = MORNING_PS_SCANNING;
                mps->Event              = MORNING_EVT_GET_LEXEME;
                result                  = 1;
            }
        }
        else
        {
            if (mps->Lexeme)
            {
                mps->Index              += 1;
                mps->State              = MORNING_PS_LEX_NEXT;
                mps->Event              = MORNING_EVT_GET_LEXEME;
                result                  = 1;
            }
            else
            {
                result                  = 0;
            }
        }
        break;
    }

    mps->NewItem                = 0;

    return result;
}

int morningParse(MorningParseState* mps, void* cb)
{
    return 1;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif//MORNING_CPP_IMPL