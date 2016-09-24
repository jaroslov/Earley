#include <stdio.h>
#include <set>
#include <vector>

typedef std::vector<int>        sequence;
typedef std::vector<sequence>   alternate;
typedef std::vector<alternate>  grammar;

typedef std::vector<int>        input;

bool Parse(grammar&, input const& expr);

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

const char* NAMES[] =
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

void PrintAlt(grammar& G, int rule, int alt, int dot=-1, int ref=-1)
{
    fprintf(stdout, "%12s ::=", NAMES[rule]);
    sequence const& s   = G[rule][alt];
    int length  = 0;
    for (int TT = 0, TE = s.size(); TT < TE; ++TT)
    {
        if ((dot >= 0) && (dot == TT))
        {
            length += fprintf(stdout, " %s", "*");
        }
        length += fprintf(stdout, " %s", NAMES[s[TT]]);
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

void PrintRule(grammar& G, int rule)
{
    for (int AA = 0, AE = G[rule].size(); AA < AE; ++AA)
    {
        PrintAlt(G, rule, AA);
    }
}

void PrintGrammar(grammar& G)
{
    for (int RR = 0, RE = G.size(); RR < RE; ++RR)
    {
        PrintRule(G, RR);
    }
}

int main(int argc, char *argv[])
{
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

    PrintGrammar(G);

    input expr = { DIGIT, PLUSMINUS, LPAREN, DIGIT, TIMESDIVIDE, DIGIT, PLUSMINUS, DIGIT, RPAREN };

    fprintf(stdout, "Parse %s.\n",
        Parse(G, expr)
        ? "succeeded"
        : "failed");

    return 0;
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

bool Parse(grammar& G, input const& expr)
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
                    for (auto const& preitem : states[ref])
                    {
                        int crule   = std::get<RULE>(preitem);
                        int calt    = std::get<ALT>(preitem);
                        int cdot    = std::get<DOT>(preitem);
                        int cref    = std::get<REF>(preitem);
                        if ((cdot < G[crule][calt].size()) && (G[crule][calt][cdot] == rule))
                        {
                            nss.insert(std::make_tuple(crule, calt, cdot+1, cref));
                        }
                    }
                }
                else
                {
                    if (G[rule][alt][dot] < G.size())
                    {
                        int nonTerm = G[rule][alt][dot];
                        for (int AA = 0, AE = G[nonTerm].size(); AA < AE; ++AA)
                        {
                            nss.insert(std::make_tuple(nonTerm, AA, 0, KK));
                        }
                    }
                    else
                    {
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
        fprintf(stdout, "STATE(%d)\n", KK);
        for (auto const& item : states[KK])
        {
            auto rule   = std::get<RULE>(item);
            auto alt    = std::get<ALT>(item);
            auto dot    = std::get<DOT>(item);
            auto ref    = std::get<REF>(item);
            fprintf(stdout, "%s", "    ");
            PrintAlt(G, rule, alt, dot, ref);
        }
    }

    for (auto const& item : states.back())
    {
        auto rule   = std::get<RULE>(item);
        auto alt    = std::get<ALT>(item);
        auto dot    = std::get<DOT>(item);
        if ((rule == ENTRY) && (dot >= G[rule][alt].size()))
        {
            return true;
        }
    }

    return false;
}
