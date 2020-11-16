#include "data_struct.hpp"
#include "parse_regular.hpp"
#include "DFA.hpp"

// string operation_terminal = "()\\*+?[]-";

/*正则表达式->NFA->DFA*/
int main(int argc, char** argv)
{
    string regular1 = "a|b";
    string regular2 = "cd";
    vector<Nr*> pnrs;

    Regular re1(regular1, 111);
    Regular re2(regular2, 222);

    Nr* pnr1 = re1.parse_regular_to_NFA();
    Nr* pnr2 = re2.parse_regular_to_NFA();


    // pnr1->printNr();
    // cout << endl;
    // pnr2->printNr();
    // cout << endl;

    pnrs.push_back(pnr1);
    pnrs.push_back(pnr2);

    Dfa dfa(pnrs);
    // dfa.printNFA();

    dfa.printDFA();

    return 0;
}