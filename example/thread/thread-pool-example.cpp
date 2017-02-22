// test_2015.cpp : 定义控制台应用程序的入口点。
//
#include <stdlib.h>
#include "thread_pool.h"
#include <iostream>
#include <stdio.h>

using namespace std;

struct stTest
{
	int32_t  a;
	int32_t b;
	stTest()
	{
		memset(this, 0, sizeof(*this));
	}
};

class MyClass
{
public:
	MyClass() {}
	~MyClass() {}

	stTest test_func_param(int data, const char* s)
	{
		printf("[ test_func_param ] member function get data : %d, buffer : %s\n", data, s);
		stTest info;
		info.a = data * 2;
		info.b = data + 10;
		return info;
	}

	void test_func_no_param()
	{
		printf("[ test_func_no_param ] member function\n");
	}
private:

};

int test_global(int data, const char* s, stTest& info)
{
	printf("[ test_global ] function  get data : %d, buffer : %s\n", data + (info.a * info.b), s);
	return data + (info.a * info.b);
}

void test_global_no_param()
{
	printf("[ test_global_no_param ] global function\n");
}

//测试
int main()
{
	stTest s;
	s.a = 10;
	s.b = 20;
	MyClass  my;
	int nCount = 1000;
	for (int i = 0; i < nCount; ++i)
	{
		if (i % 2 == 0)
		{
			auto result = thread_pool->add_critical_task ( &MyClass::test_func_param , my , i * 100 , "hello" );
			stTest& info = result.get();
			printf("main thread get stTest info :  a=%d, b=%d\n", info.a, info.b);
			thread_pool->add_critical_task(&MyClass::test_func_no_param, my);
		}
		auto result = thread_pool->add_normal_task(test_global, i, "world", s);
		int data = result.get(); 
		printf("main thread get data : = %d\n", data);
		thread_pool->add_normal_task(test_global_no_param);
	}
	thread_pool->join_all_thread();
	system("pause");
	return 0;
}


