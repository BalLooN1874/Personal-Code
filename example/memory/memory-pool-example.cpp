// memory_pool.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "mem_pool.h"
#include <iostream>
#include <thread>
#include <stdio.h>
#include<time.h>
#include <chrono>  
using namespace std;
using namespace chrono;

class myclass
{
public:
	myclass()
	{
		//cout << " myclass ����" << endl;
	}
	~myclass()
	{
		//cout << " myclass ����" << endl;
	}
private:
	std::string ss;
	int				aaa;
	double      dd;
	char buf[ 1024 * 4 ];
};

int main()
{
	MEM_POOL_CREATE();
	myclass* ptr = nullptr;
	auto start = std::chrono::system_clock::now();
	for (int i = 0; i < 100000; ++i)
	{
		ptr = MEM_POOL_NEW( myclass , sizeof( myclass ) );
		MEM_POOL_DELETE( myclass , ptr , sizeof( myclass ) );
	}
	auto end = std::chrono::system_clock::now();
	auto duration = duration_cast<microseconds>( end - start ); 
	cout << "������"
		<< double( duration.count() ) * microseconds::period::num / microseconds::period::den
		<< "��" << endl;

	start = std::chrono::system_clock::now();
	for ( int i = 0; i < 100000; ++i )
	{
		//ptr = (myclass*)malloc(sizeof(myclass));
		//free( ptr);
		ptr = new myclass();
		delete ptr;
	}
	 end = std::chrono::system_clock::now();
	 duration = duration_cast<microseconds>( end - start );
	cout << "new delete ������"
		<< double( duration.count() ) * microseconds::period::num / microseconds::period::den
		<< "��" << endl;
	MEM_POOL_DESTORY();
	system( "pause" );
    return 0;
}

