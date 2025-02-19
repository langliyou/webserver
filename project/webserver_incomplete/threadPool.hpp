#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory> //智能指针
#include <condition_variable>
#include <mutex>
#include <functional>//bind
#include <iostream>
#include <unordered_map>
#include <future>//<=>自定义Result
#include <thread>
#include <utility>

using  uint = unsigned int;
using ULong = unsigned long long;
const uint TASK_CAP_DEFAULT = 1024;
const uint THREAD_CAP_DEFAULT = 8;
const uint THREAD_MAX_IDLETIME = 5;//单位: 秒

enum class PoolMode {
	FIXED_MODE,
	CASHED_MODE,
};

class Thread {
	using ThreadFunc = std::function<void(uint)>;//起别名
public:
	/*Thread() :
		id_(idBase_++)
	{}*/
	Thread(ThreadFunc threadFunc) :
		func_(threadFunc),
		id_(idBase_++)
	{}
	~Thread() = default;
	void start() 
	{
		std::thread t(this->func_, this->id_);//内部又创建一个标准库的线程，执行func_
		t.detach();//使得t 与func_ 分离，这样不至于说t析构导致func_也析构，缩短其生命周期
	}
	uint getId() const { return id_; }

private:
	//内含一个绑定了线程池this指针的（确保可以直接用线程池的所有成员）、线程池指派的函数（在线程池类内声明、定义）
	static uint idBase_;
	ThreadFunc func_;
	uint id_;
};

uint Thread::idBase_ = 0;

class ThreadPool {
	using Task = std::function<void()>;//和绑定bind一起用，这样就能实际上把参数和函数也一起存进去
public:
	using ThreadFunc = std::function<void(uint)>;//起别名
	
	ThreadPool();
	~ThreadPool();
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	//用户给定初始线程个数，减少set接口; 默认为系统内核数
	void start(int initThreadNum = std::thread::hardware_concurrency());

	template<typename Func, typename... Args>
	auto submitTask(Func&& f, Args&&... args) -> std::future<decltype(f(args...))>;

	void setPoolMode(PoolMode mode);
	void setTaskUpperBound(uint cap);
	void setThreadUpperBound(uint cap);//cashed、fixed都能设置线程上限
private:
	void threadFunc(uint threadId);
	void addWhenTram();
	uint addThread();// 返回新建的线程的id
	
	std::unordered_map<uint, std::unique_ptr<Thread>> idThreads_;
	std::atomic_uint curThreadSize_;//这个可以写成uint
	std::atomic_uint idleThreadSize_;
	uint threadUpperBound_;
	uint initThreadSize_;

	uint taskUpperBound_;
	std::atomic_uint curTaskSize_;//轻量的整型+互斥锁
	std::queue<Task> taskQue_;

	std::mutex taskQueMutex_;//与条件变量配套使用
	std::condition_variable notFull_;
	std::condition_variable notEmpty_;

	PoolMode poolMode_;
	std::atomic_bool isPoolRunning_;
	std::condition_variable exitCond_;
};
#endif

//-------------------------------------------------------------threadPool实现----------------------------------------------------------------------------------------
ThreadPool::ThreadPool() :
	threadUpperBound_(THREAD_CAP_DEFAULT),
	initThreadSize_(0),
	curThreadSize_(0),
	idleThreadSize_(0),
	taskUpperBound_(TASK_CAP_DEFAULT),
	curTaskSize_(0),
	poolMode_(PoolMode::FIXED_MODE),
	isPoolRunning_(false)
{
}
ThreadPool::~ThreadPool()
{
	isPoolRunning_ = false;
	//notEmpty_.notify_all();//条件没变，但需唤醒线程，可能造成死锁
	std::unique_lock<std::mutex> lock(taskQueMutex_);
	notEmpty_.notify_all();
	exitCond_.wait(lock, [&]()->bool {return idThreads_.size() == 0; });//线程没有全部结束 线程池对象就不析构
	std::cout << "tid=" << std::this_thread::get_id() << "\t" << "ThreadPool destroyed." << std::endl;
}
void ThreadPool::start(int initThreadNum) {
	isPoolRunning_ = true;
	initThreadSize_ = initThreadNum;
	//先创建，再调用，确保地位相同
	for (int i = 0; i < initThreadNum; ++i) {
		//原地构造；绑定
		this->addThread();
	}

	for (int i = 0; i < initThreadNum; ++i) {
		idThreads_[i]->start();
		//std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}
inline void ThreadPool::setPoolMode(PoolMode mode)
{
	if (isPoolRunning_ == false)
		this->poolMode_ = mode;
}
inline void ThreadPool::setTaskUpperBound(uint cap)
{
	if (isPoolRunning_ == false)
		this->taskUpperBound_ = cap;
}
inline void ThreadPool::setThreadUpperBound(uint cap)
{
	if (isPoolRunning_ == false)
		this->threadUpperBound_ = cap;
}
inline void ThreadPool::threadFunc(uint threadId)
{
	auto lastTime = std::chrono::high_resolution_clock().now();

	while (true) {//线程会反复准备执行任务，属于是生产队的驴
		Task task;
		{
			// 先获取锁
			std::unique_lock<std::mutex> lock(this->taskQueMutex_);
			//std::cout << "tid=" << std::this_thread::get_id() << "\t" << "Try to get task... " << std::endl;


			// 等待notEmpty条件
			while (curTaskSize_ == 0) {
				if (isPoolRunning_) {
					if (poolMode_ == PoolMode::CASHED_MODE) {
						//考虑线程大于初始 等太久了就回收
						//上一次执行时间 - 目前时间 > 60s则回收一个线程
						//每次等待一秒则判断是否时间 > 60s
						if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
							auto now = std::chrono::high_resolution_clock().now();
							auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);//强转为秒

							//std::unique_lock<std::mutex> lock(this->threadRecallMtx_);
							if (dur.count() > THREAD_MAX_IDLETIME && curThreadSize_ > initThreadSize_) {
								//判断要在里面，否则超额回收了；回收当前线程对象   而不是this_thread
								--curThreadSize_;
								--idleThreadSize_;
								idThreads_.erase(threadId);//唯一id，不会线程冲突
								std::cout << "tid=" << std::this_thread::get_id() << "\t" << "Recall one Thread." << std::endl;
								return;
							}
						}

					}
					else {
						notEmpty_.wait(lock);
					}
				}
				else {//没有任务+线程池对象析构 =》回收线程
					idThreads_.erase(threadId);//显式删除
					std::cout << "tid=" << std::this_thread::get_id() << "\t" << "Thread" << " " << threadId << " exit." << std::endl;
					exitCond_.notify_all();//配套V操作：我走了 我告诉你，你条件满足则执行否则继续等
					return;
				}

			}


			--idleThreadSize_;//准备取任务 线程即不空闲
			//std::cout << "tid=" << std::this_thread::get_id() << "\t" << "Got task. " << std::endl;

			// 从任务队列取一个任务出来
			task = std::move(taskQue_.front());
			taskQue_.pop();//要弹出来！！不然会反复拿到空任务
			--curTaskSize_;

			//V操作，有P就要有V
			if (taskQue_.size() > 0) notEmpty_.notify_all(); //??????????

			notFull_.notify_all();
		}
		//自己造了作用域，出作用域，即释放锁；执行任务时手里不拿锁

		//// 当前线程负责执行这个任务
		//if (task == nullptr) {
		//	std::cerr << "Null task error." << std::endl;
		//	continue;
		//}
		task();//在这执行任务！！
		++idleThreadSize_;//执行完 线程又空闲了
		lastTime = std::chrono::high_resolution_clock().now();//更新时间点
	}
}
inline void ThreadPool::addWhenTram()
{
	if (poolMode_ == PoolMode::CASHED_MODE
		&& curThreadSize_ < threadUpperBound_
		&& idleThreadSize_ < curTaskSize_)
	{
		std::cout << "tid=" << std::this_thread::get_id() << "\t"
			<< "Add new thread, since curTaskSize_= " << curTaskSize_
			<< "curThreadSize_= " << idThreads_.size() << std::endl;
		uint newId = this->addThread();
		idThreads_[newId]->start();
	}
}
inline uint ThreadPool::addThread()
{
	if (curThreadSize_ == threadUpperBound_) return 0;//达到上限 不予增加
	using UPtr = std::unique_ptr<Thread>;//也可以写auto
	UPtr ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));//原地构造智能指针
	uint ret = ptr->getId();//防止ptr move完访问不了
	idThreads_[ret] = std::move(ptr);//unique不能拷贝，所以移动过来（右值引用？）
	++curThreadSize_;
	++idleThreadSize_;

	return ret;
}

template<typename Func, typename ...Args>
auto ThreadPool::submitTask(Func&& f, Args && ...args) -> std::future<decltype(f(args ...))>
{
	using rType = decltype(f(args...));
	auto task = std::make_shared<std::packaged_task<rType()>>(
		std::bind(std::forward<Func>(f), std::forward<Args>(args)...));//bind实现异步
	std::future<rType> result = task->get_future();
	// 获取锁
	std::unique_lock<std::mutex> lock(this->taskQueMutex_);
	//线程的通信,等待任务队列有空余
	//	wait_for如果等待超过某时间，就终止，任务提交失败
	if (false == notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()-> bool { return taskQue_.size() < (size_t)taskUpperBound_; }))
	{
		std::cerr << "Time out, submit task failed." << std::endl;//不缓冲直接输出
		auto voidTask = std::make_shared<std::packaged_task<rType()>>(
			[]() ->rType {return rType(); });
		(*voidTask)();//解引用=》调用lambda=》获得RType()
		return voidTask->get_future();//手动终止、返回空结果
	}

	// 如果有空余，把任务放入任务队列中 
	//中间层函数对象std::function<void()>，可以是lambda函数，内部解引用packaged_task，并()调用 创建一个匿名对象
	//该匿名对象在被调用时才执行内部函数体
	taskQue_.emplace([task]() {(*task)(); });
	++curTaskSize_;
	//std::cout << "taskQue_.size()= " << taskQue_.size() << std::endl;// queue.size()没有立即更新
	std::cout << "taskQue_.size()= " << curTaskSize_ << std::endl;

	// 因为新放了任务，任务队列肯定不空了，在notEmpty_上进行通知
	notEmpty_.notify_all();//V

	//这一段要在临界区！不然会访问冲突
	if (poolMode_ == PoolMode::CASHED_MODE) addWhenTram();


	return result;
}
