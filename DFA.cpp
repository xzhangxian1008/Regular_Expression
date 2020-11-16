#include "DFA.hpp"
#include "utility.hpp"

/* 构造函数的任务是创建一个新的起始节点
 * 这个新节点和所有输入的NFA以""相连*/
Dfa::Dfa(vector<Nr*> _pnrs): pnrs(_pnrs)
{
    vector<node_id> closure;
    vector<dnode_id> dnode_nei;// 保存dnode生成的邻居dnode id
    deque<node_id> node_ids;// 生成dnode所用
    deque<dnode_id> dnode_ids;// 存放未被遍历的dnode
    dnode_id dnode;
    DfaNode* pdnode;

    start_id = create_node(Node::START);

    size_t size = pnrs.size();
    for (size_t i = 0; i < size; ++i)    
    {
        Nr* pnr = pnrs[i];
        node_id pnr_start_id = pnr->start_id;

        nodes[pnr_start_id]->state = Node::ORDINARY;
        nodes[start_id]->add_neighbour(Mark(Char(string(""))), pnr_start_id);
    }

    /* 上面是把输入的Nr合并为一个NFA
     *
     * 下面进行NFA转换为DFA的操作*/
    node_ids.push_back(start_id);
    closure = get_closure(node_ids); // 得到组成DFA起始节点的所有node
    dnode = create_dnode(closure); // 创建DFA的起始节点
    start_did = dnode;
    dnode_ids.push_back(dnode);
    while (dnode_ids.size() > 0)
    {
        dnode = dnode_ids.front(); // 取出下一个遍历的dnode节点
        pdnode = pdnodes[dnode];
        dnode_ids.pop_front();

        dnode_nei = pdnode->generate_neighbour();
        
        size_t size = dnode_nei.size();
        for (size_t i = 0; i < size; ++i)
            dnode_ids.push_back(dnode_nei[i]); // 把邻居节点压入队列做后续遍历
    }
}

// 使用广度搜索打印
void Dfa::printNFA()
{
    // print("print NFA");
    vector<bool> flags(current_id, false);
    deque<node_id> q;
    node_id node;
    Node* pnode;

    q.push_back(start_id);
    flags[start_id] = true;
    while (q.size() > 0)
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
            if (flags[iter->second])
                continue;
            
            q.push_back(iter->second);
            flags[iter->second] = true;
        }
    }
}

/* 对DFA进行广度打印*/
void Dfa::printDFA()
{
    deque<dnode_id> d_dnodes;
    DfaNode* pdnode;
    dnode_id dnode;
    dnode_id nei_dnode; // 标识被遍历节点的邻居节点

    d_dnodes.push_back(start_did);
    while (d_dnodes.size() > 0)
    {
        dnode = d_dnodes.front();
        d_dnodes.pop_front();
        pdnode = pdnodes[dnode];

        auto end = pdnode->dnode_neighbour.end();
        for (auto iter = pdnode->dnode_neighbour.begin(); iter != end; ++iter)
        {
            nei_dnode = iter->second;
            d_dnodes.push_back(nei_dnode); // 邻居dnode压入队列准备做遍历

            cout << dnode << "->" << nei_dnode << " ";
            iter->first.printMark();

            cout << "recv_types: ";
            for (size_t i = 0; i < pdnodes[dnode]->recv_types.size(); ++i)
                cout << pdnodes[dnode]->recv_types[i] << " ";
            
            cout << endl << "recv_types: ";
            for (size_t i = 0; i < pdnodes[nei_dnode]->recv_types.size(); ++i)
                cout << pdnodes[nei_dnode]->recv_types[i] << " ";
            
            cout << endl;
        }
    }
}