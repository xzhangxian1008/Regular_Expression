#include "parse_regular.hpp"
#include "utility.hpp"

extern int current_id;
extern vector<Node*> nodes;

//控制reduce_Px是不是要打印调试信息
bool if_reduce_print = false;

/*记录左中括号(square bracket)是否出现。如果sbra已被压入stack栈，后续操作符
 *只有"-"是有效的，其余均非法，并且此时只能进行P1和P3操作，其它均被抑制。直到
 *右中括号出现并将其规约后上述限制才被取消*/
bool left_sbra_existed = false;

inline bool is_ordinary_terminal(char c)
{
    if ((c == '_') ||
        ((c >= 'a') && (c <= 'z')) ||
        (((c >= 'A') && (c <= 'Z'))) ||
        ((c >= '0') && (c <= '9'))
        )
        return true;
    
    return false;
}


/*该Nr要是被删除的话务必要将其中的Node*全部回收
 *回收方法选用广度优先搜索（其实深优也差不多）*/
Nr::~Nr()
{
    /*设置那么多道判断完全是出于无奈，因为就算提醒自己在书写一些函数的时候要
     *注意规范以防止不合法的情况出现，但总会有不合法的情况出现。
     *所以只能在这最后一道防线多设置几道关卡以防止意外情况出现
     *还有一个事实要记住，if的使用会增大分支预测的失败从而造成错误执行带来的开销*/
    //如果这个Nr没有被设置为DELETED就不要把它指向的node删除
    if ((this->state != DELETED) || 
        (this->state != NON_TERMINAL))
        return;
    
    // if (this->state != NON_TERMINAL)
    //     return;
    
    if (this->start_id == -1 || this->recv_id == -1)
        return;
    
    deque<node_id> q;
    node_id nid = start_id;
    Node* pnode = nodes[nid];

    q.push_back(start_id);

    while (!q.empty())
    {
        nid = q.front();
        pnode = nodes[nid];
        q.pop_front();

        //对邻居进行扫描，把所有还存在的邻居压入队列
        for (multimap<Mark, int>::iterator iter = pnode->neighbour.begin();
            iter != pnode->neighbour.end(); ++iter)
            if (nodes[iter->second] != nullptr)
                q.push_back(iter->second);
        
        delete_node(pnode);
    }
}

void Regular::
print_regular_stack(vector<Nr*> &regular_stack)
{
    for (int i = regular_stack.size() - 1; i >= 0; --i)
        cout << regular_stack[i]->terminal.c << " " << regular_stack[i]->state << endl;
    
    cout << endl;
}

//当正则表达式是以反斜杠结尾的时候需要另作处理，并且将去除了以连续反斜杠作为结尾的
//新字符串长度返回
int Regular::
process_end_backslash(vector<Nr*> &regular_stack,string &regular)
{
    int regular_length = regular.length();
    int end_backslash_len = 0;
    int i = regular_length - 1;

    while ( (i >= 0) && (regular[i] == '\\'))
    {
        --i;
        ++end_backslash_len;
    }
    
    //如果尾部连续的反斜杠数目是奇数则报错
    if (end_backslash_len % 2 != 0)
    {
        error("end_backslash_len is odd number");
        exit(-1);
    }

    ++i;

    //把尾部被转义了的反斜杠压入regular_stack
    while (i < regular_length)
    {
        Char c(string(1, '\\'));
        Nr *nr = new Nr(c, Nr::ORDINARY_TERMINAL);

        regular_stack.push_back(nr);

        i += 2;
    }

    return (regular_length - end_backslash_len);
}

/*把string类型的正则表达式压入到正则表达式栈中作为输入
 *压入的时候要判断被压入终结符是属于操作终结符还是普通终结符
 *还要注意转义字符的存在，跟在转义字符后面的终结符统一作为
 *普通终结符*/
void Regular::
push_regular_into_stack(vector<Nr*> &regular_stack, string &regular)
{
    vector<Nr*> tmp_stack;//这个栈用于出现连续转义字符\的时候临时存储数据使用
    int regular_length = regular.length();
    int backslash_length;
    int tmp;
    int j;
    int k;

    if (regular[regular_length - 1] == '\\')
        regular_length = process_end_backslash(regular_stack, regular);

    for (int i = regular_length - 1; i >= 0; --i)
    {
        backslash_length = 0;
        Char c(string(1, regular[i]));
        Nr *nr = new Nr(c);

        j = i - 1;
        while ((j >= 0) && (regular[j] == '\\')) 
        {
            --j;
            ++backslash_length;
        }
        
        k = j + 1;
        
        if (k == i)
        {
            //regular[i]的前面没有转义字符就直接压栈
            if (is_ordinary_terminal(regular[i]))
                nr->set_state(Nr::ORDINARY_TERMINAL);
            else
                nr->set_state(Nr::OPERATION_TERMINAL);
            
            regular_stack.push_back(nr);
            continue;
        }

        //k!=i就说明出现了\，此时i的下一个位置不是单纯的--i就可以，它必须跳到第一个'\'
        tmp = i;//i所处位置临时保存在tmp
        i = k;
        
        //把这段连续反斜杠压入临时栈中
        while (k <= tmp)        
        {
            if ( (regular[k] == '\\') && (k + 1 < regular_length) )
            {
                Char c(string(1, regular[k + 1]));
                Nr *nr = new Nr(c, Nr::ORDINARY_TERMINAL);

                tmp_stack.push_back(nr);
            }
            else
            {
                Char c(string(1, regular[k]));
                Nr *nr = new Nr(c);

                if (is_ordinary_terminal(regular[k]))
                    nr->set_state(Nr::ORDINARY_TERMINAL);
                else
                    nr->set_state(Nr::OPERATION_TERMINAL);

                tmp_stack.push_back(nr);
            }

            k += 2;
        }

        //把临时栈中的东西再倒到regular_stack中
        while (tmp_stack.size() != 0)
        {
            regular_stack.push_back(tmp_stack.back());
            tmp_stack.pop_back();
        }
    }

    // print_regular_stack(regular_stack);
}

/*Core function*/
Nr* Regular::
parse_regular_to_NFA()
{
    Reduction reduction_type;
    push_regular_into_stack(regular_stack, regular);

    if (regular_stack.size() == 0)
        return nullptr;

    do
    {
        pop_regular_stack_to_stack();
        reduction_type = check_reduction();
        reduce(reduction_type);
    } while (regular_stack.size() != 0);

    Node* precv_node = nodes[stack[0]->recv_id];
    precv_node->recv_type = recv_type; // 向Nr的接受Node中设置接受状态

    // cout << "stack.size() == " << stack.size() << endl;
    return stack[0];
}

void Regular::
pop_regular_stack_to_stack()
{
    if (regular_stack.size() == 0)
        error("regular_stack empty");

    Nr *nr = regular_stack.back();

    if ( left_sbra_existed &&
        nr->state == Nr::OPERATION_TERMINAL &&
        nr->terminal.c[0] != '-' &&
        nr->terminal.c[0] != ']')
    {
        string str = "illegal operation terminal " + string(1, nr->terminal.c[0]);
        str += " in []";
        error(str);
    }

    if ( nr->state == Nr::OPERATION_TERMINAL &&
        nr->terminal.c[0] == '[')
        left_sbra_existed = true;
    
    if ( nr->state == Nr::OPERATION_TERMINAL &&
        nr->terminal.c[0] == ']')
        left_sbra_existed = false;

    stack.push_back(nr);
    regular_stack.pop_back();
}

/*寻找stack中是不是有以]为结尾的[....]形式
 *check_len的改变只是为了保持被调函数和调用点
 *数据的一致性，目前来说并没有实际含义*/
bool Regular::
if_find_lr_square_brackets(int &left, int right, int &check_len)
{
    //如果最右侧不是']'的话也就没有继续检查下去的必要了
    if (!(stack[right]->state == Nr::OPERATION_TERMINAL) ||
        !(stack[right]->terminal.c[0] == ']'))
        return false;

    while (left >= 0)
    {
        if ((stack[left]->state == Nr::OPERATION_TERMINAL) && 
            (stack[left]->terminal.c[0] == '['))
            return true;
        
        --left;
        ++check_len;
    }

    return false;
}

Reduction Regular::
check_reduction()
{
    Nr *pnr;
    int check_len = 1;
    int stack_len = stack.size();
    Reduction reduction_type = P0;

    /*P1的优先级是全场最高的，第一个就检查它，不然会和后面的运算符优先级起冲突*/
    if (check_len_1(stack) == P1)
        return P1;

    for (int i = stack_len - 2; i >= 0; --i)
    {
        ++check_len;

        /*向前看一个符号
         *先查看regular_stack的下一个是不是类似于| *这样的操作符，如果是，则返回P0。
         *比如说ab|c，很明显|的优先级比较高，所以我们必须向前看一位以保证能够以正确
         *的顺序进行结合。
         *如果下一个是优先级比较高的运算符我们不急于马上运算，而是先返回P0，继续压栈
         *的操作，到了后面自然会执行b|c的操作
         *注意：这个符号不能是一个运算符！*/
        if ( regular_stack.size() != 0 && 
            Nr::is_ordinary_terminal(regular_stack.back()) )
        {
            pnr = regular_stack.back();
            if (pnr->terminal.if_used)
                if ( pnr->terminal.c == "|" ||
                    pnr->terminal.c == "*" ||
                    pnr->terminal.c == "+" ||
                    pnr->terminal.c == "?" ||
                    pnr->terminal.c == "-")
                    return P0;
        }

        //当检查产生式的长度大于3的时候就要确定是不是继续检查下去
        if ( (check_len > 3) &&
            !if_find_lr_square_brackets(i, stack_len - 1, check_len) )
            return P0;
        
        switch(check_len)
        {
            case 2:
                reduction_type = check_len_2(stack);
                if (reduction_type != P0)
                    return reduction_type;
                break;
            case 3:
                reduction_type = check_len_3(stack);
                if (reduction_type != P0)
                    return reduction_type;
                break;
            default://default只有可能是检查P2了
                reduction_type = check_len_P2(stack, i);
                return reduction_type;
        }
    }

    return P0;
}

void Regular::
reduce(Reduction reduction_type)
{
    while (reduction_type != P0)
    {
        switch(reduction_type)
        {
            case P1:
                reduce_P1(stack);
                reduction_type = check_reduction();
                break;
            case P2:
                reduce_P2(stack);
                reduction_type = check_reduction();
                break;
            case P3:
                reduce_P3(stack);
                reduction_type = check_reduction();
                break;
            case P4:
                reduce_P4(stack);
                reduction_type = check_reduction();
                break;
            case P5:
                reduce_P5(stack);
                reduction_type = check_reduction();
                break;
            case P6:
                reduce_P6(stack);
                reduction_type = check_reduction();
                break;
            case P7:
                reduce_P7(stack);
                reduction_type = check_reduction();
                break;
            case P8:
                reduce_P8(stack);
                reduction_type = check_reduction();
                break;
            case P9:
                reduce_P9(stack);
                reduction_type = check_reduction();
                break;
            default:
                cout << "reduce receives reduction_type:"
                    << reduction_type << endl;
                return;
        }
    }

}

//使用广度优先进行打印
void Nr::
printNr() const
{
    vector<bool> flag(current_id, false);
    vector<node_id> next_nodes;
    deque<node_id> q;
    node_id node;
    Node* pnode;

    q.push_back(start_id);
    flag[start_id] = true;
    while (!q.empty())
    {
        node = q.front();
        pnode = nodes[node];
        q.pop_front();
        
        for (multimap<Mark, node_id>::iterator iter = pnode->neighbour.begin();
            iter != pnode->neighbour.end(); ++iter)
        {
            cout << node << "->" << iter->second << " ";
            iter->first.printMark();

            //如果邻居被加入过则无需再次加入
            if (flag[iter->second])
                continue;
            
            q.push_back(iter->second);
            flag[iter->second] = true;
        }
    }
}

inline Reduction check_len_1(vector<Nr*> &stack)
{
    if (stack.back()->state == Nr::ORDINARY_TERMINAL)
        return P1;
    
    return P0;
}

//产生式右侧长度为2的有P6,P7,P8,P9
inline Reduction check_len_2(vector<Nr*> &stack)
{
    if (left_sbra_existed)
        return P0;

    if (check_if_P6(stack))
        return P6;
    
    if (check_if_P7(stack))
        return P7;
    
    if (check_if_P8(stack))
        return P8;
    
    if (check_if_P9(stack))
        return P9;
    
    return P0;
}

//产生式右侧长度为2的有P3,P4,P5
inline Reduction check_len_3(vector<Nr*> &stack)
{
    if (check_if_P3(stack))
        return P3;
    
    if (check_if_P2(stack))
        return P2;

    //如果出现了左中括号，某些功能就不再使用
    if (left_sbra_existed)
        return P0;

    if (check_if_P4(stack))
        return P4;
    
    if (check_if_P5(stack))
        return P5;
    
    return P0;
}

inline Reduction check_len_P2(vector<Nr*> &stack, int left)
{
    if (check_if_P2(stack, left))
        return P2;
    
    return P0;
}

//E->[E1E2E3...En] 注意这里右侧的E全部是非终结符
bool check_if_P2(vector<Nr*> &stack, int left)
{
    int right = stack.size() - 1;

    for (int i = left + 1; i < right; ++i)
        if (stack[i]->state != Nr::NON_TERMINAL && stack[i]->rule != Nr::BASIC_RULE)
            return false;
    
    //判断两端是不是[]
    if ( (stack[left]->state == Nr::OPERATION_TERMINAL) &&
        (stack[left]->terminal.c[0] == '[') &&
        (stack[right]->state == Nr::OPERATION_TERMINAL) &&
        (stack[right]->terminal.c[0] == ']') )
        return true;
    
    return false;
}

bool check_if_P2(vector<Nr*> &stack)
{
    int right = stack.size() - 1;
    int left = right - 2;

    if (stack[left + 1]->state != Nr::NON_TERMINAL &&
        stack[left + 1]->rule != Nr::BASIC_RULE)
        return false;

    if ( (stack[left]->state == Nr::OPERATION_TERMINAL) &&
        (stack[left]->terminal.c[0] == '[') &&
        (stack[right]->state == Nr::OPERATION_TERMINAL) &&
        (stack[right]->terminal.c[0] == ']') )
        return true;

    return false;
}

//E->E1-E2
bool check_if_P3(vector<Nr*> &stack)
{
    int right = stack.size() - 1;
    int left = right - 2;

    if ( !(Nr::is_non_terminal(stack[left])) ||
        !(Nr::is_non_terminal(stack[right])) )
        return false;
    
    if ( (Nr::is_operation_terminal(stack[left + 1])) &&
        (stack[left + 1]->terminal.c[0] == '-'))
        return true;

    return false;
}

//E->(E)
bool check_if_P4(vector<Nr*> &stack)
{
    int right = stack.size() - 1;
    int left = right - 2;

    if ( !(Nr::is_operation_terminal(stack[left])) ||
        !(Nr::is_operation_terminal(stack[right])) )
        return false;
    
    if ( (stack[left]->terminal.c[0] == '(') &&
        (stack[right]->terminal.c[0] == ')') &&
        Nr::is_non_terminal(stack[left + 1]))
        return true;

    return false;
}

//E->E1|E2
bool check_if_P5(vector<Nr*> &stack)
{
    int right = stack.size() - 1;
    int left = right - 2;

    if ( !(Nr::is_non_terminal(stack[left])) ||
        !(Nr::is_non_terminal(stack[right])) )
        return false;
    
    if ( (Nr::is_operation_terminal(stack[left + 1])) &&
        (stack[left + 1]->terminal.c[0] == '|'))
        return true;

    return false;
}

//E->E1E2
bool check_if_P6(vector<Nr*> &stack)
{
    int right = stack.size() - 1;
    int left = right - 1;

    //两个必须均为非终结符才能进行连接操作
    if ( (Nr::is_non_terminal(stack[left])) &&
        (Nr::is_non_terminal(stack[right])) )
        return true;
    
    return false;
}

//E->E*
bool check_if_P7(vector<Nr*> &stack)
{
    int right = stack.size() - 1;
    int left = right - 1;

    if ( (Nr::is_non_terminal(stack[left])) &&
        (Nr::is_operation_terminal(stack[right])) &&
        (stack[right]->terminal.c[0] == '*'))
        return true;
    
    return false;
}

//E->E+
bool check_if_P8(vector<Nr*> &stack)
{
    int right = stack.size() - 1;
    int left = right - 1;

    if ( (Nr::is_non_terminal(stack[left])) &&
        (Nr::is_operation_terminal(stack[right])) &&
        (stack[right]->terminal.c[0] == '+'))
        return true;
    
    return false;
}

//E->E?
bool check_if_P9(vector<Nr*> &stack)
{
    int right = stack.size() - 1;
    int left = right - 1;

    if ( (Nr::is_non_terminal(stack[left])) &&
        (Nr::is_operation_terminal(stack[right])) &&
        (stack[right]->terminal.c[0] == '?'))
        return true;
    
    return false;
}

void do_basic_rule(Nr *nr, Mark m)
{
    nr->start_id = create_node(Node::START);
    nr->recv_id = create_node(Node::RECEIVE);

    nodes[nr->start_id]->add_neighbour(m, nr->recv_id);
    nr->set_state(Nr::NON_TERMINAL);
    nr->set_rule(Nr::BASIC_RULE);
}

//E->id
void reduce_P1(Nr *nr)
{
    print("reduce_P1 is executing...", if_reduce_print);
    /*这个函数仅仅是用于正则转NFA的基本规则，
     *必须确保这个Nr代表的是普通终结符并且是
     *第一次初始化*/
    if ( (nr->state != Nr::ORDINARY_TERMINAL) ||
        (nr->start_id != -1) ||
        (nr->recv_id != -1))
    {
        error("reduce_P1 encounter illegal Nr");
    }

    do_basic_rule(nr, Mark(nr->terminal));
}

//对P1做规约就不需要弹栈压栈了，直接原地解决即可
inline void reduce_P1(vector<Nr*> &stack)
{
    Nr *nr = stack.back();

    reduce_P1(nr);
}

//E->[E1E2...En] 将其规约为一个基本规则
void reduce_P2(vector<Nr*> &stack)
{
    Nr *opr = stack.back();
    opr->delete_Nr();
    stack.pop_back();

    node_id new_start = create_node(Node::START);
    node_id new_recv = create_node(Node::RECEIVE);
    Nr *new_nr = new Nr(new_start, new_recv, Nr::NON_TERMINAL);
    new_nr->set_rule(Nr::BASIC_RULE);

    Nr *nr = stack.back();
    stack.pop_back();

    //新nr中基本规则的标记
    Mark m;

    //因为在检查的时候就已经保证栈中的符号满足式子要求，所以无需进行更严格的检查
    while (Nr::is_non_terminal(nr))
    {
        //因为是基本规则，所以它的邻居肯定只有一个
        multimap<Mark, node_id>::iterator iter = nodes[nr->start_id]->neighbour.begin();

        iter->first;
        m.add(iter->first);

        nr = stack.back();
        stack.pop_back();
    }

    nodes[new_start]->add_neighbour(m, new_recv);

    stack.push_back(new_nr);
}

//E->E1-E2
void reduce_P3(vector<Nr*> &stack)
{
    print("reduce_P3 is executing...", if_reduce_print);
    Nr *e2 = stack.back();
    stack.pop_back();
    Nr *opr_nr = stack.back();
    stack.pop_back();
    Nr *e1 = stack.back();
    stack.pop_back();

    if (!check_P3_expr(e1, e2))
        error("illegal [] expression");
    
    char ce1 = e1->terminal.c[0];
    char ce2 = e2->terminal.c[0];

    //删除不再需要的nr
    opr_nr->delete_Nr(true);
    e2->delete_Nr();

    //不创建新的nr，在e1基础上进行修改
    Mark m;

    if (ce1 == '0' && ce2 == '9')
    {
        m.set_all_number();
        do_basic_rule(e1, m);
        stack.push_back(e1);
        return;
    }
    if (ce1 == 'a' && ce2 == 'z')
    {
        m.set_all_lowercase();
        do_basic_rule(e1, m);
        stack.push_back(e1);
        return;
    }
    if (ce1 == 'A' && ce2 == 'Z')
    {
        m.set_all_capital();
        do_basic_rule(e1, m);
        stack.push_back(e1);
        return;
    }

    //如果没有满足上面的要求那我们就把这个范围内的字符一个个地加进Mark中
    for (char c = ce1; c <= ce2; ++c)
        m.add(Char(string(1, c)));
    
    do_basic_rule(e1, m);
    stack.push_back(e1);
}

/*检查E->E1-E2中的E1和E2是不是合法
 *两个必须都是大写或者小写或者数字
 *两个的顺序不能相反*/
bool check_P3_expr(Nr *e1, Nr *e2)
{
    char ce1 = e1->terminal.c[0];
    char ce2 = e2->terminal.c[0];

    if ( isupper(ce1) &&
        isupper(ce2) &&
        ce1 <= ce2)
        return true;

    if ( islower(ce1) &&
        islower(ce2) &&
        ce1 <= ce2)
        return true;

    if ( isdigit(ce1) &&
        isdigit(ce2) &&
        ce1 <= ce2)
        return true;
    
    return false;
}

//E->(E)
void reduce_P4(vector<Nr*> &stack)
{
    print("reduce_P4 is executing...", if_reduce_print);
    Nr *right = stack.back();
    stack.pop_back();
    Nr *nr = stack.back();
    stack.pop_back();
    Nr *left = stack.back();
    stack.pop_back();

    right->delete_Nr(true);
    left->delete_Nr(true);

    stack.push_back(nr);
}

//E->E1|E2
void reduce_P5(vector<Nr*> &stack)
{
    print("reduce_P5 is executing...", if_reduce_print);
    Nr* e2 = stack.back();
    stack.pop_back();
    Nr* opr = stack.back();
    stack.pop_back();
    opr->delete_Nr(true);
    Nr* e1 = stack.back();
    stack.pop_back();

    Nr *nr = reduce_P5(e1, e2);

    stack.push_back(nr);
}

//E->E1|E2
Nr* reduce_P5(Nr *e1, Nr *e2)
{
    node_id start_id = create_node(Node::START);
    Node *start_pnode = nodes[start_id];
    
    node_id recv_id = create_node(Node::RECEIVE);
    Node *recv_pnode = nodes[recv_id];

    //添加节点间的邻居关系
    start_pnode->add_neighbour(Mark(Char(string(""))), e1->start_id);
    start_pnode->add_neighbour(Mark(Char(string(""))), e2->start_id);
    nodes[e1->recv_id]->add_neighbour(Mark(Char(string(""))), recv_id);
    nodes[e2->recv_id]->add_neighbour(Mark(Char(string(""))), recv_id);

    //改变节点状态
    nodes[e1->start_id]->state = Node::ORDINARY;
    nodes[e1->recv_id]->state = Node::ORDINARY;
    nodes[e2->start_id]->state = Node::ORDINARY;
    nodes[e2->recv_id]->state = Node::ORDINARY;

    Nr *nr = new Nr(start_id, recv_id, Nr::NON_TERMINAL);
    nr->set_state(Nr::NON_TERMINAL);
    nr->set_rule(Nr::CONCLUSION_RULE);

    e1->delete_Nr();
    e2->delete_Nr();

    return nr;
}

//E->E1E2
void reduce_P6(vector<Nr*> &stack)
{
    print("reduce_P6 is executing...", if_reduce_print);
    Nr* e2 = stack.back();
    stack.pop_back();
    Nr* e1 = stack.back();
    stack.pop_back();

    node_id e2_start_id = e2->start_id;

    reduce_P6(e1, e2);

    //越是高等级抽象的越是要放在最后销毁！
    delete_node(nodes[e2_start_id]);

    stack.push_back(e1);
}

//E->E1E2 原e2被删除，新Nr是在e1的基础上构成
inline void reduce_P6(Nr *e1, Nr *e2)
{
    e1->set_state(Nr::NON_TERMINAL);
    e1->set_rule(Nr::CONCLUSION_RULE);

    /*向e1 recv_id中添加e2 start_id的所有邻居,并修改该节点的状态
     *注意：这里最好的做法是删除e2 start_id，因为如果是删除e1 recv_id
     *势必需要找到e1 recv_id的前驱结点，对它们的邻居进行修改，删除旧的
     *指向新的，这会非常的麻烦！*/
    nodes[e1->recv_id]->neighbour = nodes[e2->start_id]->neighbour;
    nodes[e1->recv_id]->state = Node::ORDINARY;

    e1->recv_id = e2->recv_id; // 修改e1的recv_id为e2的

    e2->delete_Nr();
}

//E->E*
void reduce_P7(vector<Nr*> &stack)
{
    print("reduce_P7 is executing...", if_reduce_print);
    Nr *opr = stack.back();
    stack.pop_back();
    Nr *e = stack.back();
    stack.pop_back();

    reduce_P7(e);

    opr->delete_Nr();

    stack.push_back(e);
}

//E->E*
void reduce_P7(Nr* pnr)
{
    pnr->set_state(Nr::NON_TERMINAL);
    pnr->set_rule(Nr::CONCLUSION_RULE);
    node_id old_start = pnr->start_id;
    node_id old_recv = pnr->recv_id;
    nodes[old_start]->state = Node::ORDINARY;
    nodes[old_recv]->state = Node::ORDINARY;
    
    node_id new_start = create_node(Node::START);
    node_id new_recv = create_node(Node::RECEIVE);
    pnr->start_id = new_start;
    pnr->recv_id = new_recv;

    nodes[new_start]->add_neighbour(Mark(Char(string(""))), old_start);
    nodes[new_start]->add_neighbour(Mark(Char(string(""))), new_recv);
    nodes[old_recv]->add_neighbour(Mark(Char(string(""))), old_start);
    nodes[old_recv]->add_neighbour(Mark(Char(string(""))), new_recv);
}

//E->E+ 可转换为 E->EE*
void reduce_P8(vector<Nr*> &stack)
{
    print("reduce_P8 is executing...", if_reduce_print);
    Nr *opr = stack.back();
    stack.pop_back();
    Nr *e1 = stack.back();
    stack.pop_back();

    Nr *e2 = copy_Nr(e1);

    reduce_P7(e2);
    reduce_P6(e1, e2);

    opr->delete_Nr();

    stack.push_back(e1);
}

//E->E? 可转换为 E->""|E
void reduce_P9(vector<Nr*> &stack)
{
    print("reduce_P9 is executing...", if_reduce_print);
    Nr *opr = stack.back();
    stack.pop_back();
    Nr *e2 = stack.back();
    stack.pop_back();
    Nr *e1 = new Nr(Char(string("")), Nr::ORDINARY_TERMINAL);

    reduce_P1(e1);
    Nr *nr = reduce_P5(e1, e2);

    opr->delete_Nr(opr);

    stack.push_back(nr);
}

Nr* copy_Nr(Nr *pnr)
{
    //如果nr不是非终结符，我们无需为其创建新节点，做简单的拷贝即可
    if ( pnr->state == Nr::OPERATION_TERMINAL ||
        pnr->state == Nr::ORDINARY_TERMINAL )
    {
        Nr *new_nr = new Nr(*pnr);
        return new_nr;
    }

    //用于对应新旧节点，key为旧节点，value为新节点
    map<node_id, node_id> copy_map;
    deque<node_id> q;

    Nr *new_nr = new Nr(*pnr);
    new_nr->start_id = create_node(Node::START);
    new_nr->recv_id = create_node(Node::RECEIVE);
    copy_map.insert(make_pair(pnr->start_id, new_nr->start_id));
    copy_map.insert(make_pair(pnr->recv_id, new_nr->recv_id));

    q.push_back(pnr->start_id);

    while (!q.empty())
    {
        node_id self_id = q.front();//旧nr中自身节点
        node_id map_self_id = copy_map.find(self_id)->second;//新nr中映射的自身节点
        Node *pnode = nodes[self_id];
        q.pop_front();

        for (multimap<Mark, node_id>::iterator iter = pnode->neighbour.begin();
            iter != pnode->neighbour.end(); ++iter)
        {
            map<node_id, node_id>::iterator nei_iter = copy_map.find(iter->second);
            //判断被copy Nr中的这个节点是不是已经被被映射过
            if (nei_iter != copy_map.end())
            {
                /*如果属于旧Nr的这个node被映射过，则把相应的新Nr node
                 *加入到新Nr中其前驱节点的邻居*/
                nodes[map_self_id]->add_neighbour(iter->first, nei_iter->second);
                continue;
            }

            node_id new_nei_id = create_node(Node::ORDINARY);
            q.push_back(iter->second);
            copy_map.insert(make_pair(iter->second, new_nei_id));
            nodes[map_self_id]->add_neighbour(iter->first, new_nei_id);
        }
    }

    return new_nr;
}