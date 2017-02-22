#pragma once
#include "builder.h"
#include "event.h"
#include "event_manager.h"
#include <thread>
#include <chrono>
enum INDEX
{
	INDEX_ONE = 0 ,
	INDEX_TWO ,
	INDEX_TREE ,
	INDEX_FOUL
};

struct st
{
	int a;
	int b;
	char buf[ 20 ];
	st(int l, int k) : a(l), b(k)
	{
		memcpy( buf , "hello world" , 20 );
	}

	~st()
	{
		//printf( "st destrcut\n" );
	}
};

class test1 : public Event
{
public:
	test1()
	{
	}
	virtual ~test1() {}
	virtual void on_notify( const int32_t nEventIndex , std::shared_ptr<BaseData> pData )
	{
		if ( INDEX_ONE == nEventIndex )
		{
			std::shared_ptr<Data<char*>> ptr = std::static_pointer_cast< Data<char*> >(pData);
			char buf[ 120 ] = { 0 };
			memcpy( buf , *ptr->get_data() , 120 );
			printf( "recv INDEX_ONE msg , data :%s \n" , buf );;
		}
		else if ( INDEX_TWO == nEventIndex )
		{
			std::shared_ptr<Data<char*>> ptr = std::static_pointer_cast< Data<char*> >(pData);
			char* p = *ptr->get_data();
			printf( "recv INDEX_TWO msg , data :%s\n" , *ptr->get_data() );
		}
	}

	virtual void init_event()
	{
		EventManager::get_ins()->register_event( INDEX_ONE , this );
		EventManager::get_ins()->register_event( INDEX_TWO , this );
	}
	void dispatch()
	{
		char* p = new char[ 30 ];
		memcpy( p , "send msg to INDEX_TREE" , 30 );
		st s( 5 , 4 );
		event_mgr->push_notify( INDEX_TREE , s );

		char* p1 = new char[ 30 ];
		memcpy( p1 , "send msg to INDEX_FOUL" , 30 );
		st s1( 500 , 400 );
		event_mgr->push_notify( INDEX_FOUL , s1 );
	}
};
class test2 : public Event
{
public:
	test2()
	{
	}
	virtual ~test2() {}

	virtual void on_notify( const int32_t nEventIndex , std::shared_ptr<BaseData> pData )
	{
		if ( INDEX_TREE == nEventIndex )
		{
			std::shared_ptr<Data<st>> ptr = std::static_pointer_cast< Data<st> >(pData);
			printf( "recv INDEX_TREE msg , data :%d, %d \n" , ptr->get_data()->a , ptr->get_data()->b );
		}
		else if ( INDEX_FOUL == nEventIndex )
		{
			std::shared_ptr<Data<st>> ptr = std::static_pointer_cast< Data<st> >(pData);
			printf( "recv INDEX_FOUL msg , data :%d,  %d \n"  , ptr->get_data()->a , ptr->get_data()->b );
		}
		else if ( INDEX_ONE == nEventIndex )
		{
			std::shared_ptr<Data<char*>> ptr = std::static_pointer_cast< Data<char*> >(pData);
			char buf[ 120 ] = { 0 };
			memcpy( buf , *ptr->get_data() , 120 );
			printf( "recv  INDEX_ONE msg , data :%s \n" , buf );
		}
	}

	virtual void init_event()
	{
		event_mgr->register_event( INDEX_TREE , this );
		event_mgr->register_event( INDEX_FOUL , this );
		event_mgr->register_event( INDEX_ONE , this );
	}

	void dispatch()
	{
		char* p = new char[ 50 ];
		memcpy( p , "send msg to INDEX_ONE" , 50 );
		EventManager::get_ins()->push_notify( INDEX_ONE , p);

		char* p1 = new char[ 50 ];
		memcpy( p1 , "send msg to INDEX_TWO" , 50 );
		EventManager::get_ins()->push_notify( INDEX_TWO , p1 );
	}
};