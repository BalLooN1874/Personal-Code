#include "builder.h"
#include "test.h"
Builder::Builder()
{
}

Builder::~Builder()
{
	destruct_event();
}

void Builder::construct_event()
{
	
	test1* ptr = new test1();
	_vct_events.emplace_back( ptr );

	test2* ptr1 = new test2();
	_vct_events.emplace_back( ptr1 );

	for (auto ptr : _vct_events)
	{
		ptr->init_event();
	}

	ptr->dispatch();
	ptr1->dispatch();
}

void Builder::destruct_event()
{
	for (auto ptr : _vct_events)
	{
		if ( ptr )
		{
			delete ptr;
			ptr = nullptr;
		}
	}
}