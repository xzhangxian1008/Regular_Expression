/*正则表达式将要实现的符号：()\*+?[]-连接*/
#ifndef PARSE_REGULAR
#define PARSE_REGULAR

#include "data_struct.hpp"

// string operation_terminal = "()\\*+?[]-";

/*
 *按照产生式右部长度划分：
 *长度1：P1
 *长度2：P6,P7,P8,P9
 *长度3：P3,P4,P5
 *长度>=3：P2
 *补充：中括号[]在这里仅仅实现类似[a-z0-9]或[A-Z1234]这样的功能
 *括号中仅支持操作符"-"，其中出现的其它操作符均属于非法。
 */
enum Reduction
{
    /*P1-P3全是基本规则*/
    P0,//表示不规约
    P1,//E->id
    P2,//E->[E1E2E3...En]
    P3,//E->E1-E2
    /*P4-P10全是归纳规则，其中+？要进行另外的转换*/
    P4,//E->(E)
    P5,//E->E1|E2
    P6,//E->E1E2
    P7,//E->E*
    P8,//E->E+ 可转换为 E->EE*
    P9,//E->E? 可转换为 E->""|E
};

class Nr;

/*下面关于规约的产生式函数全部都依赖于stack！！！！
 *往后写函数的最佳实践就是尽力让它独立于别的数据，换言之要对其准备处理的数据做抽象
 *但是在初期编写的时候没必要过早考虑抽象性问题，因为很多的东西只有在写完了之后才发现
 *需要抽象，过早进行抽象不仅使得编码更加复杂而且很可能是进行了一些没必要的抽象*/
inline Reduction check_len_1(vector<Nr*> &stack);
inline Reduction check_len_2(vector<Nr*> &stack);
inline Reduction check_len_3(vector<Nr*> &stack);//这里也可能会返回P2产生式
inline Reduction check_len_P2(vector<Nr*> &stack, int left);

bool check_if_P2(vector<Nr*> &stack, int left);//E->[E1E2...En] 注意这里右侧的E全部是普通终结符
bool check_if_P2(vector<Nr*> &stack);//E->[E]是上面的特殊形式
bool check_if_P3(vector<Nr*> &stack);//E->E1-E2
bool check_if_P4(vector<Nr*> &stack);//E->(E)
bool check_if_P5(vector<Nr*> &stack);//E->E1|E2
bool check_if_P6(vector<Nr*> &stack);//E->E1E2
bool check_if_P7(vector<Nr*> &stack);//E->E*
bool check_if_P8(vector<Nr*> &stack);//E->E+
bool check_if_P9(vector<Nr*> &stack);//E->E?

/*reduce操作会先把相应Nr弹出，然后自动将规约后的Nr*压入栈中
 *
 *因为reduce中不再进行合法性检测而默认认为可以把其中的东西直接
 *拿出来使用，所以进入reduce之前必须要保证待处理的表达式合法*/
void reduce_P1(Nr *nr);
inline void reduce_P1(vector<Nr*> &stack);
void reduce_P2(vector<Nr*> &stack);
void reduce_P3(vector<Nr*> &stack);
void reduce_P4(vector<Nr*> &stack);
void reduce_P5(vector<Nr*> &stack);
Nr* reduce_P5(Nr *e1, Nr *e2);
void reduce_P6(vector<Nr*> &stack);
inline void reduce_P6(Nr *ple, Nr *pre);
void reduce_P7(vector<Nr*> &stack);
void reduce_P7(Nr *pnr);
void reduce_P8(vector<Nr*> &stack);
void reduce_P9(vector<Nr*> &stack);

bool check_P3_expr(Nr *e1, Nr *e2);
void do_basic_rule(Nr *nr, Mark m);//将nr按照基本规则从终结符转化为非终结符
Nr* copy_Nr(Nr *pnr);//虽说是复制一个nr，其实必须创建全新的node_id替代旧有的node

/*Nr含义见龙书第二版3.7.4*/
class Nr
{
public:
    enum STATE
    {
        /*类名虽然是Nr，但正则式在被解析时，正则式中的字符被压入栈中，此时需要某种方式
         *表达这些字符并且需要为之后进行Nr相关操作提供便利的数据类型，于是就规定了这几个
         *状态*/
        OPERATION_TERMINAL, ORDINARY_TERMINAL, NON_TERMINAL,
        STATE_UNDEFINED, DELETED
    };
    enum RULE
    {
        BASIC_RULE, CONCLUSION_RULE, RULE_UNDEFINED
    };
public:
    node_id start_id;
    node_id recv_id;
    STATE state;
    RULE rule;
    Char terminal;/*只有当state为OPERATION_TERMINAL和ORDINARY_TERMINAL时才会设置
                   *有的Nr会从终结符转为非终结符，Char此时虽然仍保留并且不做任何修改
                   *但是它已经没有任何意义，不应该对它有任何操作*/
public:
    Nr(Char c, STATE _state):terminal(c), state(_state),
        rule(RULE_UNDEFINED), start_id(-1), recv_id(-1){}
    Nr(Char c):terminal(c), state(STATE_UNDEFINED),
        rule(RULE_UNDEFINED), start_id(-1), recv_id(-1){}
    Nr(node_id _start_id, node_id _recv_id, STATE _state):
        terminal(Char(false)), state(_state), rule(RULE_UNDEFINED),
        start_id(_start_id), recv_id(_recv_id){}
    ~Nr();
public:
    inline void set_state(STATE _state);
    inline void set_rule(RULE _rule) { rule = _rule; }
    inline void delete_Nr(bool if_delete_nodes = false);
    static bool is_operation_terminal(Nr *nr) { return (nr->state == OPERATION_TERMINAL); }
    static bool is_ordinary_terminal(Nr *nr) { return (nr->state == ORDINARY_TERMINAL); }
    static bool is_non_terminal(Nr *nr) { return (nr->state == NON_TERMINAL); }
    static bool is_undefined(Nr *nr) { return (nr->state == STATE_UNDEFINED); }
public:
    void printNr() const;
};

inline void Nr::
set_state(STATE _state)
{
    state = _state;
    if ( (state != OPERATION_TERMINAL) &&
        (state != ORDINARY_TERMINAL))
        terminal.set_if_used(false);
}

inline void Nr::
delete_Nr(bool if_delete_nodes)
{
    if (if_delete_nodes)
        this->state = DELETED;
    
    delete this;
}

class Regular
{
public:
    string regular;
    token_num recv_type;//记录该正则式对应的词法单元名
    vector<Nr*> stack;
    vector<Nr*> regular_stack;//用于装载转换为Nr的正则表达式的栈
public:
    Regular(string str, token_num _recv_type): regular(str), recv_type(_recv_type){}
public:
    void print_regular_stack(vector<Nr*> &regular_stack);
    int process_end_backslash(vector<Nr*> &regular_stack, string &regular);
    /*把正则表达式变为Nr然后压入栈*/
    void push_regular_into_stack(vector<Nr*> &regular_stack, string &regular);
    Nr* parse_regular_to_NFA();

    void pop_regular_stack_to_stack();//把regular_stack顶部*Nr弹出并且压入stack
    Reduction check_reduction();//对stack检查栈中式子是不是可以进行规约
    /*进行规约操作，start表示要对从start到stack最后的范围进行规约，reduction_type表示规约类型*/
    void reduce(Reduction reduction_type);//进行规约
public:
    bool if_find_lr_square_brackets(int &left, int right, int &check_len);//找到左右中括号
};


#endif