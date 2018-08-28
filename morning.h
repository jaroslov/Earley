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
    X(PREDICT_NULLABLE)         \
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
    X(ADD_ITEM_NEXT)            \
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

#define MORNING_ACTION_TABLE(X) \
    X(ERROR)                    \
    X(PARSE)                    \
    X(NONE)

typedef enum MORNING_ACTION
{
#undef MORNING_ENTRY
#define MORNING_ENTRY(E) MORNING_ACT_ ## E,
MORNING_ACTION_TABLE(MORNING_ENTRY)
#undef MORNING_ENTRY
} MORNING_ACTION;

typedef struct MorningRecogState MorningRecogState;
typedef struct MorningParseState MorningParseState;

typedef struct MorningItem
{
    int                     Rule;
    int                     Alt;
    int                     Dot;
    int                     Source;
} MorningItem;

typedef int (*MorningRecogAction)    (void*, MorningRecogState*);

typedef struct MorningRecogActions
{
    void                   *Handle;
    MorningRecogAction      GetLexeme;
    MorningRecogAction      AddItem;
    MorningRecogAction      AddItemNext;
    MorningRecogAction      GetNextItem;
    MorningRecogAction      InitParentList;
    MorningRecogAction      GetNextParentItem;
} MorningRecogActions;

typedef struct MorningParseActions
{
    void                   *Handle;
} MorningParseActions;

int morningRecogStateSize();
int morningInitRecogState(MorningRecogState*);

int morningIsTerminal(MorningRecogState*, int NTN);
int morningIsNonterminal(MorningRecogState*, int NTN);
int morningIsNull(MorningRecogState*, int NTN);
int morningIsInNullKernel(MorningRecogState*, int NTN);
int morningSequenceLength(MorningRecogState*, int AltStart);
int morningRuleLength(MorningRecogState*, int RuleStart);
int morningEndOfGrammar(MorningRecogState*, int RuleStart);
int morningRuleBase(MorningRecogState*, MorningItem*);
int morningAltBase(MorningRecogState*, MorningItem*);
int morningGetNTN(MorningRecogState*, MorningItem*);
int morningNumAlternates(MorningRecogState*, MorningItem*);

int morningAddGrammar(MorningRecogState*, int* Grammar, int NumRules);
int morningAddRandomAccessTable(MorningRecogState*, int (*RAT)[2], int* ARAT);
int morningAddNullKernel(MorningRecogState*, int nullTerminal, int* nullSet);
int morningSetStartRule(MorningRecogState*, int StartRule);
int morningSetLexeme(MorningRecogState*, int Lexeme);
int morningSetNewItem(MorningRecogState*, MorningItem* NewItem);

int* morningGetGrammar(MorningRecogState*);
int morningGetIndex(MorningRecogState*);
MORNING_PSTATE morningGetState(MorningRecogState*);
MORNING_EVENT morningGetEvent(MorningRecogState*);
int morningGetWorkItem(MorningRecogState*, MorningItem** WorkItem);
int morningParentTrigger(MorningRecogState* mrs, MorningItem* item, int WhichRule);

int morningBuildRandomAccessTable(MorningRecogState*);
int morningBuildNullKernel(MorningRecogState*);

int morningRecognizerStep(MorningRecogState* mrs);
int morningRecognizerStepAct(MorningRecogState* mrs, MorningRecogActions* mact);
int morningRecognize(MorningRecogState*, MorningRecogActions* mact);

int morningParseStateSize();
int morningInitParseState(MorningRecogState*, MorningParseState*);

int morningParserStep(MorningParseState* mrs);
int morningParserStepAct(MorningParseState* mrs, MorningParseActions* mact);
int morningParse(MorningParseState*, MorningParseActions* mact);

#ifdef __cplusplus
} // extern "C"
#endif

#endif//MORNING_H

#ifdef  MORNING_CPP_IMPL

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct MorningRecogState
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
} MorningRecogState;

int morningAddGrammar(MorningRecogState* mrs, int* Grammar, int NumRules)
{
    if (!mrs) return 0;
    if (!Grammar) return 0;
    if (NumRules < 1) return 0;
    mrs->Grammar    = Grammar;
    mrs->NumRules   = NumRules;
    return 1;
}

int morningAddRandomAccessTable(MorningRecogState* mrs, int (*RAT)[2], int* ARAT)
{
    if (!mrs) return 0;
    if (!RAT) return 0;
    mrs->RAT    = RAT;
    mrs->ARAT   = ARAT;
    return 1;
}

int morningAddNullKernel(MorningRecogState* mrs, int nullTerminal, int* nullSet)
{
    if (!mrs) return 0;
    if (!nullSet) return 0;
    mrs->NullTerminal   = nullTerminal;
    mrs->NullSet        = nullSet;
    return 1;
}

int morningSetStartRule(MorningRecogState* mrs, int StartRule)
{
    if (!mrs) return 0;
    if (!StartRule) return 0;
    if (StartRule > mrs->NumRules) return 0;
    mrs->StartRule  = StartRule;
    return 1;
}

int morningSetLexeme(MorningRecogState* mrs, int Lexeme)
{
    if (!mrs) return 0;
    mrs->Lexeme = Lexeme;
    return 1;
}

int morningSetNewItem(MorningRecogState* mrs, MorningItem* NewItem)
{
    if (!mrs) return 0;
    mrs->NewItem    = NewItem;
    return 1;
}

int* morningGetGrammar(MorningRecogState* mrs)
{
    if (!mrs) return 0;
    return mrs->Grammar;
}

int morningGetIndex(MorningRecogState* mrs)
{
    if (!mrs) return -1;
    return mrs->Index;
}

MORNING_PSTATE morningGetState(MorningRecogState* mrs)
{
    if (!mrs) return MORNING_PS_ERROR;
    return mrs->State;
}

MORNING_EVENT morningGetEvent(MorningRecogState* mrs)
{
    if (!mrs) return MORNING_EVT_ERROR;
    return mrs->Event;
}

int morningGetWorkItem(MorningRecogState* mrs, MorningItem** WorkItem)
{
    if (!mrs) return 0;
    if (!WorkItem) return 0;
    *WorkItem   = &mrs->WorkItem;
    return 1;
}

int morningRecogStateSize()
{
    return sizeof(MorningRecogState);
}

int morningInitRecogState(MorningRecogState* mrs)
{
    if (!mrs) return 0;
    for (int BB = 0; BB < sizeof(*mrs); ++BB)
    {
        ((unsigned char*)mrs)[BB] = 0;
    }
    mrs->State      = MORNING_PS_INIT;
    return 1;
}

int morningIsTerminal(MorningRecogState* mrs, int NTN)
{
    if (!mrs) return 0;
    return NTN >= mrs->NumRules;
}

int morningIsNonterminal(MorningRecogState* mrs, int NTN)
{
    if (!mrs) return 0;
    return NTN && (NTN < mrs->NumRules);
}

int morningIsNull(MorningRecogState* mrs, int NTN)
{
    if (!mrs) return 0;
    return NTN && (NTN == mrs->NullTerminal);
}

int morningIsInNullKernel(MorningRecogState* mrs, int NTN)
{
    if (!morningIsNonterminal(mrs, NTN)) return 0;
    if (!mrs->NullSet) return 0;
    return mrs->NullSet[NTN];
}

int morningSequenceLength(MorningRecogState* mrs, int AltStart)
{
    if (!mrs) return 0;
    int AA  = AltStart;
    while (mrs->Grammar[AA])
    {
        ++AA;
    }
    return AA - AltStart;
}

int morningRuleLength(MorningRecogState* mrs, int RuleStart)
{
    if (!mrs) return 0;
    int SeqLen  = 0;
    do
    {
        int AltLen  = morningSequenceLength(mrs, RuleStart + SeqLen);
        SeqLen      += AltLen + 1;
    }
    while (mrs->Grammar[RuleStart + SeqLen]);
    return SeqLen;
}

int morningEndOfGrammar(MorningRecogState* mrs, int RuleStart)
{
    if (!mrs) return 0;
    return !!!mrs->Grammar[RuleStart];
}

int morningRuleBase(MorningRecogState* mrs, MorningItem* item)
{
    if (!mrs) return 0;
    if (!item) return 0;
    if (item->Rule >= mrs->NumRules) return 0;
    int RuleBase        = mrs->RAT[item->Rule][0];
    int AltBase         = mrs->ARAT[RuleBase + 0];
    return AltBase;
}

int morningAltBase(MorningRecogState* mrs, MorningItem* item)
{
    if (!mrs) return 0;
    if (!item) return 0;
    if (item->Rule >= mrs->NumRules) return 0;
    int RuleBase        = mrs->RAT[item->Rule][0];
    int NumAlts         = mrs->RAT[item->Rule][1] - RuleBase;
    if (item->Alt >= NumAlts) return 0;
    int AltBase         = mrs->ARAT[RuleBase + item->Alt];
    return AltBase;
}

int morningGetNTN(MorningRecogState* mrs, MorningItem* item)
{
    if (!mrs) return 0;
    if (!item) return 0;
    if (item->Rule >= mrs->NumRules) return 0;
    int RuleBase        = mrs->RAT[item->Rule][0];
    int NumAlts         = mrs->RAT[item->Rule][1] - RuleBase;
    if (item->Alt >= NumAlts) return 0;
    int AltBase         = mrs->ARAT[RuleBase + item->Alt];
    // XXX: We can't check for shenanigans, here, sorry.
    int NTN             = mrs->Grammar[AltBase + item->Dot];
    return NTN;
}

int morningNumAlternates(MorningRecogState* mrs, MorningItem* item)
{
    if (!mrs) return 0;
    if (!item) return 0;
    if (item->Rule >= mrs->NumRules) return 0;
    return mrs->RAT[item->Rule][1] - mrs->RAT[item->Rule][0];
}

int morningBuildRandomAccessTable(MorningRecogState* mrs)
{
    if (!mrs) return 0;
    if (!mrs->Grammar) return 0;
    if (mrs->NumRules < 1) return 0;
    for (int RR = 1, PP = 0, AR = 0; RR <= mrs->NumRules; ++RR)
    {
        mrs->RAT[RR][0]     = AR;
        //fprintf(stdout, "RAT[%d][%d] = %d\n", RR, 0, AR);
        for (int AA = PP; mrs->Grammar[AA]; ++AR)
        {
            mrs->ARAT[AR]   = AA;
            //fprintf(stdout, "    ARAT[%d] = %d\n", AR, AA);
            for (int SS = AA; mrs->Grammar[SS]; ++SS, ++AA, ++PP)
            {
            }
            ++AA;
            ++PP;
        }
        mrs->RAT[RR][1]     = AR;
        //fprintf(stdout, "RAT[%d][%d] = %d\n", RR, 1, AR);
        ++PP;
    }
    return 1;
}

int morningBuildNullKernel(MorningRecogState* mrs)
{
    if (!mrs) return 0;
    if (mrs->NullTerminal == 0)
    {
        return 1;
    }
    for (int RR = 0; RR < mrs->NumRules; ++RR)
    {
        mrs->NullSet[RR] = 0;
    }
    int anyNewSet   = 0;
    do
    {
        anyNewSet   = 0;
        for (int RR = 0; RR < mrs->NumRules; ++RR)
        {
            for (int AA = mrs->RAT[RR][0]; AA < mrs->RAT[RR][1]; ++AA)
            {
                int allNullTerminal = 1;
                for (int SS = mrs->ARAT[AA]; mrs->Grammar[SS]; ++SS)
                {
                    int seq = mrs->Grammar[SS];
                    allNullTerminal &= ((seq >= mrs->NumRules) && (seq == mrs->NullTerminal)) || ((seq < mrs->NumRules) && mrs->NullSet[seq]);
                }
                if (allNullTerminal)
                {
                    anyNewSet   = !mrs->NullSet[RR];
                    mrs->NullSet[RR] = 1;
                }
            }
        }
    } while (anyNewSet);
    return 1;
}

int morningParentTrigger(MorningRecogState* mrs, MorningItem* item, int WhichRule)
{
    if (!mrs) return 0;
    if (!item) return 0;
    if (WhichRule == 0) return 0;
    if (WhichRule >= mrs->NumRules) return 0;

    int RuleIndex       = mrs->RAT[item->Rule][0];
    int NumAlts         = mrs->RAT[item->Rule][1] - RuleIndex;
    if (item->Alt >= NumAlts)
    {
        return 0;
    }
    int AltIndex        = RuleIndex + item->Alt;
    int TheRule         = mrs->ARAT[AltIndex];
    int NTN             = mrs->Grammar[TheRule + item->Dot];
    return NTN == WhichRule;
}

int morningRecognizerStep(MorningRecogState* mrs)
{
    if (!mrs) return -1;

    mrs->Event                  = MORNING_EVT_NONE;
    int result                  = 0;

    switch (mrs->State)
    {
    case MORNING_PS_ERROR                   :
        result                  = -1;
        break;
    case MORNING_PS_NONE                    :
        result                  = 0;
        break;
    case MORNING_PS_INIT                    :
        mrs->State              = MORNING_PS_INIT_ITEMS;
        mrs->Event              = MORNING_EVT_ADD_ITEM;
        mrs->Index              = 0;
        mrs->WorkItem.Rule      = mrs->StartRule;
        mrs->WorkItem.Alt       = 0;
        mrs->WorkItem.Dot       = 0;
        mrs->WorkItem.Source    = 0;
        result                  = mrs->StartRule && (mrs->StartRule <= mrs->NumRules);
        break;
    case MORNING_PS_INIT_ITEMS              :
        {
            int NumAlts         = morningNumAlternates(mrs, &mrs->WorkItem);
            ++mrs->WorkItem.Alt;
            if (mrs->WorkItem.Alt < NumAlts)
            {
                mrs->Event      = MORNING_EVT_ADD_ITEM;
            }
            else
            {
                mrs->State      = MORNING_PS_ANALYZE_ITEM;
                mrs->Event      = MORNING_EVT_GET_NEXT_ITEM;
            }
            result              = 1;
        }
        break;
    case MORNING_PS_LEX_NEXT                :
        mrs->State                      = MORNING_PS_ANALYZE_ITEM;
        mrs->Event                      = MORNING_EVT_GET_NEXT_ITEM;
        result                          = 1;
        break;
    case MORNING_PS_SCANNING                :
        //
        // SCANNER. If [A -> ... * a .., j] is in S_i and a = x_i+1,
        //          add [ A -> ... a * ..., j] to S_i+1
        //
        {
            int NTN                     = morningGetNTN(mrs, &mrs->WorkItem);
            if (NTN == mrs->Lexeme)
            {
                mrs->WorkItem.Dot       += 1;
                mrs->State              = MORNING_PS_GET_NEXT_ITEM;
                mrs->Event              = MORNING_EVT_ADD_ITEM_NEXT;
                result                  = 1;
            }
            else
            {
                mrs->State              = MORNING_PS_ANALYZE_ITEM;
                mrs->Event              = MORNING_EVT_GET_NEXT_ITEM;
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
            mrs->State                  = MORNING_PS_GET_NEXT_PARENT_ITEM;
            mrs->Event                  = MORNING_EVT_GET_NEXT_PARENT_ITEM;
            result                      = 1;
        }
        break;
    case MORNING_PS_GET_NEXT_PARENT_ITEM    :
        if (mrs->NewItem)
        {
            mrs->State                  = MORNING_PS_ADD_PARENT_ITEM;
            mrs->Event                  = MORNING_EVT_ADD_ITEM;
            mrs->WorkItem               = *mrs->NewItem;
            mrs->WorkItem.Dot           += 1;
            result                      = 1;
        }
        else
        {
            mrs->State                  = MORNING_PS_ANALYZE_ITEM;
            mrs->Event                  = MORNING_EVT_GET_NEXT_ITEM;
            result                      = 1;
        }
        break;
    case MORNING_PS_ADD_PARENT_ITEM         :
        mrs->State                      = MORNING_PS_GET_NEXT_PARENT_ITEM;
        mrs->Event                      = MORNING_EVT_GET_NEXT_PARENT_ITEM;
        result                          = 1;
        break;
    case MORNING_PS_PREDICTION              :
        //
        // PREDICTOR. If [A -> ... *  B ..., j] is in S_i, add
        //          [B -> * C ..., i] to S_i for all rules B -> C
        //
        {
            int NumAlts                 = morningNumAlternates(mrs, &mrs->WorkItem);
            ++mrs->WorkItem.Alt;
            if (mrs->WorkItem.Alt < NumAlts)
            {
                mrs->Event              = MORNING_EVT_ADD_ITEM;
            }
            else
            {
                mrs->State              = MORNING_PS_ANALYZE_ITEM;
                mrs->Event              = MORNING_EVT_GET_NEXT_ITEM;
            }
            result                      = 1;
        }
        break;
    case MORNING_PS_GET_NEXT_ITEM           :
        mrs->State                      = MORNING_PS_ANALYZE_ITEM;
        mrs->Event                      = MORNING_EVT_GET_NEXT_ITEM;
        result                          = 1;
        break;
    case MORNING_PS_PREDICT_NULLABLE        :
        // If B is nullable, we also need to save off:
        //     [A -> ... B * ..., j]
        // Then we go on to the regular prediction.
        {
            mrs->WorkItem.Dot           = -1;
            int NTN                     = morningGetNTN(mrs, &mrs->WorkItem);
            mrs->State                  = MORNING_PS_PREDICTION;
            mrs->Event                  = MORNING_EVT_ADD_ITEM;
            mrs->WorkItem.Rule          = NTN;
            mrs->WorkItem.Alt           = 0;
            mrs->WorkItem.Dot           = 0;
            mrs->WorkItem.Source        = mrs->Index;
            result                      = 1;
        }
        break;
    case MORNING_PS_ANALYZE_ITEM            :
        if (mrs->NewItem)
        {
            mrs->WorkItem               = *mrs->NewItem;
            int NTN                     = morningGetNTN(mrs, &mrs->WorkItem);
            if (NTN == 0)
            {
                mrs->State              = MORNING_PS_COMPLETION;
                mrs->Event              = MORNING_EVT_INIT_PARENT_LIST;
                result                  = 1;
            }
            else if (NTN <= mrs->NumRules)
            {
                MorningItem Next        = mrs->WorkItem;
                Next.Dot                += 1;
                int nextNTN             = morningGetNTN(mrs, &Next);
                if ((nextNTN < mrs->NumRules) && mrs->NullSet[nextNTN])
                {
                    mrs->State              = MORNING_PS_PREDICT_NULLABLE;
                    mrs->Event              = MORNING_EVT_ADD_ITEM;
                    mrs->WorkItem           = Next;
                    result                  = 1;
                }
                else
                {
                    mrs->State              = MORNING_PS_PREDICTION;
                    mrs->Event              = MORNING_EVT_ADD_ITEM;
                    mrs->WorkItem.Rule      = NTN;
                    mrs->WorkItem.Alt       = 0;
                    mrs->WorkItem.Dot       = 0;
                    mrs->WorkItem.Source    = mrs->Index;
                    result                  = 1;
                }
            }
            else
            {
                mrs->State              = MORNING_PS_SCANNING;
                mrs->Event              = MORNING_EVT_GET_LEXEME;
                result                  = 1;
            }
        }
        else
        {
            if (mrs->Lexeme)
            {
                mrs->Index              += 1;
                mrs->State              = MORNING_PS_LEX_NEXT;
                mrs->Event              = MORNING_EVT_GET_LEXEME;
                result                  = 1;
            }
            else
            {
                result                  = 0;
            }
        }
        break;
    }

    mrs->NewItem                = 0;

    return result;
}

int morningRecognizerStepAct(MorningRecogState* mrs, MorningRecogActions* mact)
{
    if (!morningRecognizerStep(mrs))
    {
        return 0;
    }
    switch (mrs->Event)
    {
    case MORNING_EVT_ERROR                  : return -1;
    case MORNING_EVT_GET_LEXEME             : return mact->GetLexeme(mact->Handle, mrs);
    case MORNING_EVT_ADD_ITEM               : return mact->AddItem(mact->Handle, mrs);
    case MORNING_EVT_ADD_ITEM_NEXT          : return mact->AddItemNext(mact->Handle, mrs);
    case MORNING_EVT_GET_NEXT_ITEM          : return mact->GetNextItem(mact->Handle, mrs);
    case MORNING_EVT_INIT_PARENT_LIST       : return mact->InitParentList(mact->Handle, mrs);
    case MORNING_EVT_GET_NEXT_PARENT_ITEM   : return mact->GetNextParentItem(mact->Handle, mrs);
    case MORNING_EVT_NONE                   : return 0;
    }
    return -1;
}

int morningRecognize(MorningRecogState* mrs, MorningRecogActions* mact)
{
    if (!mrs) return -1;
    if (!mact) return -1;
    do
    {
        int result  = morningRecognizerStepAct(mrs, mact);
        if (result < 0) return -1;
        if (result == 0) break;
    }
    while (1);
    return 1;
}

typedef struct MorningParseState
{
    int*                    Grammar;
    int                     NumRules;
    int                   (*RAT)[2];
    int*                    ARAT;
    int                     NullTerminal;
    int*                    NullSet;
    int                     StartRule;
} MorningParseState;

int morningParseStateSize()
{
    return sizeof(MorningParseState);
}

int morningInitParseState(MorningRecogState* mrs, MorningParseState* mps)
{
    if (!mrs) return 0;
    if (!mps) return 0;

    mps->Grammar        = mrs->Grammar;
    mps->NumRules       = mrs->NumRules;
    mps->RAT            = mrs->RAT;
    mps->ARAT           = mrs->ARAT;
    mps->NullTerminal   = mrs->NullTerminal;
    mps->NullSet        = mrs->NullSet;
    mps->StartRule      = mrs->StartRule;

    return 1;
}

int morningParserStep(MorningParseState* mps)
{
    /*

    Depth first search:

        (1) Initialize with all the Start rules with the Dot at the complete state,
            with the Source of (0) (full parse).
        (2) Pop an item off the stack.
            (1) Ask if this item is in the current index;
            (2) If it is, ask for all the items who's LHS is the rule that is completed
                at the dot; add those items to the list
        (3) Keep going until there's no more items.

    */
    return 0;
}

int morningParserStepAct(MorningParseState* mps, MorningParseActions* mact)
{
    return 0;
}

int morningParse(MorningParseState*, MorningParseActions* mact)
{
    return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif//MORNING_CPP_IMPL