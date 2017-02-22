
#include "event.h"
#include "event_manager.h"
#include <iostream>
#include <memory.h>
#include <thread>
#include "builder.h"

int main( int argc , _TCHAR* argv[] )
{
	Builder::get_ins()->construct_event();
	getchar();
    return 0;
}
