// test_timer.cpp : 定义控制台应用程序的入口点。
//
#include "timer_queue.h"
#include <iostream>
#include <random>
#include <string>
#include <chrono>
#include <algorithm>
#include <iterator>
using namespace std;

atomic_int dd;
int test_func_1(int a, const char* buf)
{
	cout << "test_func_1 get index : " << ++dd << ", and index " << buf << endl;
	if ( dd == 10 )
		int l = 0;
	return dd;
}

int test_func_2( int a , const char* buf )
{
	cout << "test_func_2 get index : " << ++dd << ", and buffer " << buf << endl;
	return dd;
}

class myclass
{
	public:
		void  add(int a , int b)
		{
			cout << "sum = " << a + b << endl;
		}
};
int main()
{
	dd = 0;
	uint32_t timerID_1 = 0;
	uint32_t timerID_2 = 0;
	uint32_t timerID_3 = 0;
	uint32_t timerID_4 = 0;
	std::random_device rd;
	std::mt19937 gen( rd() );

	std::uniform_int_distribution<> dis( 500 , 5000 );
	std::string str = "hello world : ";
	timer_queue->create_timer( timerID_1 , 1000 , true , test_func_1 , 1000 , str.c_str() );
	timer_queue->create_timer( timerID_2 , 500 , true , test_func_1 , 500 , "hello" );
	timer_queue->create_timer( timerID_3 , 520 , true , test_func_1 , 550 , "world" );
	std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
	timer_queue->create_timer( timerID_4 , 2000 , true , test_func_2 , 9999 , "####云南昆明####" );
	myclass my;
	timer_queue->create_timer( timerID_2 , 500 , true , &myclass::add ,my, 123 , 456 );
	this_thread::sleep_for( std::chrono::milliseconds( 10* 1000 ) );
	timer_queue->timer_queue_exit();

	system( "pause" );
    return 0;
}
