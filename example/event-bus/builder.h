#pragma onceheh
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
	//初始化所有的业务类
	void construct_event();
	//析构所有的业务类
	void destruct_event();

private:
	Builder();
	Builder( const Builder& ) = delete;
	Builder& operator=( const Builder& ) = delete;
	Builder& operator=( Builder&& ) = delete;
private:
	std::vector<Event*> _vct_events;
};

