#include "pch.h"

#include <TaskSystem/lambda_details.h>

template<typename T>
auto bind(T&& binding)
{
}

template<typename T>
struct FutureResult
{
	T value;

	//operator T && () && { 	return value; }
	operator T& () { return value; }
};

template<>
struct FutureResult<void>
{
};

template<typename RetType, typename ... ParamTypes>
FutureResult<RetType> getFutureResult(RetType(*func)(ParamTypes...))
{
	return FutureResult<RetType>();
};

template<typename Type, typename RetType, typename ... ParamTypes>
FutureResult<RetType> getFutureResult(RetType(Type::* func)(ParamTypes...))
{
	return FutureResult<RetType>();
};

template<typename Callable, typename = std::enable_if_t<std::is_class_v<std::remove_reference_t<Callable>>>>
auto getFutureResult(Callable&& arg)
{
	return FutureResult<typename lambda_details<Callable>::RetType>();
}

template<typename PrevFunc>
auto result(PrevFunc&& func)
{
	return getFutureResult(func);
}

struct ResultHolder {};

template<size_t... Index>
auto delayed()
{
	return std::make_tuple(ResultHolder{}, std::array<ResultHolder, sizeof...(Index)> {});
}

struct MemberFunctionTest
{
	uint32_t MemberFunctionA() { return 0; }
};

// result() 정리
void resultTest(const char* arg)
{
	// Member Function Test
	struct MemberFunctionTest
	{
		bool func(const char*) { return false; }
	};
	auto resultBoolOfMemberFunctionTest = result(&MemberFunctionTest::func);

	// Functor Test
	//	result() 를 통해 선택 시 더 인자가 적은 operator()가 선택됨
	struct FunctorTest
	{
		int operator()(double, double, double) { return true; }
		//bool operator()(const char*, int) { return true; }
	} functorTest;
	auto resultVoidOfFunctorTest = result(functorTest);

	// Lambda Test
	//	Lambda는 곧 unnamed functor이므로 클래스로써 식별, functor test와 같음.
	auto lambdaTest = [arg]() {};
	auto resultVoidOfLambdaTest = result(lambdaTest);


	&FunctorTest::operator();
}