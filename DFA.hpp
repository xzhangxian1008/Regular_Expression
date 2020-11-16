#ifndef DFA
#define DFA

#include "parse_regular.hpp"
#include "data_struct.hpp"
#include "utility.hpp"

class Dfa
{
public:
    vector<Nr*> pnrs; // 保存所有需要参与其中的Nr指针
    node_id start_id; // Nr组合成NFA时使用
    dnode_id start_did; // DFA起始节点
    enum STATE
    {
        ORDINARY, RECEIVE
    };
public:
    Dfa(vector<Nr*> _pnrs);
public:
    void printNFA();
    void printDFA();
};

#endif