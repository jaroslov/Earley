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

typedef struct MorningParseState MorningParseState;

typedef struct MorningItem
{
    int                     Index;
    int                     Rule;
    int                     Alt;
    int                     Dot;
    int                     Source;
} MorningItem;

typedef int (*MorningAction)    (void*, MorningParseState*);

typedef struct MorningActions
{
    void           *Handle;
    MorningAction   GetLexeme;
    MorningAction   AddItem;
    MorningAction   GetNextItem;
    MorningAction   InitParentList;
    MorningAction   GetNextParentItem;
} MorningActions;

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
int morningGetWorkItem(MorningParseState*, MorningItem** WorkItem);
int morningParentTrigger(MorningParseState* mps, MorningItem* item, int WhichRule);

int morningBuildRandomAccessTable(MorningParseState*);
int morningBuildNullKernel(MorningParseState*);

int morningRecognizerStep(MorningParseState* mps);
int morningRecognizerStepAct(MorningParseState* mps, MorningActions* mact);
int morningRecognize(MorningParseState*, MorningActions* mact);

#ifdef __cplusplus
} // extern "C"
#endif

#endif//MORNING_H

#ifdef  MORNING_CPP_IMPL

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

int morningGetWorkItem(MorningParseState* mps, MorningItem** WorkItem)
{
    if (!mps) return 0;
    if (!WorkItem) return 0;
    *WorkItem   = &mps->WorkItem;
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
        //fprintf(stdout, "RAT[%d][%d] = %d\n", RR, 0, AR);
        for (int AA = PP; mps->Grammar[AA]; ++AR)
        {
            mps->ARAT[AR]   = AA;
            //fprintf(stdout, "    ARAT[%d] = %d\n", AR, AA);
            for (int SS = AA; mps->Grammar[SS]; ++SS, ++AA, ++PP)
            {
            }
            ++AA;
            ++PP;
        }
        mps->RAT[RR][1]     = AR;
        //fprintf(stdout, "RAT[%d][%d] = %d\n", RR, 1, AR);
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

int morningRecognizerStep(MorningParseState* mps)
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
    case MORNING_PS_PREDICT_NULLABLE        :
        // If B is nullable, we also need to save off:
        //     [A -> ... B * ..., j]
        // Then we go on to the regular prediction.
        {
            mps->WorkItem.Dot           = -1;
            int NTN                     = morningGetNTN(mps, &mps->WorkItem);
            mps->State                  = MORNING_PS_PREDICTION;
            mps->Event                  = MORNING_EVT_ADD_ITEM;
            mps->WorkItem.Index         = mps->Index;
            mps->WorkItem.Rule          = NTN;
            mps->WorkItem.Alt           = 0;
            mps->WorkItem.Dot           = 0;
            mps->WorkItem.Source        = mps->Index;
            result                      = 1;
        }
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
                MorningItem Next        = mps->WorkItem;
                Next.Dot                += 1;
                int nextNTN             = morningGetNTN(mps, &Next);
                if ((nextNTN < mps->NumRules) && mps->NullSet[nextNTN])
                {
                    mps->State              = MORNING_PS_PREDICT_NULLABLE;
                    mps->Event              = MORNING_EVT_ADD_ITEM;
                    mps->WorkItem           = Next;
                    result                  = 1;
                }
                else
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

int morningRecognizerStepAct(MorningParseState* mps, MorningActions* mact)
{
    if (!morningRecognizerStep(mps))
    {
        return 0;
    }
    switch (mps->Event)
    {
    case MORNING_EVT_ERROR                  : return -1;
    case MORNING_EVT_GET_LEXEME             : return mact->GetLexeme(mact->Handle, mps);
    case MORNING_EVT_ADD_ITEM               : return mact->AddItem(mact->Handle, mps);
    case MORNING_EVT_GET_NEXT_ITEM          : return mact->GetNextItem(mact->Handle, mps);
    case MORNING_EVT_INIT_PARENT_LIST       : return mact->InitParentList(mact->Handle, mps);
    case MORNING_EVT_GET_NEXT_PARENT_ITEM   : return mact->GetNextParentItem(mact->Handle, mps);
    case MORNING_EVT_NONE                   : return 0;
    }
    return -1;
}

int morningRecognize(MorningParseState* mps, MorningActions* mact)
{
    if (!mps) return -1;
    if (!mact) return -1;
    do
    {
        int result  = morningRecognizerStepAct(mps, mact);
        if (result < 0) return -1;
        if (result == 0) break;
    }
    while (1);
    return 1;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif//MORNING_CPP_IMPL