#include "morning.h"

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
    int L[5] = { left.Rule,  left.Alt,  left.Dot,  left.Source  };
    int R[5] = { right.Rule, right.Alt, right.Dot, right.Source };
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

void printItem(int Index, MorningRecogState* mps, MorningItem* item)
{
    fprintf(stdout, "[%8d] %6s ::=", Index, GS[item->Rule]);
    int AltStart    = morningAltBase(mps, item);
    int AltLen      = morningSequenceLength(mps, AltStart);
    int prLen       = 0;
    for (int DD = 0; DD < AltLen; ++DD)
    {
        prLen       += fprintf(stdout, " %s%-6s", (item->Dot == DD) ? "*" : " ", GS[morningGetGrammar(mps)[AltStart + DD]]);
    }
    prLen           += fprintf(stdout, "%s", (item->Dot == AltLen) ? "*" : " ");
    fprintf(stdout, "%*.*s (%d)", (28 - prLen), (28 - prLen), "", item->Source);
    //fprintf(stdout, " : %d%d%d%d%d", Index, item->Rule, item->Alt, item->Dot, item->Source);
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

    unsigned char mpStore[morningRecogStateSize()];
    MorningRecogState* mps  = (MorningRecogState*)&mpStore[0];
    morningInitRecogState(mps);

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

    MorningItem item    = { 1, 0, 0, 0 };
    printItem(0, mps, &item);
    fprintf(stdout, "What? %s\n", GS[morningGetNTN(mps, &item)]);
    item.Alt            = 1;
    printItem(0, mps, &item);
    fprintf(stdout, "What? %s\n", GS[morningGetNTN(mps, &item)]);
    item.Rule           = 1;
    item.Alt            = 0;
    printItem(0, mps, &item);
    fprintf(stdout, "What? %s\n", GS[morningGetNTN(mps, &item)]);
    item.Alt            = 1;
    printItem(0, mps, &item);
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
        int result  = morningRecognizerStep(mps);
        int Index   = morningGetIndex(mps);
        if (result != 1)
        {
            break;
        }
        fprintf(stdout, "[%4d] STATE: %s x %s\n", morningGetIndex(mps), STATES[morningGetState(mps)], EVENTS[morningGetEvent(mps)]);
        MorningItem *WorkItem                   = { };
        morningGetWorkItem(mps, &WorkItem);
        switch (morningGetEvent(mps))
        {
        case MORNING_EVT_ERROR                  :
            break;
        case MORNING_EVT_GET_LEXEME             :
            fprintf(stdout, "       --> %s.\n", "GET LEXEME");
            morningSetLexeme(mps, lexemes[morningGetIndex(mps)]);
            break;
        case MORNING_EVT_ADD_ITEM_NEXT          : Index += 1;
        case MORNING_EVT_ADD_ITEM               :
            {
                auto iitr       = gcb.items[Index].find(*WorkItem);
                if (iitr != gcb.items[Index].end())
                {
                    break;
                }
                fprintf(stdout, "       --> ITEM ADDED to idx %d.\n", Index);
                fprintf(stdout, "%s", "           ");
                printItem(Index, mps, WorkItem);
                gcb.items[Index].insert(*WorkItem);
                gcb.unused[Index].insert(*WorkItem);
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
                printItem(Index, mps, &NewItem);
            }
            break;
        case MORNING_EVT_INIT_PARENT_LIST       :
            gcb.parents.clear();
            fprintf(stdout, "       --> %s.\n", "INIT'D PARENT LIST");
            {
                for (auto& item : gcb.items[WorkItem->Source])
                {
                    fprintf(stdout, "%s", "       FILTERING:\n       ");
                    printItem(Index, mps, (MorningItem*)&item);
                    if (morningParentTrigger(mps, (MorningItem*)&item, WorkItem->Rule))
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
            printItem(Index, mps, (MorningItem*)&item);
        }
        fprintf(stdout, "%s", "\n");
        ++Index;
    }

    return 0;
}

#define MORNING_CPP_IMPL
#include "morning.h"