#pragma once
#include <vector>
#define  builder Builder::get_ins()
class Event;
class Builder
{
public:
	static Builder* get_ins()
	{
		static Builder ins;
		return &ins;
	}
	virtual ~Builder();
	
public:
	void construct_event();
	void destruct_event();

private:
	Builder();
	Builder( const Builder& ) = delete;
	Builder& operator=( const Builder& ) = delete;
	Builder& operator=( Builder&& ) = delete;
private:
	std::vector<Event*> _vct_events;
};

