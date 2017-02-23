#pragma once
#include <iostream>
#include <atomic>
#include <list>
#include <string>
#include <future>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept>
#include <chrono>

enum PRIORITY //事件优先级
{
	EP_Normal = 1, //普通
	EP_Emergency //紧急
};

typedef std::function<void()> CALL_FUNC;
#define  THREAD_COUNT 2
#define  thread_pool ThreadPool::get_instance_ptr()
struct stThreadPoolInfo
{
	PRIORITY	   nPriority; //event priority
	CALL_FUNC  pFuncPtr;//callback function pointer
	stThreadPoolInfo()
	{
		nPriority = EP_Normal;
		pFuncPtr = nullptr;
	}

	stThreadPoolInfo(const stThreadPoolInfo& other)
	{
		nPriority = other.nPriority;
		pFuncPtr = std::move(other.pFuncPtr);
	}

	stThreadPoolInfo(stThreadPoolInfo&& other)
	{
		nPriority = other.nPriority;
		pFuncPtr = std::move(other.pFuncPtr);
	}
	
	stThreadPoolInfo& operator = (stThreadPoolInfo& other)
	{
		if (this != &other)
		{
			nPriority = other.nPriority;
			pFuncPtr = std::move(other.pFuncPtr);
		}
		return *this;
	}

	const stThreadPoolInfo operator = (const stThreadPoolInfo&& other)
	{
		if (this != &other)
		{
			nPriority = other.nPriority;
			pFuncPtr = std::move(other.pFuncPtr);
		}
		return *this;
	}
};


class ThreadPool
{
public:
	static ThreadPool*	get_instance_ptr()
	{
		static ThreadPool instance;
		return &instance;
	}
    ~ThreadPool()
    {
        join_all_thread( );
    }
	/**
	*  Get the total of threads in this pool
	*/
    size_t get_threads_total()
    {
        return _thread_count;
    }
	/**
	*  Get the number of jobs left in the queue.
	*/
    size_t	get_jobs_remaining()
    {
        std::lock_guard<std::mutex> gurad( _queue_mutex );
        return _task_queue.size();
    }
	/**
	*  Join with all threads. Block until all threads have completed.
	*  Params: WaitForAll: If true, will wait for the queue to empty
	*          before joining with threads. If false, will complete
	*          current jobs, then inform the threads to exit.
	*  The queue will be empty after this call, and the threads will
	*  be done. After invoking `ThreadPool::JoinAll`, the pool can no
	*  longer be used. If you need the pool to exist past completion
	*  of jobs, look to use `ThreadPool::wait_all`.
	*/
    void join_all_thread( bool WaitForAll = true )
    {
        if ( !_finished )
        {
            if ( WaitForAll )
                wait_all();
        }
        // note that we're done, and wake up any thread that's
        // waiting for a new job
        _bailout = true;
        _job_available_var.notify_all();
        for ( auto& x : _threads )
        {
            if ( x.joinable() )
                x.join();
        }
        _finished = true;
    }
	/**
	*  Add an normal type functions to the thread pool.
	*	If there are no jobs in the queue,
	*  a thread is woken up to take the job. If all threads are busy,
	*  the job is added to the end of the queue.
	*/
	template<class F , class... Args>
    auto add_normal_task( F&& f , Args&&... args )->std::future<typename std::result_of<F( Args... )>::type>
    {
        using return_type = typename std::result_of<F( Args... )>::type;
        auto task = std::make_shared< std::packaged_task<return_type()> >( std::bind( std::forward<F>( f ) , std::forward<Args>( args )... ) );
        std::future<return_type> res = task->get_future();
        stThreadPoolInfo stInfo;
        stInfo.pFuncPtr = std::move( [ task ] ()->void
        {
            ( *task )( );
        } );
        stInfo.nPriority = EP_Normal;
        {
            std::lock_guard<std::mutex> guard( _queue_mutex );
            auto itor = std::upper_bound( _task_queue.begin() , _task_queue.end() , stInfo , [ &] ( const stThreadPoolInfo& a , const stThreadPoolInfo& b )->bool
            {
                return a.nPriority > b.nPriority;
            } );
            _task_queue.insert( itor , std::move( stInfo ) );
        }
        ++_jobs_left;
        _job_available_var.notify_one();
        return res;
    }
	/**
	*  Add an critical type functions to the thread pool.
	*	If there are no jobs in the queue,
	*  a thread is woken up to take the job. If all threads are busy,
	*  the job is added to the end of the queue.
	*/
	template<class F , class... Args>
    auto add_critical_task( F&& f , Args&&... args )->std::future<typename std::result_of<F( Args... )>::type>
    {
        using return_type = typename std::result_of<F( Args... )>::type;
        auto task = std::make_shared< std::packaged_task<return_type()> >( std::bind( std::forward<F>( f ) , std::forward<Args>( args )... ) );
        std::future<return_type> res = task->get_future();
        stThreadPoolInfo stInfo;
        stInfo.pFuncPtr = std::move( [ task ] ()->void
        {
            ( *task )( );
        } );;
        stInfo.nPriority = EP_Emergency;
        {
            std::lock_guard<std::mutex> guard( _queue_mutex );
            auto itor = std::upper_bound( _task_queue.begin() , _task_queue.end() , stInfo , [ &] ( const stThreadPoolInfo& a , const stThreadPoolInfo& b )->bool
            {
                return a.nPriority > b.nPriority;
            } );
            _task_queue.insert( itor , std::move( stInfo ) );
        }
        ++_jobs_left;
        _job_available_var.notify_one();
        return res;
    }
private:
    ThreadPool()
    {
        size_t core = std::thread::hardware_concurrency();
        _thread_count = core == 0 ? THREAD_COUNT : core;
        _jobs_left = 0;
        _bailout = false;
        _finished = false;
        for ( size_t i = 0; i < _thread_count; ++i )
        {
            _threads.emplace_back( std::move( std::thread( [ this , i ] ()
            {
                this->execute_task();
            } ) ) );
        }
    }
	// Delete all constructors since this is singleton pattern
	ThreadPool( const ThreadPool & ) = delete;
	ThreadPool( ThreadPool && ) = delete;
	ThreadPool& operator=( const ThreadPool & ) = delete;
	ThreadPool& operator=( const ThreadPool && ) = delete;
	/**
	*  Take the next job in the queue and run it.
	*  Notify the main thread that a job has completed.
	*/
    void execute_task()
    {
        while ( !_bailout )
        {
            stThreadPoolInfo stInfo;
            next_task( stInfo );
            if ( stInfo.pFuncPtr )
                stInfo.pFuncPtr();
            --_jobs_left;
            _wait_var.notify_one();
        }
    }
	/**
	*  Get the next job; pop the first item in the queue,
	*  otherwise wait for a signal from the main thread.
	*/
    void next_task( stThreadPoolInfo& stInfo )
    {
        std::unique_lock<std::mutex> job_lock( _queue_mutex );
        _job_available_var.wait( job_lock , [ this ] ()->bool
        {
            return !_task_queue.empty() || _bailout;
        } );
        if ( !_bailout )
        {
            stInfo = std::move( _task_queue.front() );
            _task_queue.pop_front();
        }
    }
	/**
	*  Wait for the pool to empty before continuing.
	*  This does not call `std::thread::join`, it only waits until
	*  all jobs have finshed executing.
	*/
    void wait_all()
    {
        if ( _jobs_left > 0 )
        {
            std::unique_lock<std::mutex> lock( _wait_mutex );
            _wait_var.wait( lock , [ this ] ()->bool
            {
                return 0 == this->_jobs_left;
            } );
            lock.unlock();
        }
    }
private:
	size_t									_thread_count;
	std::atomic_int					_jobs_left;
	std::atomic_bool				_bailout;
	std::atomic_bool				_finished;
	std::mutex							_wait_mutex;
	std::mutex							_queue_mutex;
	std::condition_variable	_job_available_var;
	std::condition_variable	_wait_var;
	std::list<std::thread>		_threads;
	std::list<stThreadPoolInfo>		_task_queue;
};