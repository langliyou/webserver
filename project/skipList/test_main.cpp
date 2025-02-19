#include "skipList.h"

#include <chrono>
#include <time.h>
#include <string.h>
#include <iostream>
#define THREAD_NUM 1
#define NODE_NUM 100000
#define K int
#define V string
SkipList<K, V> sl(5);

void testCin()
{
    int maxLevel = 6;
    auto sl = new SkipList<K, V>(maxLevel);
    int N, Q, M;
    cin >> N >> Q >> M;
    while (N--)
    {
        K k;
        V v;
        cin >> k >> v;
        if (sl->insert_element(k, v) == 0)
        {
            cout << "Insert Success" << endl;
        }
        else
        {
            cout << "Insert Failed" << endl;
        }
        sl->display_list();
    }
    while(Q--)
    {
        K k;
        cin >> k;
        sl->delete_element(k);
        sl->display_list();
    }
    while (M--)
    {
        K k;
        cin >> k;
        if (sl->search_element(k))
        {
            cout << "Search Success" << endl;
        }
        else
        {
            cout << "Search Failed" << endl;
        }
        sl->display_list();
    }
    sl->dump_file();
    sl->clear();
    sl->display_list();
    sl->load_file();
    sl->display_list();
    getchar();
}


void* insertToList(void *arg)
{
    int n = NODE_NUM / THREAD_NUM;
    for(int i = 0; i < n; ++i)
    {
        sl.insert_element(rand() % NODE_NUM, "aa");
    }
    pthread_exit(NULL);
}

void* searchList(void *arg)
{
    int n = NODE_NUM / THREAD_NUM;
    for(int i = 0; i < n; ++i)
    {
        sl.search_element(rand() % NODE_NUM);
    }
    pthread_exit(NULL);
}

void pressureTest()
{
    srand (time(NULL));  
    pthread_t tid[THREAD_NUM];
    
    //插入元素
    auto beginTime = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < THREAD_NUM; ++i)
    {
        int ret;
        if((ret = pthread_create(&tid[i], NULL, insertToList, NULL)) != 0)
        {
            // cout << strerror(ret) << endl;
        }
    }
    for(const auto& t: tid)
    {
        pthread_join(t, NULL);
    }
    auto finishTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedTime = finishTime - beginTime;

    cout << "elapsed "<< elapsedTime.count() << " seconds" << endl;
}
int main()
{
    // testCin();
    pressureTest();
    return 0;
}