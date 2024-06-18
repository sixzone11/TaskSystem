#include "pch.h"

#define PassString(Name, ...) delegate_call([](auto caller, const char* str, auto&&... args) { static const char* installed = str; caller->passString(installed, std::forward<decltype(args)>(args)...); }, Name, ##__VA_ARGS__);

struct TestCaller1
{
	template<typename Callable, typename... Args>
	auto delegate_call(Callable&& targetMethod, Args... args) { targetMethod(this, std::forward<Args>(args)...); }

	void passString(const char* test) { std::cout << "1: " << test << std::endl; }
	void passString(const char* test, int arg1) { std::cout << "1: " << test << ", " << arg1 << std::endl; }
	void passString(const char* test, int arg1, float arg2) { std::cout << "1: " << test << ", " << arg1 << ", " << arg2 << std::endl; }
};

struct TestCaller2
{
	template<typename Callable, typename... Args>
	auto delegate_call(Callable&& targetMethod, Args... args) { targetMethod(this, std::forward<Args>(args)...); }

	void passString(const char* test) { std::cout << "2: " << test << std::endl; }
	void passString(const char* test, int arg1) { std::cout << "2: " << test << ", " << arg1 << std::endl; }
	void passString(const char* test, int arg1, float arg2) { std::cout << "2: " << test << ", " << arg1 << ", " << arg2 << std::endl; }
};

void test_delegate_call()
{
	TestCaller1 caller1;
	TestCaller2 caller2;

	caller1.PassString("first");
	caller1.PassString("second");
	caller1.PassString("first", 0);
	caller1.PassString("second", 1);
	caller1.PassString("first", 2, 5.3f);
	caller1.PassString("second", 3, 10.6f);

	caller2.PassString("first");
	caller2.PassString("second");
	caller2.PassString("first", 0);
	caller2.PassString("second", 1);
	caller2.PassString("first", 2, 5.3f);
	caller2.PassString("second", 3, 10.6f);
}