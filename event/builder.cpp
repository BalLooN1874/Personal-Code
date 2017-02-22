#include "builder.h"

Builder::Builder()
{
}

Builder::~Builder()
{
	destruct_event();
}


void Builder::construct_event()
{
	Event* ptr = nullptr;

	for (auto ptr : _vct_events)
	{
		ptr->init_event();
	}
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