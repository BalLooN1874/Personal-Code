#pragma once
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <future>
#include <list>
#include <vector>
#define  THREAD_NUM 2 
#define  timer_queue TimerQueue::get_instance_ptr()
typedef std::function<void()> void_func;
typedef std::chrono::high_resolution_clock high_res_clcok;
struct stTimerEvent
{
	uint32_t								nTimerID;	// Unique id for each event object
	uint64_t								nNextRun;	// Next scheduled execution time
	bool									isRepeat;	 // Repeat event indefinitely if true
	void_func							pEventHandler;  // Event handler callback
	high_res_clcok::time_point nTimeout;	// Interval for execution(in millisecond)
	stTimerEvent() :nTimerID( 0 ) ,
								nNextRun( 0 ) ,
								isRepeat( false ) ,
								pEventHandler( nullptr )
	{
	}
};

class TimerQueue
{
public:
	~TimerQueue()
	{
		if ( !_stop_thread )
			timer_queue_exit();
	}

	static TimerQueue* get_instance_ptr()
	{
		static TimerQueue ins;
		return &ins;
	}

	/**create timer
	 *  --------------------------  WARNING  -----------------------------
	 *If the 'isRepeat' parameter is true , the callback function returns the results only for once
	 *'std::future::get()'  just inovke once, getting results is the function for the first time the return value 
	 *otherwise there will be a unknown error
	 **/
	template<typename F , typename... Args>
	auto create_timer( uint32_t& nTimerID , const uint32_t nTimeout , bool isRepeat , F&& func , Args&&... args )->std::future<typename std::result_of<F( Args... )>::type>
	{
		using return_type = typename std::result_of<F( Args... )>::type;
		// Define generic funtion with any number / type of args
		// using std::bind
		auto event = std::make_shared<std::packaged_task<return_type()>>( std::bind( std::forward<F>( func ) , std::forward<Args>( args )... ) );
		std::future<return_type> res = event->get_future();
		// Create new event object
		stTimerEvent stInfo;
		stInfo.nTimerID = ++_timer_id;
		stInfo.nTimeout = high_res_clcok::now() + std::chrono::milliseconds( nTimeout );
		stInfo.nNextRun = nTimeout;
		stInfo.isRepeat = isRepeat;
		stInfo.pEventHandler = std::move( [ = ] ()->void
		{
			( *event )( ); 
            if ( isRepeat )
			{
				event->reset();
			}
		} );
		{
			std::unique_lock<std::mutex> lock( _event_mutex );
			_event_vector.emplace_back( std::move( stInfo ) );
			_is_sort = true;
		}
		// Notify timer loop thread to process event
		_event_cond.notify_one();
		nTimerID = _timer_id;
		return res;
	}
	/**
	 *cancel timer by timer id
	 */
	uint8_t cancel_timer( const uint32_t nTimerID )
	{
		std::unique_lock<std::mutex> lock( _event_mutex );
		// Search for event with matching id in current pair
		auto itor = std::find_if( _event_vector.begin() , _event_vector.end() , [ = ] ( const stTimerEvent& e )->bool
		{
			return nTimerID == e.nTimerID;
		} );
		if ( itor != _event_vector.end() )
		{
			_event_vector.erase( itor );
			std::cout << "Event with id " << nTimerID << " deleted succesfully\n";
			_is_sort = true;
			return 1;
		}
		return 0;
	}
	/**
	*exit timer queue
	*/
	void timer_queue_exit()
	{
		_stop_thread = true;
		_event_cond.notify_all();
		_execute_cond.notify_all();

		if ( _timer_loop_thread.joinable() )
			_timer_loop_thread.join();

		for ( auto& it : _thread_list )
		{
			if ( it.joinable() )
				it.join();
		}
	}
private:
	TimerQueue() :_stop_thread( false ) ,
		                       _is_sort( false )
	{
		_timer_id = 0;
		_timer_loop_thread = std::thread( [ this ] ()
		{
			this->timer_loop();
		} );
		for ( int i = 0; i < THREAD_NUM; ++i )
		{
			std::thread th = std::thread( [ this ] ()
			{
				this->execute_timeout_event();
			} );
			_thread_list.emplace_back( std::move( th ) );
		}
	}
	/**
	*Delete all constructors since this is singleton pattern
	*/
	TimerQueue( const TimerQueue & ) = delete;
	TimerQueue( TimerQueue && ) = delete;
	TimerQueue& operator=( const TimerQueue & ) = delete;
	TimerQueue& operator=( const TimerQueue && ) = delete;
	/**
	*timer queue loop
	*/
	int32_t timer_loop()
	{
		while ( true )
		{
			if ( _stop_thread )
			{
				std::cout << "timer loop thread has been stopped\n";
				return 0;
			}
			// Block till least timeout value in _event_map is expired or
			// new event gets added to _event_vector with lesser value
			std::unique_lock<std::mutex> lock( _event_mutex );
			_event_cond.wait( lock , [ this ] ()->bool
			{
				return this->_stop_thread || !_event_vector.empty();
			} );
			if ( _is_sort )
			{
				std::sort( _event_vector.begin() , _event_vector.end() , [] ( const stTimerEvent& t1 , const stTimerEvent& t2 )->bool
				{
					return t1.nTimeout > t2.nTimeout;
				} );
				_is_sort = false;
			}
			auto now = high_res_clcok::now();
			//cancel timer, _event_vector will be empty
			// _event_vector is empty. invoke 'vector::back' , will be have unkown error
			if ( _event_vector.empty() )
				continue;
			stTimerEvent& stInfo = _event_vector.back();
			auto expire = stInfo.nTimeout;
			if ( expire > now )
			{
				auto napTime = expire - now;
				_event_cond.wait_for( lock , napTime );
				//check again
				auto now = high_res_clcok::now();
				if ( _event_vector.empty() )
					continue;
				stTimerEvent& stInfo = _event_vector.back();
				auto expire = stInfo.nTimeout;
				if ( expire <= now )
				{
					add_timeout_event( stInfo.pEventHandler );
					// If event has to be run once then delete
					// from vector else increment  nNextRun which
					// will be compared below to get next event to be run
					if ( stInfo.isRepeat )
					{
						stInfo.nTimeout = high_res_clcok::now() + std::chrono::milliseconds( stInfo.nNextRun );
						_is_sort = true;
					}
					else
						_event_vector.pop_back();
				}
			}
			else
			{
				if ( _event_vector.empty() )
					continue;
				stTimerEvent& stInfo = _event_vector.back();
				add_timeout_event( stInfo.pEventHandler );
				if ( stInfo.isRepeat )
				{
					stInfo.nTimeout = high_res_clcok::now() + std::chrono::milliseconds( stInfo.nNextRun );
					_is_sort = true;
				}
				else
					_event_vector.pop_back();
			}
		}
		return 0;
	}
	/**
	*add timeout event to execution list
	*/
	void add_timeout_event( void_func& pFuncPtr )
	{
		if ( !pFuncPtr )
			return;
		std::unique_lock<std::mutex> lock( _execute_mutex );
		_execute_event_list.emplace_back( pFuncPtr );
		_execute_cond.notify_one();
	}
	/**
	*execute timeout event
	*/
	void execute_timeout_event()
	{
		while ( true )
		{
			void_func pFuncPtr;
			{
				std::unique_lock<std::mutex> lock( _execute_mutex );
				_execute_cond.wait( lock , [ this ] ()->bool
				{
					return !_execute_event_list.empty() || _stop_thread;
				} );
				if ( _stop_thread )
				{
					std::cout << "execute timeout event thread has been stopped\n";
					return;
				}
				pFuncPtr = _execute_event_list.front();
				_execute_event_list.pop_front();
			}

			if ( pFuncPtr )
				pFuncPtr();
		}
	}
private:
	std::list<std::thread>		_thread_list;
	std::list<void_func>		_execute_event_list;
	std::mutex							_execute_mutex;
	std::condition_variable	_execute_cond;
	std::thread							_timer_loop_thread;
	std::mutex							_event_mutex;
	std::condition_variable	_event_cond;
	bool									_stop_thread;
	bool									_is_sort;
	std::atomic_int					_timer_id;
	std::vector<stTimerEvent> _event_vector;
};
#if 0
TimerQueue::TimerQueue() :_stop_thread( false ) ,
													_is_sort( false )
{
	_timer_id = 0;
	_timer_loop_thread = std::thread( [ this ] () {this->timer_loop(); } );
	for ( int i = 0; i < THREAD_NUM; ++i )
	{
		std::thread th = std::thread( [ this ] () {this->execute_timeout_event(); } );
		_thread_list.emplace_back( std::move( th ) );
	}
}

TimerQueue::~TimerQueue()
{
	if ( !_stop_thread )
		timer_queue_exit();
}

/**create timer
*  --------------------------  WARNING  -----------------------------
*If the 'isRepeat' parameter is true , the callback function returns the results only for once
*'std::future::get()'  just inovke once, getting results is the function for the first time the return value
*otherwise there will be a unknown error
**/
template<typename F , typename... Args>
auto TimerQueue::create_timer( uint32_t& nTimerID , const uint32_t nTimeout , bool isRepeat , F&& func , Args&&... args )->std::future<typename std::result_of<F( Args... )>::type>
{
	using return_type = typename std::result_of<F( Args... )>::type;
	// Define generic funtion with any number / type of args
	// using std::bind
	auto event = std::make_shared<std::packaged_task<return_type()>>( std::bind( std::forward<F>( func ) , std::forward<Args>( args )... ) );
	std::future<return_type> res = event->get_future();
	// Create new event object
	stTimerEvent stInfo;
	stInfo.nTimerID = ++_timer_id;
	stInfo.nTimeout = high_res_clcok::now() + std::chrono::milliseconds( nTimeout );
	stInfo.nNextRun = nTimeout;
	stInfo.isRepeat = isRepeat;
	stInfo.pEventHandler = std::move( [=]()->void {( *event )( ); if ( isRepeat ) { event->reset(); } } );
	{
		std::unique_lock<std::mutex> lock( _event_mutex );
		_event_vector.emplace_back( std::move( stInfo ) );
		_is_sort = true;
	}
	// Notify timer loop thread to process event
	_event_cond.notify_one();
	nTimerID = _timer_id;
	return res;
}
/**
*cancel timer by timer id
*/
uint8_t TimerQueue::cancel_timer( const uint32_t nTimerID )
{
	std::unique_lock<std::mutex> lock( _event_mutex );
	// Search for event with matching id in current pair
	auto itor = std::find_if( _event_vector.begin() , _event_vector.end() , [ = ] (const stTimerEvent& e )->bool {return nTimerID ==e.nTimerID ; } );
	if ( itor != _event_vector.end() )
	{
		_event_vector.erase( itor );
		std::cout << "Event with id " << nTimerID << " deleted succesfully\n";
		_is_sort = true;
		return 1;
	}
	return 0;
}

/**
*exit timer queue
*/
void TimerQueue::timer_queue_exit()
{
	_stop_thread = true;
	_event_cond.notify_all();
	_execute_cond.notify_all();

	if ( _timer_loop_thread.joinable() )
		_timer_loop_thread.join();

	for ( auto& it : _thread_list )
	{
		if ( it.joinable() )
			it.join();
	}
}

/**
*timer queue loop
*/
int32_t TimerQueue::timer_loop()
{
	while ( true )
	{	
		if ( _stop_thread )
		{
			std::cout << "timer loop thread has been stopped\n";
			return 0;
		}
		// Block till least timeout value in _event_map is expired or
		// new event gets added to _event_vector with lesser value
		std::unique_lock<std::mutex> lock( _event_mutex );
		_event_cond.wait( lock , [ this ] ()->bool { return this->_stop_thread || !_event_vector.empty(); } );
		if ( _is_sort )
		{
			std::sort( _event_vector.begin() , _event_vector.end() , [ ] ( const stTimerEvent& t1 , const stTimerEvent& t2 )->bool {return t1.nTimeout > t2.nTimeout; } );
			_is_sort = false;
		}
		auto now = high_res_clcok::now();
		//cancel timer, _event_vector will be empty
		// _event_vector is empty. invoke 'vector::back' , will be have unkown error
		if ( _event_vector.empty() )
			continue;
		stTimerEvent& stInfo = _event_vector.back();
		auto expire = stInfo.nTimeout;
		if ( expire > now )
		{
			auto napTime = expire - now;
			_event_cond.wait_for( lock , napTime );
			//check again
			auto now = high_res_clcok::now();
			if ( _event_vector.empty() )
				continue;
			stTimerEvent& stInfo = _event_vector.back();
			auto expire = stInfo.nTimeout;
			if ( expire <= now )
			{
				add_timeout_event( stInfo.pEventHandler );
				// If event has to be run once then delete
				// from vector else increment  nNextRun which
				// will be compared below to get next event to be run
				if ( stInfo.isRepeat )
				{
					stInfo.nTimeout = high_res_clcok::now() + std::chrono::milliseconds( stInfo.nNextRun );
					_is_sort = true;
				}
				else
					_event_vector.pop_back();
			}
		}
		else
		{
			if ( _event_vector.empty() )
				continue;
			stTimerEvent& stInfo = _event_vector.back();
			add_timeout_event( stInfo.pEventHandler );
			if ( stInfo.isRepeat )
			{
				stInfo.nTimeout = high_res_clcok::now() + std::chrono::milliseconds( stInfo.nNextRun );
				_is_sort = true;
			}
			else
				_event_vector.pop_back();
		}
	}
	return 0;
}

/**
*add timeout event to execution list
*/
void TimerQueue::add_timeout_event( void_func& pFuncPtr )
{
	if ( !pFuncPtr )
		return;
	std::unique_lock<std::mutex> lock( _execute_mutex );
	_execute_event_list.emplace_back( pFuncPtr );
	_execute_cond.notify_one();
}

/**
*execute timeout event
*/
void TimerQueue::execute_timeout_event()
{
	while ( true )
	{
		void_func pFuncPtr;
		{
			std::unique_lock<std::mutex> lock( _execute_mutex );
			_execute_cond.wait( lock , [ this ] ()->bool {return !_execute_event_list.empty() || _stop_thread; } );
			if ( _stop_thread )
			{
				std::cout << "execute timeout event thread has been stopped\n";
				return;
			}
			pFuncPtr = _execute_event_list.front();
			_execute_event_list.pop_front();
		}

		if( pFuncPtr )
			pFuncPtr();
	}
}
#endif