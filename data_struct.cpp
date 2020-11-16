#include "parse_regular.hpp"
#include "utility.hpp"

/* 我们每增加一个Node就要为其设置一个id，为了防止id被重复设置
 * 我们在这里设置一个current_id用于记录目前使用到的最大id，
 * 但是在构造NFA的时候会出现Node合并的现象，比如3和4号节点合并
 * 使用id=3，此时id=4会空出来，但我们为了避免程序变得更复杂
 * 并且易于编写，这里空出的id=4不再使用，而是选择current_id
 * 往后的数字作为Node标识*/
node_id current_id = 0;
/* 把节点放入相应的id位置，如果位置为null则说明不存在这个节点
 *
 * 下次编写代码应尽量使用智能指针以防止空悬指针的问题*/
vector<Node*> nodes(10, nullptr);

/* 下面是dnode需要的数据结构*/
dnode_id max_id = -1;
vector<DfaNode*> pdnodes;// 存放各个节点，利用索引访问
deque<int> avail_space;// 存放被回收的空间的索引

void set_node(node_id id, Node *node)
{
    if (id >= nodes.size())
    {
        /* 一旦发现nodes的size不够就进行扩容，扩容大小为id的一半
         * 注意这里不能用reserve扩容然后用数组索引的方式进行修改
         * vector,因为空间不够对数据进行转移的时候不会把size之外
         * 的数据转移，而增加size的方法只有push_back。
         * 
         * 因为一开始的设计问题所以这里只能改用循环push_back的低效率
         * 方式进行扩容。*/
        int num = id / 2;
        for (int i = 0; i < num; ++i)
            nodes.push_back(nullptr);
    }

    //如果node id相应位置本身就存在一个node则报一个警告
    if (nodes[id])
    {
        string id_str = to_string(id);
        string str = "set_node replaces the node " + id_str;
        warn(str);
    }

    nodes[id] = node;
}

void clear_node(node_id id)
{
    if (!nodes[id])
    {
        string id_str = to_string(id);
        string str = "node " + id_str + " is an nullptr";
        warn(str);
        return;
    }

    delete nodes[id];
    nodes[id] = nullptr;
}

/* 仅仅获得一个dnode id，并不创建dnode*/
dnode_id get_dnode()
{
    dnode_id ret;

    if (avail_space.size() != 0)
    {
        ret = avail_space.front();
        avail_space.pop_front();

        return ret;
    }

    ret = pdnodes.size();
    pdnodes.push_back(nullptr);

    return ret;
}

/* 创建dnode，传入的参数是组成这个dnode的所有node id
 *
 * ids集合中id的合法性不做检查，自己确保是合法的*/
dnode_id create_dnode(vector<node_id> ids)
{
    dnode_id id = get_dnode();
    pdnodes[id] = new DfaNode(id, ids);

    return id;
}

void clear_dnode(dnode_id id)
{
    if (id < 0 || id >= pdnodes.size() || !pdnodes[id])
    {
        string str = to_string(id) + "is illegal";
        warn(str);
        return;
    }
    
    delete pdnodes[id];
    pdnodes[id] = nullptr;
    avail_space.push_back(id);
}

/* 以ids为起点，找到所有ids通过""到达
 * 的所有node的集合以vector形式返回
 * 其中也包含ids自己*/
vector<node_id> get_closure(deque<node_id> ids)
{
    Mark null(string(""));
    vector<node_id> ret;
    Node* pnode;
    node_id node;
    size_t count;
    multimap<Mark, node_id>::iterator iter;

    while (ids.size() > 0)
    {
        node = ids.front();
        ret.push_back(node);
        ids.pop_front();

        pnode = nodes[node];
        count = pnode->neighbour.count(null);
        iter = pnode->neighbour.find(null);
        while (count)
        {
            ids.push_back(iter->second);
            ++iter;
            --count;
        }
    }

    return ret;
}

bool Mark::operator<(const Mark m)
{
    if (this->all_number != m.all_number)
        return (this->all_number == true ? false : true);
    
    if (this->all_letter != m.all_letter)
        return (this->all_letter == true ? false : true);

    if (this->all_capital != m.all_capital)
        return (this->all_capital == true ? false : true);

    if (this->all_lowercase != m.all_lowercase)
        return (this->all_lowercase == true ? false : true);
    
    if (this->marks.size() != m.marks.size())
        return (this->marks.size() < m.marks.size());
    
    int size = this->marks.size();

    for (int i = 0; i < size; ++i)
        if (this->marks[i] != m.marks[i])
            return (this->marks[i] < m.marks[i]);
    
    return false;
}

bool operator<(const Mark m1, const Mark m2)
{
    if (m1.all_number != m2.all_number)
        return (m1.all_number == true ? false : true);
    
    if (m1.all_letter != m2.all_letter)
        return (m1.all_letter == true ? false : true);

    if (m1.all_capital != m2.all_capital)
        return (m1.all_capital == true ? false : true);

    if (m1.all_lowercase != m2.all_lowercase)
        return (m1.all_lowercase == true ? false : true);
    
    if (m1.marks.size() != m2.marks.size())
        return (m1.marks.size() < m2.marks.size());
    
    int size = m1.marks.size();

    for (int i = 0; i < size; ++i)
        if (m1.marks[i] != m2.marks[i])
            return (m1.marks[i] < m2.marks[i]);
    
    return false;
}

void Mark::add(const Char &c)
{
    if (check(c))
        return;
    
    marks.push_back(c);

    sort(this->marks.begin(), this->marks.end());
}

//把另一个mark中的东西加入到自己身上
void Mark::add(const Mark &m)
{
    if (m.all_capital)
        this->all_capital = m.all_capital;
    if (m.all_letter)
        this->all_letter = m.all_letter;
    if (m.all_lowercase)
        this->all_lowercase = m.all_lowercase;
    if (m.all_number)
        this->all_number = m.all_number;
    
    for (vector<Char>::const_iterator iter = m.marks.begin();
        iter != m.marks.end();
        ++iter)
        this->add(*iter);
}

bool Mark::check(Char c) const
{
    if (all_capital && Char::is_capital(c))
        return true;
    
    if (all_lowercase && Char::is_lowercase(c))
        return true;
    
    if (all_number && Char::is_number(c))
        return true;
    
    for (int i = 0; i < marks.size(); ++i)
        if (c == marks[i])
            return true;
    
    return false;
}

void Mark::
printMark() const
{
    cout << "Mark:" << ends;
    if (all_letter)
        cout << "all_letter " << ends;
    if (all_capital)
        cout << "all_capital " << ends;
    if (all_lowercase)
        cout << "all_lowercase " << ends;
    if (all_number)
        cout << "all_number " << ends;
    
    for (int i = 0; i < marks.size(); ++i)
        marks[i].printChar();
    
    cout << endl;
}

bool Char::
is_number(const Char ch)
{
    if (ch.len != 1) return false;

    char c = ch.c[0];

    if ((c >= '0') && (c <= '9'))
        return true;
    
    return false;
}

bool Char::
is_capital(const Char ch)
{
    if (ch.len != 1) return false;

    char c = ch.c[0];

    if ((c >= 'A') && (c <= 'Z'))
        return true;
    
    return false;
}

bool Char::
is_lowercase(const Char ch)
{
    if (ch.len != 1) return false;

    char c = ch.c[0];

    if ((c >= 'a') && (c <= 'b'))
        return true;
    
    return false;
}

bool Char::
is_letter(const Char ch)
{
    if (Char::is_capital(ch) || Char::is_lowercase(ch))
        return true;
    
    return false;
}

void Char::
printChar() const
{
    if (is_empty(*this))
    {
        cout << "empty";
        return;
    }

    cout << this->c;
}

//根据输入的字符找到状态的后续状态
vector<node_id> Node::next_nodes(Char c)
{
    vector<node_id> ns;

    for (multimap<Mark, node_id>::iterator iter = neighbour.begin();
        iter != neighbour.end(); ++iter)
        if (iter->first.check(c) &&
            nodes[iter->second] != nullptr)
            ns.push_back(iter->second);
    
    return ns;
}

vector<node_id> Node::all_neighbours()
{
    vector<node_id> ns;

    for (multimap<Mark, node_id>::iterator iter = neighbour.begin();
        iter != neighbour.end(); ++iter)
            ns.push_back(iter->second);
    
    return ns;
}

/* 产生邻居节点不会在构造函数中进行，若想产生邻居节点，需要手动调用相关函数
 * 反复考虑之后发现这么做反而比在构造函数中生成邻居节点更方便协调其它功能*/
DfaNode::DfaNode(dnode_id _id, vector<node_id> _node_ids):
    id(_id), node_ids(_node_ids), state(STATE::ORDINARY)
{
    /* 如果组成DfaNode中的node有接收状态的，则把自身的状态设置为接收状态
     * 并且标明接收的是哪个正则式*/
    size_t size = _node_ids.size();
    Node* pnode;
    for (size_t i = 0; i < size; ++i)
    {
        pnode = nodes[node_ids[i]];
        if (pnode->state == pnode->RECEIVE)
        {
            state = STATE::RECEIVE; // 将DFA节点设置为接受状态
            recv_types.push_back(pnode->recv_type); // 设置DFA接收哪些类型
        }
    }
}

vector<dnode_id> DfaNode::generate_neighbour()
{
    Node* pnode;
    Mark m;
    Mark empty_m(Char(string("")));
    vector<dnode_id> _dnodes;
    multimap<Mark, node_id>::iterator end;
    map<Mark, vector<dnode_id>> dnode_nei;
    map<Mark, vector<dnode_id>>::iterator dnode_nei_iter;


    /* 把dnode中所有node的边全部找出来合并在一起,找到所有node邻居
     * 注意，连接邻居的边不能是空*/
    size_t size = node_ids.size();
    for (size_t i = 0; i < size; ++i)
    {
        pnode = nodes[node_ids[i]];
        end = pnode->neighbour.end();
        for (auto iter = pnode->neighbour.begin(); iter != end; ++iter)
        {
            m = iter->first;
            dnode_nei_iter = node_neighbour.find(m);

            if (dnode_nei_iter == node_neighbour.end())
            {
                node_neighbour[m].push_back(iter->second);
                continue;
            }
            node_neighbour[m].push_back(iter->second);
        }
    }

    node_neighbour.erase(empty_m);

    /* dnode的一个Mark可以连接很多node，这些node在上一个循环中已经被找到，
     * 组成一个新dnode需要的不仅是上面找到的node，还有那些node通过empty可以
     * 到达的node，这两种node的组合被称为闭包，也是组成新dnode的所有node
     * 下面的操作是要找到剩下的node以组成新dnode*/
    vector<node_id> cluster;
    deque<node_id> q;
    for (auto iter = node_neighbour.begin(); iter != node_neighbour.end(); ++iter)
    {
        for (size_t i = 0; i < iter->second.size(); ++i)
            q.push_back(iter->second[i]);
        
        cluster = get_closure(q);
        iter->second = cluster;

        q.clear();
    }

    // print_node_neighbour();

    /* 根据上面找到的所有node邻居，创建本dnode的dnode邻居*/
    auto iter_end = node_neighbour.end();
    for (map<Mark, vector<node_id>>::iterator iter = node_neighbour.begin();
        iter != iter_end; ++iter)
    {
        dnode_id id = create_dnode(iter->second);
        _dnodes.push_back(id);// 压入返回的集合中
        dnode_neighbour[iter->first] = id;
    }

    return _dnodes;
}

void DfaNode::print_node_neighbour()
{
    vector<node_id> nei_nodes;
    Mark m;
    auto end = node_neighbour.end();
    for (auto iter = node_neighbour.begin(); iter != end; ++iter)
    {
        m = iter->first;
        nei_nodes = iter->second;
        for (size_t i = 0; i < nei_nodes.size(); ++i)
            cout << nei_nodes[i] << " ";
        
        cout << "Mark: ";
        m.printMark();
    }
}

void DfaNode::print_dnode_neighbour()
{
    auto end = dnode_neighbour.end();
    for (auto iter = dnode_neighbour.begin(); iter != end; ++iter)
    {
        cout << "nei: " << iter->second << " ";
        iter->first.printMark();
    }
}

void DfaNode::print_nodes_in_dnode()
{
    size_t size = node_ids.size();
    for (size_t i = 0; i < size; ++i)
        cout << node_ids[i] << " ";

    cout << endl;
}