#pragma once
#include <unordered_map>
#include <functional>
#include <mutex>
#include "thread_pool.h"
class Event;
class  BaseData
{
public:
	BaseData( int32_t id ) : _id (id){}
	virtual ~BaseData() {}

	uint32_t    get_id() const { return _id; }
protected:
	uint32_t   _id;
};

template<typename T>
class Data : public BaseData
{
public:
	Data() {}
	Data( int32_t id , T& data ) : BaseData( id ) 
	{
		_data = std::make_shared<T>( data );
	}
	virtual ~Data() {}

	T* get_data()
	{
		return _data.get();
	}
private:
	std::shared_ptr<T> _data;
};

#define  event_mgr EventManager::get_ins()
class EventManager
{
public:
	static EventManager* get_ins()
	{
		static EventManager ins;
		return &ins;
	}

	virtual ~EventManager();
	//注册事件
	bool register_event( const int32_t nEventIndex ,  Event* pInstance );
	//移除事件
	bool unregister_event( const int32_t nEventIndex , const Event* pInstance );
	//推送通知
	template<typename T>
	bool push_notify(  int32_t nEventIndex ,  T& data )
	{
		if ( nEventIndex < 0 )
			return false;

		std::shared_ptr<Data<T>> pData = std::make_shared<Data<T>>(  nEventIndex , data );
		auto itor = _event_map.equal_range( nEventIndex );
		for_each( itor.first , itor.second , [ & ]( std::unordered_multimap<int32_t , Event*>::value_type& i )
		{
			Event* pEvent = itor.first->second;
			thread_pool->add_normal_task( &Event::on_notify , pEvent , nEventIndex , pData );
			++itor.first;
		} );
		return true;
	}

private:
	EventManager();
	EventManager( const EventManager& ) = delete;
	EventManager& operator=( const EventManager& ) = delete;
	EventManager& operator=( EventManager&& ) = delete;
private:
	std::unordered_multimap<int32_t , Event*> _event_map;
	std::mutex _mtx;
};

