#include "event_manager.h"
EventManager::EventManager() : _event_map()
{
}


EventManager::~EventManager()
{
}

bool EventManager::register_event( const int32_t nEventIndex ,  Event* pInstance )
{
	if ( nEventIndex < 0 || !pInstance )
		return false;

	std::lock_guard<std::mutex> lock( _mtx );
	_event_map.emplace( nEventIndex , pInstance );
	return true;
}

bool EventManager::unregister_event( const int32_t nEventIndex , const Event* pInstance )
{
	if ( nEventIndex < 0 || !pInstance )
		return false;
	
	auto itor = _event_map.equal_range( nEventIndex );
	while (itor.first != itor.second)
	{
		Event* ptr = itor.first->second;
		if ( pInstance == ptr )
		{
			std::lock_guard<std::mutex> lock( _mtx );
			itor.first = _event_map.erase( itor.first );
			continue;
		}
		++itor.first;
	}
	return true;
}

