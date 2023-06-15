/**
 * skiplist.h  基于Redis的调表思想实现的内存数据库主要实现代码 
 * 
 * author:GXH
 * 
 * 个人理解：
 *      跳表底层是为了一个多级链表，第 0 层是链表的所有数据，但是由于链表的访问的时间复杂度为O(n)，所以
 * 为了调高链表的访问速度，提取了一个多层级链表，一级一级向下查找，数据的访问速度可以达到O(logn),当数据量足够的情况下，可以
 * 极大的提高系统性能
 * level 4     1+                                                      100
                 |
                 |
level 3         1+-------->10+------------------>50+           70       100
                                                   |
                                                   |
level 2         1          10         30         50|           70       100
                                                   |
                                                   |
level 1         1    4     10         30         50|           70       100
                                                   |
                                                   |
level 0         1    4   9 10         30   40    50+-->60      70       100

*   说到底，跳表只维护以第0层这一条链表，只不过在每个节点中维护了一个层数，即当前节点可以在多少层
    可以遍历到 还有一个指针数组，这个指针数组保留着这个节点可以到达的层级中，在那一层的下一个节点的位置
*   所以，抽象来看就是一个多层的链表，越往上层，节点个数越少
*
*/
#ifndef SKIPLIST_
#define SKIPLIST_
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>

#define STORE_FILE "store/dumpFile"//数据落盘的文件路径

std::mutex mtx;   //对于共享数据的互斥锁
std::string delimiter = ":";//在读取数据和存储数据时 对于KV的分隔符

/* 实现节点的模板类 Node*/
/**
 * 跳表中维护的节点的数据结构
 * 可以认为，这是一条单向链表，直指向下一个节点的位置
 * 拥有功能，数据部分，K，V，level（当前节点可以在level个层中遍历到）
 * forward 当前节点在每一层指向下一个节点的地址 然后组成一个指针数组，数组长度为 level + 1
*/
template<typename K,typename V>
class Node
{
public:
    Node(){}

    Node(K k,V v,int);

    ~Node();

    K get_key() const;

    V get_value() const;

    void set_value(V);

    /* 保存指向不同级别下一个节点的指针的线性数组 */
    Node<K,V> **forward;

    int node_level;
private:
    K key;
    V value;
};

template<typename K,typename V>
Node<K,V>::Node(const K k,const V v,int level)
{
    this -> key = k;
    this -> value = v;
    this -> node_level = level;

    this -> forward = new Node<K,V>*[level+1];

    memset(this->forward,0,sizeof(Node<K,V>*) * (level + 1));

}

template<typename K,typename V>
Node<K,V>::~Node()
{
    delete []forward;
}

template<typename K, typename V> 
K Node<K, V>::get_key() const {
    return key;
};

template<typename K, typename V> 
V Node<K, V>::get_value() const {
    return value;
};
template<typename K, typename V> 
void Node<K, V>::set_value(V value) {
    this->value=value;
};

/* 跳表的模板类 */
/**
 * 跳表，维护的是 n 个Node节点，由于每个node节点除了在第 0 层是顺序的指向下一个节点位置，在有些
 * 节点在更高的层级中还有指向下一个节点位置的指针，导致形成了一个多层级的链表
*/
template <typename K,typename V>
class SkipList
{

public:
    SkipList(int);
    ~SkipList();
    int get_random_level();
    Node<K,V>* create_node(K,V,int);

    int insert_element(K,V);
    void display_list();
    bool search_element(K,V&);
    bool delete_element(K);
    void dump_file();
    void load_file();
    int size();

private:
    void get_key_value_from_string(const std::string& str,std::string* key,std::string* value);
    bool is_valid_string(const std::string& str);
private:
    /*跳表的最大层数*/
    int m_max_level;
    /*当前跳表的层数*/
    int m_cur_level;
    /*头节点指针*/
    Node<K,V>* m_header;
    /*文件操作*/
    std::ofstream m_file_writer;
    std::ifstream m_file_reader;

    /*当前跳表的元素数量*/
    int m_element_count;
};

/*创建skiplist*/
template<typename K,typename V>
SkipList<K,V>::SkipList(int max_level)
{
    //初始化 整个skiplist的所能达到的最高层级（但不一定有这么多层）
    this->m_max_level = max_level;
    // 当前已有的数据节点构成的skiplist的最高层级
    this->m_cur_level = 0;
    //节点数
    this->m_element_count = 0;

    //创建头节点 并且初始化 key和value 为null
    K k;
    V v;

    //头节点 头节点默认初始化为max_level层，在每一层都维护了一个指向下一个可以访问节点的指针
    //保证无论这个跳表有多少层 都可以从 这个头节点开始遍历
    this -> m_header = new Node<K,V>(k,v,m_max_level);
}

/*销毁skiplist*/
template<typename K,typename V>
SkipList<K,V>::~SkipList()
{
    if(m_file_writer.is_open())
    {
        m_file_writer.close();
    }
    if(m_file_reader.is_open())
    {
        m_file_reader.close();
    }

    Node<K,V>* tmp = m_header;
    while (tmp)
    {
       m_header = m_header->forward[0];
       delete tmp;
       tmp = m_header;
    }
    
    m_header = NULL;
}

/*create new node*/
template<typename K,typename V>
Node<K,V>* SkipList<K,V>::create_node(const K k,const V v,int level)
{
    Node<K,V>* node = new Node<K,V>(k,v,level);
    return node;
}

/*向跳表中插入给定的 key 和 value*/
//RETURN VALUE
// 1 : element exists already
// 0 : insert successfully

/* 
                           +------------+
                           |  insert 50 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |                      insert +----+
level 3         1+-------->10+---------------> | 50 |          70       100
                                               |    |
                                               |    |
level 2         1          10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 1         1    4     10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 0         1    4   9 10         30   40  | 50 |  60      70       100
          head                                   +----+

*/
/**
 * 理解：
 *      维护一个update的指针数组，这个指针数组是即将要插入的节点如果在第i层出现，那么它的前一个
 * 节点的位置
 *      如果插入的节点key存在，那么只需要将value值改变即可，element_size不需要改变
 *      如果插入的节点key和value都存在，返回节点已存在
 *      如果要插入的key和value都不存在，那么需要随机生成一个level值，表示这个节点可以记录level层
 *  下一个节点的值，然后将update[i]-forward[i] 的值赋给当前节点的forward[i] 再将update[i]-forward[i]
 * 指向当前节点的forward[i] 相当于是在 update[i]（前一个节点）和 update[i]-forward[i]（后一个节点）
 * 中间插入了一个forward[i](当前节点)，所以当前节点所能达到的层的前后指针都需要改变
*/
template<typename K,typename V>
int SkipList<K,V>::insert_element(const K key,const V value)
{
    mtx.lock();
    Node<K,V>* current = this-> m_header;

    //创建一个 update数组 并初始化,update数组中每一个Node*节点是第i层将要添加的节点的前一个节点 
    // update是存放node->forward[i]的数组，该node->forward[i]将在稍后操作
    Node<K,V>* update[m_max_level + 1];
    memset(update,0,sizeof(Node<K,V>*) * (m_max_level + 1));

    //从跳表的最高层开始
    for(int i = m_cur_level;i >= 0;--i)
    {
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key)
        {
            current = current->forward[i];
        }
        update[i] = current;
    }

    //到达第 0 level 并且forward的指针指向右边的节点，这就是要插入key的地方
    current = current->forward[0];
    //如果current 节点的key 等于我们插入的节点 插入失败
    if(current != NULL && current -> get_key() == key)
    {
        if(current->get_value() != value)
        {
            current->set_value(value);
            #ifdef DEBUG
            std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
            #endif
            mtx.unlock();
            return 1;
        }
        #ifdef DEBUG
        std::cout << "key: " << key << ",exists" << std::endl;
        #endif
        mtx.unlock();
        return 1;
    }

    //如果current等于NULL，说明我们已经到了这一层的末尾
    //如果current不等于key，说明我们可以把节点出入到 update[0] 和current节点之间
    if(current == NULL || current->get_key() != key)
    {
        // 为这个节点生成一个随机层数
        int random_level = get_random_level();
        //如果random_level 大于 skiplist的当前层数，
        if(random_level > m_cur_level)
        {
            for(int i = m_cur_level + 1;i < random_level + 1;++i)
            {
                //在m_cur_level + 1 ~ random_level之间，当前节点的前一个节点是头节点
                update[i] = m_header;
            }
            m_cur_level = random_level;
        }

        //创建random层的新节点
        Node<K,V>* inserted_node = create_node(key,value,random_level);

        //插入节点
        for(int i = 0; i <= random_level;++i)
        {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        #ifdef DEBUG
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        #endif
        m_element_count ++;
    }
    mtx.unlock();
    return 0;
}

/**
 * 为当前节点随机生成一个level层级
*/
template<typename K,typename V>
int SkipList<K,V>::get_random_level()
{
    int k = 1;
    while(rand() % 2)
    {
        k++;
    }
    k = (k < m_max_level) ? k : m_max_level;
    return k;
}

/*display skiplist*/
/**
 * 遍历数据 直接从头节点开始 遍历forward[0] 第0层的数据即可
*/
template<typename K,typename V>
void SkipList<K,V>::display_list()
{
    std::cout << "\n*****Skip List****\n";
    for(int i = 0; i <= m_cur_level;++i)
    {
        Node<K,V>* node = this->m_header->forward[i];
        std::cout << "Level " << i << ": ";
        while(node != NULL)
        {
            std::cout << node->get_key() << ":" << node->get_value() << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

/*获取当前skip list的节点个数*/
template<typename K,typename V>
int SkipList<K,V>::size()
{
    return this->m_element_count;
}

// 从skipList中删除节点
/**
 * 和插入节点的思路类似 维护一个要删除节点在每一层的前一个节点 然后将前一个节点update[i]的下一个节点
 * update[i]->forward[i]指向要删除节点的下一个节点current[i]->forward[i]
*/
template<typename K,typename V>
bool SkipList<K,V>::delete_element(K key)
{
    mtx.lock();
    Node<K,V>* current = this->m_header;
    Node<K,V>* update[m_max_level+1];
    memset(update,0,sizeof(Node<K,V>*) * (m_max_level + 1));

    /*从当前skiplist的最高层出发*/
    for(int i = m_cur_level;i >= 0;--i)
    {
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key)
        {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    if(current != NULL && current->get_key() == key)
    {
        for(int i = 0; i <= m_cur_level;++i)
        {
            if(update[i]->forward[i] != current)
            {
                break;
            }
            update[i]->forward[i] = current->forward[i];
        }

        //删除没有节点的层
        while(m_cur_level > 0 && m_header->forward[m_cur_level] == 0)
        {
            --m_cur_level;
        }

        delete current;
        std::cout << "Successfully deleted key " << key << std::endl;
        --m_element_count;
    }else{
        mtx.unlock();
        std::cout << "fail deleted key ,not exists" << key << std::endl;
    }

    mtx.unlock();
    return true;

}

/*再skip list中查找元素*/
/*
                           +------------+
                           |  select 60 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |
level 3         1+-------->10+------------------>50+           70       100
                                                   |
                                                   |
level 2         1          10         30         50|           70       100
                                                   |
                                                   |
level 1         1    4     10         30         50|           70       100
                                                   |
                                                   |
level 0         1    4   9 10         30   40    50+-->60      70       100
*/
template<typename K,typename V>
bool SkipList<K,V>::search_element(K key,V& v)
{
    #ifdef DEBUG
    // std::cout << "search_element--------------" << std::endl;
    #endif
    Node<K,V>* current = m_header;

    /*从当前最高层出发*/
    for(int i = m_cur_level;i >= 0; --i)
    {
        while(current->forward[i] && current->forward[i]->get_key() < key)
        {
            current = current->forward[i];
        }
    }

    //到达第0层 
    current = current->forward[0];

    if(current && current->get_key() == key)
    {
        #ifdef DEBUG
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        #endif
        v = current->get_value();
        return true;
    }
     #ifdef DEBUG
    std::cout << "Not Found Key:" << key << std::endl;
    #endif
    v = "nil";
    return false;
}

/*将内存中的数据存储到文件中*/
/**
 * 和遍历元素是一个思路：只遍历第0层的数据 然后一个节点的KV写入文件的一行
*/
template<typename K,typename V>
void SkipList<K,V>::dump_file()
{
    std::cout << "dump_file-------------" << std::endl;
    m_file_writer.open(STORE_FILE);
    Node<K,V>* node = this->m_header->forward[0];
    while(node != NULL)
    {
        m_file_writer << node->get_key() << ":" << node->get_value() << "\n";
        std::cout << node->get_key() << ":" << node->get_value() << "\n";
        node = node->forward[0];
    }

    m_file_writer.flush();
    m_file_writer.close();

}

/*加载文件*/
/**
 * 一行一行读取文件，然后根据写入进去的KV分隔符 delimiter切割数据 然后重新一个一个insert进skiplist中
*/
template<typename K,typename V>
void SkipList<K,V>::load_file()
{
    std::cout << "load_file--------------" << std::endl;
    m_file_reader.open(STORE_FILE);
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();
    while(getline(m_file_reader,line))
    {
        get_key_value_from_string(line,key,value);
        if(key->empty() || value->empty()) 
            continue;

        insert_element(*key,*value);
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }

    m_file_reader.close();
}

template<typename K,typename V>
void SkipList<K,V>::get_key_value_from_string(const std::string& str,std::string* key,std::string* value)
{
    if(!is_valid_string(str))
    {
        return ;
    }

    *key = str.substr(0,str.find(delimiter));
    *value = str.substr(str.find(delimiter)+1,str.length());
}

template<typename K,typename V>
bool SkipList<K,V>::is_valid_string(const std::string& str)
{
    if(str.empty())
    {
        return false;
    }
    if(str.find(delimiter) == std::string::npos){
        return false;
    }

    return true;
}
#endif
