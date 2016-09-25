#include <stdio.h>
#include <set>
#include <string.h>
#include <vector>

typedef std::vector<bool>       nullableset;
typedef std::vector<int>        sequence;
typedef std::vector<sequence>   alternate;
typedef std::vector<alternate>  grammar;

typedef std::vector<int>        input;

void BuildNullableSet(const char** names, grammar& G, nullableset& nullset);
bool Parse(const char**, grammar&, nullableset& nullset, input const& expr);
void PrintAlt(const char** names, grammar& G, int rule, int alt, int dot=-1, int ref=-1);
void PrintRule(const char** names, grammar& G, int rule);
void PrintGrammar(const char** names, grammar& G);
void PrintNullSet(const char** names, nullableset& nullset);

int main(int argc, char *argv[])
{
    if (!strcmp(argv[1], "math"))
    {
        enum
        {
            ENTRY,
            SUM,
            PRODUCT,
            FACTOR,
            NUMBER,
            DIGIT,
            PLUSMINUS,
            TIMESDIVIDE,
            LPAREN,
            RPAREN,
        };

        const char* MATH_NAMES[] =
        {
            "ENTRY",
            "SUM",
            "PRODUCT",
            "FACTOR",
            "NUMBER",
            "DIGIT",
            "PLUS-MINUS",
            "TIMES-DIVIDE",
            "(",
            ")",
        };

        grammar G  = {
            {
                { SUM },
            },
            {
                { SUM, PLUSMINUS, PRODUCT },
                { PRODUCT },
            },
            {
                { PRODUCT, TIMESDIVIDE, FACTOR },
                { FACTOR },
            },
            {
                { LPAREN, SUM, RPAREN },
                { NUMBER },
            },
            {
                { DIGIT, NUMBER },
                { DIGIT },
            },
        };

        PrintGrammar(MATH_NAMES, G);
        nullableset nullset;
        BuildNullableSet(MATH_NAMES, G, nullset);
        PrintNullSet(MATH_NAMES, nullset);

        input expr = { DIGIT, PLUSMINUS, LPAREN, DIGIT, TIMESDIVIDE, DIGIT, PLUSMINUS, DIGIT, RPAREN };

        fprintf(stdout, "Parse %s.\n",
            Parse(MATH_NAMES, G, nullset, expr)
            ? "succeeded"
            : "failed");
    }
    else if (!strcmp(argv[1], "nullable"))
    {
        enum
        {
            ENTRY,
            A,
            B,
        };

        const char* NULL_NAMES[] =
        {
            "ENTRY",
            "A",
            "B",
        };

        grammar G   = {
            {
                { A },
            },
            {
                { },
                { B },
            },
            {
                { A },
            },
        };

        PrintGrammar(NULL_NAMES, G);
        nullableset nullset;
        BuildNullableSet(NULL_NAMES, G, nullset);
        PrintNullSet(NULL_NAMES, nullset);

        input expr = { };

        fprintf(stdout, "Parse %s.\n",
            Parse(NULL_NAMES, G, nullset, expr)
            ? "succeeded"
            : "failed");
    }

    return 0;
}

void PrintAlt(const char** names, grammar& G, int rule, int alt, int dot, int ref)
{
    fprintf(stdout, "%12s ::=", names[rule]);
    sequence const& s   = G[rule][alt];
    int length  = 0;
    for (int TT = 0, TE = s.size(); TT < TE; ++TT)
    {
        if ((dot >= 0) && (dot == TT))
        {
            length += fprintf(stdout, " %s", "*");
        }
        length += fprintf(stdout, " %s", names[s[TT]]);
    }
    if ((dot >= 0) && (dot >= s.size()))
    {
        length += fprintf(stdout, " %s", "*");
    }
    for (int LL = length; length < 35; ++length)
    {
        fprintf(stdout, "%s", " ");
    }
    if (ref >= 0)
    {
        fprintf(stdout, "(%d)", ref);
    }
    fprintf(stdout, "%s", "\n");
}

void PrintRule(const char** names, grammar& G, int rule)
{
    for (int AA = 0, AE = G[rule].size(); AA < AE; ++AA)
    {
        PrintAlt(names, G, rule, AA);
    }
}

void PrintGrammar(const char** names, grammar& G)
{
    for (int RR = 0, RE = G.size(); RR < RE; ++RR)
    {
        PrintRule(names, G, RR);
    }
    fprintf(stdout, "%s", "\n");
}

void PrintNullSet(const char** names, nullableset& nullset)
{
    fprintf(stdout, "%s", "Null Set\n");
    for (int RR = 0, RE = nullset.size(); RR < RE; ++RR)
    {
        if (nullset[RR])
        {
            fprintf(stdout, "    %s is null.\n", names[RR]);
        }
    }
    fprintf(stdout, "%s", "\n");
}

typedef std::tuple<int, int, int, int>  item_t;
typedef std::set<item_t>                itemset_t;
typedef std::vector<itemset_t>          itemvector_t;

enum
{
    RULE,
    ALT,
    DOT,
    REF,
};

void BuildNullableSet(const char** names, grammar& G, nullableset& nullset)
{
    typedef std::set<int>               rule2rule_t;
    typedef std::vector<rule2rule_t>    depgraph_t;
    std::set<int> knownnull;
    depgraph_t iG;
    nullset.resize(G.size());
    iG.resize(G.size());

    for (int RR = 0, RE = G.size(); RR < RE; ++RR)
    {
        for (int AA = 0, AE = G[RR].size(); AA < AE; ++AA)
        {
            if (G[RR][AA].empty())
            {
                knownnull.insert(knownnull.end(), RR);
            }
            for (int TT = 0, TE = G[RR][AA].size(); TT < TE; ++TT)
            {
                iG[RR].insert(iG[RR].end(), G[RR][AA][TT]);
            }
        }
    }
    bool addedNull  = false;
    do
    {
        addedNull   = false;
        std::set<int> newnull;
        for (auto null : knownnull)
        {
            std::set<int> const& irhs   = iG[null];
            for (auto irhterm : irhs)
            {
                bool isNullable = true;
                for (int AA = 0, AE = G[irhterm].size(); AA < AE; ++AA)
                {
                    for (int TT = 0, TE = G[irhterm][AA].size(); TT < TE; ++TT)
                    {
                        int term    = G[irhterm][AA][TT];
                        bool isNull = knownnull.find(term) != knownnull.end();
                        if ((term >= G.size()) || isNull)
                        {
                            isNullable = false;
                            break;
                        }
                    }
                }
                auto nntr = knownnull.find(irhterm);
                if (nntr == knownnull.end())
                {
                    newnull.insert(newnull.end(), irhterm);
                    addedNull = true;
                }
            }
        }
        knownnull.insert(newnull.begin(), newnull.end());
    } while (addedNull);

    for (auto null : knownnull)
    {
        nullset[null]   = true;
    }
}

bool Parse(const char** names, grammar& G, nullableset& nullset, input const& expr)
{
    itemvector_t    states;
    states.resize(expr.size() + 1);

    states[0].insert(std::make_tuple(0, 0, 0, 0));

    for (int KK = 0, KE = expr.size()+1; KK < KE; ++KK)
    {
        bool addedTerm = false;
        itemset_t consider  = states[KK];
        do
        {
            addedTerm   = false;
            itemset_t nss;
            for (auto const& item : consider)
            {
                auto rule   = std::get<RULE>(item);
                auto alt    = std::get<ALT>(item);
                auto dot    = std::get<DOT>(item);
                auto ref    = std::get<REF>(item);
                if (dot >= G[rule][alt].size())
                {
                    // Cmopletion.
                    for (auto const& preitem : states[ref])
                    {
                        int crule   = std::get<RULE>(preitem);
                        int calt    = std::get<ALT>(preitem);
                        int cdot    = std::get<DOT>(preitem);
                        int cref    = std::get<REF>(preitem);
                        if ((cdot < G[crule][calt].size()) && (G[crule][calt][cdot] == rule))
                        {
                            auto newitem    = std::make_tuple(crule, calt, cdot+1, cref);
                            auto nitr       = nss.find(newitem);
                            if (nitr == nss.end())
                            {
                                nss.insert(nitr, newitem);
                            }
                        }
                    }
                }
                else
                {
                    // Predict.
                    if (G[rule][alt][dot] < G.size())
                    {
                        int nonTerm = G[rule][alt][dot];
                        for (int AA = 0, AE = G[nonTerm].size(); AA < AE; ++AA)
                        {
                            auto newitem    = std::make_tuple(nonTerm, AA, 0, KK);
                            auto nitr       = nss.find(newitem);
                            if (nitr == nss.end())
                            {
                                nss.insert(nitr, newitem);
                            }
                            if (!G[nonTerm][AA].empty() && nullset[G[nonTerm][AA][0]])
                            {
                                auto newitem    = std::make_tuple(nonTerm, AA, 1, KK);
                                auto nitr       = nss.find(newitem);
                                if (nitr == nss.end())
                                {
                                    nss.insert(nitr, newitem);
                                }
                            }
                        }
                    }
                    else
                    {
                        // Scan.
                        if (G[rule][alt][dot] == expr[KK])
                        {
                            item_t matched  = item;
                            std::get<DOT>(matched) = std::get<DOT>(matched) + 1;
                            states[KK+1].insert(matched);
                        }
                    }
                }
            }
            int size    = states[KK].size();
            states[KK].insert(nss.begin(), nss.end());
            addedTerm   = size < states[KK].size();
            consider    = nss;
        } while (addedTerm);
    }

    for (int KK = 0, KE = states.size(); KK < KE; ++KK)
    {
        fprintf(stdout, "%*s === %d ===\n", 10, "", KK);
        for (auto const& item : states[KK])
        {
            auto rule   = std::get<RULE>(item);
            auto alt    = std::get<ALT>(item);
            auto dot    = std::get<DOT>(item);
            auto ref    = std::get<REF>(item);
            fprintf(stdout, "%s", "    ");
            PrintAlt(names, G, rule, alt, dot, ref);
        }
    }

    for (auto const& item : states.back())
    {
        auto rule   = std::get<RULE>(item);
        auto alt    = std::get<ALT>(item);
        auto dot    = std::get<DOT>(item);
        if ((rule == 0) && (dot >= G[rule][alt].size()))
        {
            return true;
        }
    }

    return false;
}
