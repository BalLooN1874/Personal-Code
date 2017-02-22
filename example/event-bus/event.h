#pragma once
#include <stdint.h>
#include <memory>
class BaseData;
class Event
{
public:
	Event() {}
	virtual ~Event() {}
	//初始化类
	virtual void init_event() = 0;
	//接收推送的消息
	virtual void on_notify( const int32_t nEventIndex , std::shared_ptr<BaseData> pData ) = 0;
};