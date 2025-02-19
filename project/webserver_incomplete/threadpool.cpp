#include <queue>  
#include "concurrent.h"  

#define THREAD_CAP_DEFAULT 8
#define TASK_CAP_DEFAULT 1024
using std::queue;

enum class PoolMode {  
    FIXED_MODE,  
    CACHED_MODE,  
};  

// 接口声明部分
template<typename T>  
class Threadpool 
{  
private:  
    int m_maxThreadNum;  
    int m_initThreadNum;  
    queue<T*> m_taskQueue;  
  
    int m_curThreadSize;  
    int m_idleThreadSize;  
    int m_taskUpperBound;  
    int m_curTaskSize;  
    PoolMode m_poolMode;  
    bool m_isPoolRunning;  
    // ITC
    Sem m_semThread;  
    Cond m_condEmpty;  
    Cond m_condFull;  
    Mutex m_mutex;
  
public:  
    Threadpool(int maxThreadNum = THREAD_CAP_DEFAULT
            , int initThreadNum = 0
            , int taskUpperBound = TASK_CAP_DEFAULT
            , PoolMode poolMode = PoolMode::FIXED_MODE);
  
    ~Threadpool();  
  
    void stop();  
  
    void appendTask(T* task);  
  
private:  
    void startThreads(int num);  
  
    void* threadFunction(void* arg);  
};

// 实现部分
template<typename T>
Threadpool<T>::Threadpool(int maxThreadNum, int initThreadNum, int taskUpperBound, PoolMode poolMode)
    : m_maxThreadNum(maxThreadNum)
    , m_initThreadNum(initThreadNum)
    , m_taskUpperBound(taskUpperBound)
    , m_poolMode(poolMode)
    , m_curThreadSize(0)
    , m_idleThreadSize(0)
    , m_curTaskSize(0)
    , m_isPoolRunning(false)
    // ITC  
    , m_condEmpty()    // 初始时没有空闲线程  
    , m_condFull()   // 初始时没有任务  
    , m_semThread(0)    // 初始时没有活跃的线程  
    , m_mutex()
{  
    if (m_initThreadNum > 0) {  
        startThreads(m_initThreadNum);  
    }  
}

template<typename T>
Threadpool<T>::~Threadpool() {  
    stop();  
  
    // 等待所有线程结束  
    m_semThread.wait();  
  
    // 清理任务队列（可选，取决于任务对象的生命周期管理）  
    while (!m_taskQueue.empty()) {  
        delete m_taskQueue.front();  
        m_taskQueue.pop();  
    }  
}

template<typename T>
void Threadpool<T>::stop() 
{  
    m_isPoolRunning = false;  
}

template<typename T>
void Threadpool<T>::appendTask(T* task) 
{  
    m_mutex.lock();
    if(m_curTaskSize == m_taskUpperBound){
        m_condFull.wait(m_mutex.get()); //释放锁, 进入wait态
    }
    m_taskQueue.push(task);  
    m_curTaskSize++;  
    m_condEmpty.signal(m_mutex.get()); //唤醒等待任务的线程
    m_mutex.unlock();
}

template<typename T>
void Threadpool<T>::startThreads(int num) {  
    int startNum = std::min(num, m_maxThreadNum);
    for (int i = 0; i < startNum; ++i) {  
        pthread_create(NULL, NULL, threadFunction, NULL); 
        m_curThreadSize++;  
        m_idleThreadSize++;  
    }  
}

template<typename T>
void* Threadpool<T>::threadFunction(void* arg) 
{  
    while (true) {  
        m_mutex.lock(); 
        if (!m_isPoolRunning && m_taskQueue.empty()) {  
            // 如果线程池不再运行且没有任务，则线程退出  
            m_mutex.unlock();
            break;  
        }  
        m_condEmpty.wait(m_mutex.get());  // 等待任务  
        T* task;
        if (!m_taskQueue.empty()) {  
            task = m_taskQueue.front();  
            m_taskQueue.pop();  
            m_curTaskSize--;  
            m_condFull.signal(m_mutex.get());   



        }  
        m_mutex.unlock();
        // 执行任务（注意：这里应该添加异常处理）  
        // 假设任务有一个名为execute的成员函数  
        task->execute();  
        // 任务完成后，释放资源（如果需要）  
        delete task;  // 注意：这里假设任务对象是用new分配的  
        if (m_poolMode == PoolMode::CACHED_MODE && m_curThreadSize > m_initThreadNum) {  
            // 如果在缓存模式下且线程数超过初始线程数，则线程可以退出  
            break;  
        }  
    }  

    // 线程退出前，减少活跃线程数  
    {  
        m_mutex.lock(); 
        m_curThreadSize--;  
        m_idleThreadSize--;  
        m_mutex.unlock();
        if (m_curThreadSize == 0) {  
            m_semThread.post();  // 通知所有线程已退出  
        }  
    }  
    pthread_exit(NULL);
}
  
  
// 注意：上面的 threadFunction 是一个伪代码示例，  
// 实际使用时你需要根据你的线程库（如 std::thread）来创建和管理线程。  
// 你可能还需要一个线程安全的机制来管理线程的创建和销毁，  
// 特别是当需要动态地增减线程数时。  
  
// ...（其他成员函数和细节）  
  
// 示例：你可能还需要一个添加任务的函数，它内部会调用 submit  
// void addTask(T* task) {  
//     submit(task);  
// }  
  
// 注意：这个类还缺少很多重要的细节，比如  
// - 如何优雅地停止线程（目前只是简单地将 m_isPoolRunning 设置为 false）  
// - 如何处理任务执行中的异常  
// - 线程池的动态扩展和收缩（在 CACHED_MODE 下）  
// - 以及其他可能的优化和错误处理  
  
// 这些都需要根据你的具体需求来设计和实现。