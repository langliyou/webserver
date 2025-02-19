#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <random>
#include <fstream>
#include <condition_variable>
#include <type_traits>
#include <sstream>
#include <pthread.h>

#define SKIPLIST_FILEPATH "skiplist.txt"
#define DELIMITER ":~:"

using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::max;
using std::ifstream;
using std::ofstream;



template <typename T>  
T stringToT(std::string& s) {  
    // if(std::is_same<std::string, T>::value)
    // {
    //     return s;
    // }
    if(std::is_integral<T>::value)
    {
        return std::stoi(s);
    }
    if(std::is_floating_point<T>::value)
    {
        return std::stof(s);
    }
     
    cout << "Unsupported type for conversion from string" << endl;  
    return T();
} 

template <>  //模板特化
std::string stringToT(std::string& s) 
{
    string ret = s;
    return ret;
}  

template<typename K, typename V>
class Node
{
public:
    Node(K k, V v, int level) :
        key(k), value(v), node_level(level),
        forwards(level + 1, nullptr) {}
    Node(int level) :
        key(K()), value(V()), node_level(level),
        forwards(level + 1, nullptr) {}
    ~Node() = default;
    K get_key() const { return key; }
    V get_value() const { return value; }
    void set_value(V v) { value = v; }
public:
    int node_level;
    vector<Node*> forwards;
private:
    K key;
    V value;
};
   
template <typename K, typename V>
class SkipList {
public:
    SkipList(int);
    ~SkipList();
    int insert_element(const K key, const V value);
    bool search_element(K key) ;
    void delete_element(K key);
    void display_list() ;
    void dump_file();
    void load_file();

    void clear();           // 递归删除节点
    inline int size() ;        
private:
    int get_random_level();
    // bool is_valid_str(const string& line);
    bool get_node_param_from_str(const string& line, string* key, string* value, string* nodeLevel);
    pthread_rwlockattr_t _rwAttr;
    pthread_rwlock_t _rwlock;
    Node<K, V>* _header;         // 跳表的头节点
    int _max_level;              // 跳表允许的最大层数
    int _skip_list_level;        // 跳表当前的层数
    int _element_count;          // 跳表中组织的所有节点的数量
};
   
template <typename K, typename V>
SkipList<K, V>::SkipList(int max_level)
    : _max_level(max_level)
    , _skip_list_level(0)/*<=_max_level的依赖于跳表实际结点的值*/
    , _element_count(0)/*头结点不算*/
{
    _header = new Node<K, V>(max_level);
    pthread_rwlockattr_init(&_rwAttr);
    // pthread_rwlockattr_setkind_np(&rwAttr, PTHREAD_RWLOCK_PREFER_READER_NP);//设置读优先
    // pthread_rwlockattr_setkind_np(&_rwAttr, 2);//设置读写公平 PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP
    pthread_rwlock_init(&_rwlock, &_rwAttr);
    cout << "set rwlock with justice." << endl;
}

template <typename K, typename V>
SkipList<K, V>::~SkipList()
{
    clear();
    pthread_rwlock_destroy(&_rwlock);
    pthread_rwlockattr_destroy(&_rwAttr);
    delete _header;
}

template <typename K, typename V>
void SkipList<K, V>::clear()
{
    pthread_rwlock_wrlock(&_rwlock);
    while(_header->forwards[1])
    {
        auto nextNode = _header->forwards[1]->forwards[1];
        delete _header->forwards[1];
        _header->forwards[1] = nextNode;
    }
    for(int i = 2; i <= _skip_list_level; ++i)
    {
        _header->forwards[i] = nullptr;
    }
    _element_count = 0;
    _skip_list_level = 0;
    pthread_rwlock_unlock(&_rwlock);
}

template <typename K, typename V>
int SkipList<K, V>::size() 
{
    pthread_rwlock_rdlock(&_rwlock);
    int size = _element_count;
    pthread_rwlock_unlock(&_rwlock);
    return size;
}

template <typename K, typename V>
int SkipList<K, V>::get_random_level()
{
    int level = 1;
    while (rand() % 2) ++level;
    return (level < _max_level) ? level : _max_level;
}

template <typename K, typename V>
bool SkipList<K, V>::get_node_param_from_str(const string &line, string *key, string *value, string *nodeLevel)
{
    int dLen = string(DELIMITER).size();
    std::string::size_type idx1 = line.find_first_of(DELIMITER);
    std::string::size_type idx2 = line.find_first_of(DELIMITER, idx1 + dLen);
    if(!(idx1 != std::string::npos && idx2 != std::string::npos && idx1 != idx2))
    {
        return false;
    }
    *key = line.substr(0, idx1);
    *value = line.substr(idx1 + dLen, idx2 - idx1 - dLen);
    *nodeLevel = line.substr(idx2 + dLen);
    return true;
}

template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value)
{
    bool isExist = false;
    pthread_rwlock_wrlock(&_rwlock);
    Node<K, V>* cur = _header;
    vector<Node<K, V> *> preNodes(_max_level + 1, nullptr);
    for (int i = _skip_list_level; i >= 1; --i)
    {
        while (cur->forwards[i] && cur->forwards[i]->get_key() < key)
        {
            cur = cur->forwards[i];
        }
        //找到插入位置的前驱结点
        if(cur->forwards[i] && cur->forwards[i]->get_key() == key)
        {
            //存在键, 则更新值
            isExist = true;
            cur->forwards[i]->set_value(value);
        }
        else
        {
            preNodes[i] = cur;
        }
    }
    //不存在键, 则插入节点
    if(isExist == false)
    {
        int level = get_random_level();
        //结点需要复用, 只在堆区创建一次
        //超过目前最大层数的 前驱结点设为头结点
        for(int k = _skip_list_level + 1; k <= level; ++k)
        {
            preNodes[k] = _header;
        }
        _skip_list_level = max(_skip_list_level, level);
        ++_element_count;
        Node<K, V> *newNode = new Node<K, V>(key, value, level);
        for(int i = level; i >= 1; --i)
        {
            newNode->forwards[i] = preNodes[i]->forwards[i];
            preNodes[i]->forwards[i] = newNode;
        }
    }
    pthread_rwlock_unlock(&_rwlock);
    return isExist;
}
template<typename K, typename V>
bool SkipList<K, V>::search_element(K key) 
{
    pthread_rwlock_rdlock(&_rwlock);
    Node<K, V>* cur = _header;
    //从实际最高层开始虚拟头开始
    //键小于所查则往右跳, 否则向下到同节点密度更大的一层 最后都回到最底层
    for (int i = _skip_list_level; i >= 1; --i)
    {
        while (cur->forwards[i] && cur->forwards[i]->get_key() < key)
        {
            cur = cur->forwards[i];
        }
    }
    //最后一定在最底层
    cur = cur->forwards[1];
    if (cur && cur->get_key() == key)
    {
        pthread_rwlock_unlock(&_rwlock);
        return true;
    }
    pthread_rwlock_unlock(&_rwlock);
    return false;
}
 
template<typename K, typename V>
void SkipList<K, V>::delete_element(K key)
{
    Node<K, V>* delNode = nullptr;
    pthread_rwlock_wrlock(&_rwlock);
    Node<K, V>* cur = _header;
    for (int i = _skip_list_level; i >= 1; --i)
    {
        while (cur->forwards[i] && cur->forwards[i]->get_key() < key)
        {
            cur = cur->forwards[i];
        }
        if(cur->forwards[i] && cur->forwards[i]->get_key() == key)
        {
            if(delNode == nullptr)
            {
                delNode = cur->forwards[i];
            }
            cur->forwards[i] = cur->forwards[i]->forwards[i];
        }
    }
    while(_skip_list_level > 0 && _header->forwards[_skip_list_level] == nullptr)
    {
        --_skip_list_level;
    }
    if(delNode)
    {
        delete delNode;
        --_element_count;
    }
    pthread_rwlock_unlock(&_rwlock);
}
template<typename K, typename V>
void SkipList<K, V>::display_list() 
{
    pthread_rwlock_rdlock(&_rwlock);
    for(int i = _skip_list_level; i >= 1; --i)
    {
        Node<K, V>* cur = _header->forwards[i];
        cout << "Level " << i << ": ";
        while(cur)
        {
            cout << cur->get_key() << ":" << cur->get_value() << "; ";
            cur = cur->forwards[i];
        }
        cout << endl;
    }
    pthread_rwlock_unlock(&_rwlock);
}

template <typename K, typename V>
void SkipList<K, V>::dump_file()
{
    ofstream ofs(SKIPLIST_FILEPATH, std::ios::trunc);
    pthread_rwlock_rdlock(&_rwlock);
    Node<K, V>* cur = _header->forwards[1];
    while(cur)
    {
        ofs << cur->get_key() << DELIMITER 
            << cur->get_value() << DELIMITER
            << cur->node_level << endl;
        cur = cur->forwards[1];
    }
    pthread_rwlock_unlock(&_rwlock);
    ofs.flush();
    ofs.close();
}

template <typename K, typename V>
void SkipList<K, V>::load_file()
{
    K key;
    V val;
    int level;
    ifstream ifs(SKIPLIST_FILEPATH, std::ios::in);
    pthread_rwlock_wrlock(&_rwlock);
    vector<Node<K, V>*> preNodes(_max_level + 1, _header);
    if(! ifs.is_open())
    { 
        cout << "读取文件打开失败" << endl; 
        return; 
    }
    string buf;//动态分配内存, 自动管理缓冲区大小
    while(getline(ifs, buf))
    {
        string* key = new string;
        string* value = new string;
        string* level = new string;
        if(! get_node_param_from_str(buf, key, value, level))
        {
            delete key;
            delete value;
            delete level;
            cout << "line load fail.\n" << endl;
            continue;
        }
        K* k = new K();
        V* v = new V();
        *k = stringToT<K>(*key);
        if(k == nullptr) continue;
        *v = stringToT<V>(*value);
        if(v == nullptr) continue;
        int nodeLevel = std::stoi(*level);
        Node<K, V>* newNode = new Node<K, V>(*k, *v, nodeLevel);
        delete key;
        delete value;
        delete level;
        delete k;
        delete v;
        for(int i = 1; i <= nodeLevel; ++i)
        {
            preNodes[i]->forwards[i] = newNode;
            preNodes[i] = preNodes[i]->forwards[i];
        }
        _skip_list_level = max(_skip_list_level, nodeLevel);
        ++_element_count;
    }
    pthread_rwlock_unlock(&_rwlock);
    ifs.close();
}
    
