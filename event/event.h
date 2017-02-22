#pragma once
#include <stdint.h>
#include <memory>
class BaseData;
class Event
{
public:
	Event() {}
	virtual ~Event() {}
	virtual void init_event() = 0;
	virtual void on_notify( const int32_t nEventIndex , std::shared_ptr<BaseData> pData ) = 0;
};