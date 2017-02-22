#pragma once
#include <stdint.h>
#include <memory>
class BaseData;
class Event
{
public:
	Event() {}
	virtual ~Event() {}
	//用作于初始化
	virtual void init_event() = 0;
	//接收推送的通知
	virtual void on_notify( const int32_t nEventIndex , std::shared_ptr<BaseData> pData ) = 0;
};