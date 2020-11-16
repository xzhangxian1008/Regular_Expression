#ifndef REGULAR_TO_NFA
#define REGULAR_TO_NFA

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <deque>
#include <cctype>
using namespace std;

typedef int node_id;//表示NFA中节点id
typedef int dnode_id;
typedef int token_num;//yacc中使用define数字的方式标识token

class Node;
class DfaNode;
class Mark;

extern node_id current_id;
extern vector<Node*> nodes;

extern dnode_id max_id;
extern vector<DfaNode*> pdnodes;
extern deque<int> avail_space;

inline node_id get_next_node_id()
{
    return ++current_id;
}

inline bool is_node_existed(node_id id)
{
    if (id >= nodes.size())
        return false;
    
    return nodes[id];
}

bool operator<(const Mark m1, const Mark m2);
void set_node(node_id id, Node *node);
void clear_node(node_id id);

dnode_id get_dnode();
dnode_id create_dnode(vector<node_id> ids);
vector<node_id> get_closure(deque<node_id> ids);

/*这是词法中的字符，也是组成单词的最小单元
 *注意：<=和<这样的符号都是最小单元*/
class Char
{
public:
    string c;
    int len;//标记string长度
    bool if_used;//对于一个非终结符Nr来说，其中的Char需要被抑制不再使用
public:
    Char(string s): c(s), len(c.length()), if_used(true){}
    Char(bool _if_used): c(""), len(-1), if_used(_if_used){}
    static bool is_number(const Char ch);
    static bool is_letter(const Char ch);
    static bool is_capital(const Char ch);
    static bool is_lowercase(const Char ch);
    static bool is_empty(const Char ch) {return (ch.len == 0);}
public:
    bool operator==(const Char &ch) const {return (c == ch.c);}               
    bool operator!=(const Char &ch) const {return (c != ch.c);}
    bool operator<(const Char &ch) const {return (c < ch.c);}
    void set_if_used(bool _if_used) { if_used = _if_used; }
    void printChar() const;
};

//Mark类用于表示有哪些符号对应于节点的出边
class Mark
{
public:
    Mark(): all_letter(false), all_capital(false),
        all_lowercase(false), all_number(false){}
    explicit Mark(Char c): all_letter(false), all_capital(false),
        all_lowercase(false), all_number(false){ add(c); }
    bool check(Char c) const;
    void add(const Char &c);
    void add(const Mark &m);
    void set_all_letter() {all_letter = true; all_capital = true; all_lowercase = true;}
    void set_all_number() {all_number = true;}
    void set_all_capital() {all_capital = true;}
    void set_all_lowercase() {all_lowercase = true;}
    bool operator<(const Mark m);
    void printMark() const;
public:
    bool all_letter;
    bool all_capital;
    bool all_lowercase;
    bool all_number;
    vector<Char> marks;
};

class Node
{
public:
    enum STATE
    {
        //START_RECEIVE表示是带*的接收状态，我们假设最多只有一颗星
        START, ORDINARY, RECEIVE, UNDEFINED
    };
    enum STATE state;//用于标记这个节点在NFA中的状态
    token_num recv_type;
    node_id id;
    multimap<Mark, node_id> neighbour;
public:
    Node(){state = UNDEFINED; id = -1;}
    Node(STATE _state, node_id _id): state(_state), id(_id){ set_node(id, this); }
public:
    STATE get_state() {return state;}
    node_id get_id() {return id;}
    void add_neighbour(Mark m, node_id nei) {neighbour.insert(make_pair(m, nei));}
    vector<node_id> next_nodes(Char c);
    vector<node_id> all_neighbours();
};

inline void delete_node(Node *pnode)
{
    nodes[pnode->id] = nullptr;
    delete pnode;
}

inline node_id create_node(Node::STATE state)
{
    node_id id = get_next_node_id();
    Node *pnode = new Node(state, id);
    nodes[id] = pnode;

    return id;
}

/* 一开始没有考虑到node在DFA中使用的需要
 * 所以创建一个新的类型来表达DFA节点*/
class DfaNode
{
public:
    enum STATE {ORDINARY, RECEIVE};

    dnode_id id;
    enum STATE state;
    vector<node_id> node_ids;
    map<Mark, vector<node_id>> node_neighbour;// 存放这个dnode的全部node邻居
    map<Mark, dnode_id> dnode_neighbour;// 存放这个dnode的全部dnode邻居
    vector<token_num> recv_types;
public:
    DfaNode(dnode_id _id, vector<node_id> _node_ids);
    vector<dnode_id> generate_neighbour();

    void print_node_neighbour();
    void print_dnode_neighbour();
    void print_nodes_in_dnode();
};

#endif