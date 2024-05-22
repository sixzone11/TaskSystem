#pragma once
#include <stdint.h>
#include <tuple>
#include <vector>
#include <iostream>

#include "TaskSystem/TaskCallableDescription.h"

template<typename T, uint32_t N>
struct AddByN
{
private:
	constexpr static const uint32_t Num = std::tuple_size<decltype(T::_var)>::value;

	template<typename T_, uint32_t N_, uint32_t RI>
	struct __AddByN
	{
		constexpr static const uint32_t I = std::tuple_size<decltype(T::_var)>::value - RI;
		constexpr static const auto _var = std::tuple_cat(std::make_tuple(std::get<I>(T::_var) + N), __AddByN<T_, N_, RI - 1>::_var);
	};

	template<typename T_, uint32_t N_>
	struct __AddByN<T_, N_, 0>
	{
		constexpr static const auto _var = std::make_tuple();
	};

public:
	constexpr static const auto _var = __AddByN<T, N, Num>::_var;
};

template<typename T1, typename T2>
struct AddTwo
{
private:
	constexpr static const uint32_t Num1 = std::tuple_size<decltype(T1::_var)>::value;
	constexpr static const uint32_t Num2 = std::tuple_size<decltype(T2::_var)>::value;
	static_assert(Num1 == Num2, "Size must be same");

	template<typename T1_, typename T2_, uint32_t RI>
	struct __AddTwo
	{
		constexpr static const uint32_t I = std::tuple_size<decltype(T1::_var)>::value - RI;
		constexpr static const auto _var = std::tuple_cat(std::make_tuple(std::get<I>(T1::_var) + std::get<I>(T2::_var)), __AddTwo<T1_, T2_, RI - 1>::_var);
	};

	template<typename T1_, typename T2_>
	struct __AddTwo<T1_, T2_, 0>
	{
		constexpr static const auto _var = std::make_tuple();
	};

public:
	constexpr static const auto _var = __AddTwo<T1, T2, Num1>::_var;
};

template<typename T, uint32_t N>
struct MultiplyByN
{
private:
	constexpr static const uint32_t Num = std::tuple_size<decltype(T::_var)>::value;

	template<typename T_, uint32_t N_, uint32_t RI>
	struct __MultiplyByN
	{
		constexpr static const uint32_t I = std::tuple_size<decltype(T::_var)>::value - RI;
		constexpr static const auto _var = std::tuple_cat(std::make_tuple(std::get<I>(T::_var) * N), __MultiplyByN<T_, N_, RI - 1>::_var);
	};

	template<typename T_, uint32_t N_>
	struct __MultiplyByN<T_, N_, 0>
	{
		constexpr static const auto _var = std::make_tuple();
	};

public:
	constexpr static const auto _var = __MultiplyByN<T, N, Num>::_var;
};

template<typename T1, typename T2>
struct MultiplyTwo
{
private:
	constexpr static const uint32_t Num1 = std::tuple_size<decltype(T1::_var)>::value;
	constexpr static const uint32_t Num2 = std::tuple_size<decltype(T2::_var)>::value;
	static_assert(Num1 == Num2, "Size must be same");

	template<typename T1_, typename T2_, uint32_t RI>
	struct __MultiplyTwo
	{
		constexpr static const uint32_t I = std::tuple_size<decltype(T1::_var)>::value - RI;
		constexpr static const auto _var = std::tuple_cat(std::make_tuple(std::get<I>(T1::_var) * std::get<I>(T2::_var)), __MultiplyTwo<T1_, T2_, RI - 1>::_var);
	};

	template<typename T1_, typename T2_>
	struct __MultiplyTwo<T1_, T2_, 0>
	{
		constexpr static const auto _var = std::make_tuple();
	};

public:
	constexpr static const auto _var = __MultiplyTwo<T1, T2, Num1>::_var;
};

template<uint32_t N, typename T>
struct MakeOffsetBias
{
private:
	constexpr static const uint32_t MAX = N - 1;

	template<typename T_, uint32_t I, uint32_t J>
	struct __MakeOffsetBias
	{
		constexpr static const auto _var = std::tuple_cat(std::make_tuple(J), __MakeOffsetBias<T, (I + 1), ((I < std::get<J>(T::_var)) ? J : (J + 1))>::_var);
	};

	template<typename T_, uint32_t J>
	struct __MakeOffsetBias<T_, MAX, J>
	{
		constexpr static const auto _var = std::make_tuple(J);
	};

public:
	constexpr static const auto _var = __MakeOffsetBias<T, 0, 0>::_var;
};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumOutputs, uint32_t NumInputsNext, typename Links, typename Offsets, typename Outputs, typename InputsNext>
struct MakeExpandedLink
{
private:
	static_assert(NumLinks == std::tuple_size<decltype(Links::_var)>::value, "Expected NumLinks is mismatched with Links");
	static_assert(NumOutputs == std::tuple_size<decltype(Outputs::_var)>::value, "Expected NumOutputs is mismatched with Outputs");
	static_assert(NumInputsNext == std::tuple_size<decltype(InputsNext::_var)>::value, "Expected NumInputsNext is mismatched with InputsNext");

	constexpr static const auto _linkedInputs = AddByN<InputsNext, NumManifests>::_var;

	constexpr static const uint32_t MaxLinks = NumLinks - 1;
	constexpr static const uint32_t MaxLinkedInputs = NumInputsNext - 1;

	template<typename Links_, uint32_t L, bool useLinkedInput>
	struct __MakeLink { constexpr static const auto _var = std::make_tuple(std::get<L>(Links::_var)); };

	template<typename Links_, uint32_t L>
	struct __MakeLink<Links_, L, false> { constexpr static const auto _var = _linkedInputs; };

	template<typename Links_, uint32_t L, uint32_t O>
	struct __MakeExpandedLink
	{
		constexpr static const bool cond = (L != std::get<std::get<O>(Outputs::_var)>(Offsets::_var));
		constexpr static const auto _var = std::tuple_cat(
			__MakeLink<Links, L, cond>::_var,
			__MakeExpandedLink<Links, L + 1, cond ? O : (O + 1)>::_var);
	};

	template<typename Links_, uint32_t O>
	struct __MakeExpandedLink<Links_, NumLinks, O>
	{
		constexpr static const auto _var = std::make_tuple();
	};


public:
	constexpr static const auto _var = __MakeExpandedLink<Links, 0, 0>::_var;
};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumOutputs, typename Links, typename Offsets, typename Outputs>
struct MakeMergedOutput
{
	template<typename Links_, uint32_t L, uint32_t O>
	struct __MakeMergedOutput
	{
		constexpr static const bool cond = (L != std::get<std::get<O>(Outputs::_var)>(Offsets::_var));
		constexpr static const auto _var = std::tuple_cat(
			std::make_tuple(cond ? std::get<L>(Links::_var) : NumManifests),
			__MakeMergedOutput<Links, (L + 1), cond ? O : (O + 1)>::_var);
	};

	template<typename Links_, uint32_t O>
	struct __MakeMergedOutput<Links_, NumLinks, O>
	{
		constexpr static const auto _var = std::make_tuple();
	};


public:
	constexpr static const auto _var = __MakeMergedOutput<Links, 0, 0>::_var;
};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes>
struct GetNext;

// Delete ambiguity between <1,1,1,1,DefType> and <M,L,I,O,true/false>
template<>
struct GetNext<1, 1, 1, 1, true>
{
public:
	constexpr static const std::tuple<uint32_t> _offsets{ 0 };
	constexpr static const std::tuple<uint32_t> _links{ 1 };
	constexpr static const std::tuple<uint32_t> _inputs{ 0 };
	constexpr static const std::tuple<uint32_t> _outputs{ 0 };
	constexpr static const std::tuple<uint32_t, uint32_t> _precedingCount{ 0, 1 };
};

template<>
struct GetNext<1, 1, 1, 1, false> : GetNext<1, 1, 1, 1, true> {};

template<typename First, typename... Nexts>
struct GetChain;

template<typename First, typename Second, typename... Nexts>
struct GetChainNext;

/**
 * Task Layout
 *
 * Task - { TaskDefine, TaskMeta }
 * Chain - { { Task, ... }, TaskChainMeta }
 * Junction - { { Task, ... }, TaskJunctionMeta }
 *
 * ChainWithArgs - { ChainDefine { Task, Task, Argument, Task, Argument, Argument }, TaskChainMeta { Task, Task, Argument, Task, Argument, Argument } }
 */

struct ResultHolder
{

};

template<
	uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes,
	uint32_t NumManifestsNext, uint32_t NumLinksNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, bool DepTypeNext, typename... PathesNext, typename... Nexts>
struct GetChainNext<
	GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>,
	GetNext<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext, DepTypeNext, PathesNext...>, Nexts...>
{
	using First = GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>;
	using Second = GetNext<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext, DepTypeNext, PathesNext...>;

	struct OutputFirstSrc { constexpr static const auto _var = First::_outputs; };
	struct LinkFirstSrc { constexpr static const auto _var = First::_links; };
	struct OffsetFirstSrc { constexpr static const auto _var = First::_offsets; };

	struct InputSecondSrc { constexpr static const auto _var = Second::_inputs; };

	struct OffsetBias { constexpr static const auto _var = MultiplyByN<MakeOffsetBias<NumManifests, OutputFirstSrc>, NumInputsNext - 1>::_var; };
	struct LinkedToNext { constexpr static const auto _var = MakeExpandedLink<NumManifests, NumLinks, NumOutputs, NumInputsNext, LinkFirstSrc, OffsetFirstSrc, OutputFirstSrc, InputSecondSrc>::_var; };

	constexpr static const uint32_t NumAddsToLink = NumLinks + NumOutputs * (NumInputsNext - 1);
};

template<
	uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes, typename... OtherArgumentTypes,
	uint32_t NumManifestsNext, uint32_t NumLinksNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, bool DepTypeNext, typename... PathesNext, typename... Nexts>
struct GetChainNext<
	GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, std::tuple<OtherArgumentTypes...>,
	GetNext<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext, DepTypeNext, PathesNext...>, Nexts...>
	: GetChainNext<
	GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>,
	GetNext<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext, DepTypeNext, PathesNext...>, Nexts...>
{};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes, typename... Nexts>
struct GetChain<GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, Nexts...>
{
	using First = GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>;

	struct OffsetFirstSrc { constexpr static const auto _var = First::_offsets; };

	struct LinkNextsSrc { constexpr static const auto _var = GetChain<Nexts...>::_links; };
	struct OutputNextsSrc { constexpr static const auto _var = GetChain<Nexts...>::_outputs; };
	struct OffsetNextsSrc { constexpr static const auto _var = GetChain<Nexts...>::_offsets; };

	constexpr static const auto _offsets = std::tuple_cat(AddTwo<OffsetFirstSrc, typename GetChainNext<First, Nexts...>::OffsetBias>::_var, AddByN<OffsetNextsSrc, GetChainNext<First, Nexts...>::NumAddsToLink>::_var);
	constexpr static const auto _links = std::tuple_cat(GetChainNext<First, Nexts...>::LinkedToNext::_var, AddByN<LinkNextsSrc, NumManifests>::_var);

	constexpr static const auto _inputs = First::_inputs;
	constexpr static const auto _outputs = AddByN<OutputNextsSrc, NumManifests>::_var;
	constexpr static const auto _precedingCount = First::_precedingCount;
};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes, typename... OtherArgumentTypes, typename... Nexts>
struct GetChain<GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, std::tuple<OtherArgumentTypes...>, Nexts...>
	: GetChain<GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, Nexts...>
{};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes>
struct GetChain<GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>>
{
	using First = GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>;

	constexpr static const auto _offsets = First::_offsets;
	constexpr static const auto _links = First::_links;
	constexpr static const auto _inputs = First::_inputs;
	constexpr static const auto _outputs = First::_outputs;
	constexpr static const auto _precedingCount = First::_precedingCount;
};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes, typename... OtherArgumentTypes>
struct GetChain<GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, std::tuple<OtherArgumentTypes...>>
	: GetChain<GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>>
{};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... Pathes>
struct GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, true, Pathes...>
{
	constexpr static const auto _offsets = GetChain<Pathes...>::_offsets;
	constexpr static const auto _links = GetChain<Pathes...>::_links;
	constexpr static const auto _inputs = GetChain<Pathes...>::_inputs;
	constexpr static const auto _outputs = GetChain<Pathes...>::_outputs;
	constexpr static const auto _precedingCount = GetChain<Pathes...>::_precedingCount;
};



template<typename First, typename... Nexts>
struct GetJunction;

template<typename First, typename Second, typename... Nexts>
struct GetJunctionNext;


template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes, typename... Nexts>
struct GetJunction<GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, Nexts...>
{
	using First = GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>;

	struct LinkNextsSrc { constexpr static const auto _var = GetJunction<Nexts...>::_links; };
	struct OffsetNextsSrc { constexpr static const auto _var = GetJunction<Nexts...>::_offsets; };
	struct InputNextsSrc { constexpr static const auto _var = GetJunction<Nexts...>::_inputs; };
	struct OutputNextsSrc { constexpr static const auto _var = GetJunction<Nexts...>::_outputs; };

	constexpr static const auto _inputs = std::tuple_cat(First::_inputs, AddByN<InputNextsSrc, NumManifests>::_var);
	constexpr static const auto _outputs = std::tuple_cat(First::_outputs, AddByN<OutputNextsSrc, NumManifests>::_var);

	constexpr static const auto _offsets = std::tuple_cat(First::_offsets, AddByN<OffsetNextsSrc, NumLinks>::_var);

	struct LinkTempSrc { constexpr static const auto _var = std::tuple_cat(First::_links, AddByN<LinkNextsSrc, NumManifests>::_var); };
	struct OffsetTempSrc { constexpr static const auto _var = _offsets; };
	struct OutputTempSrc { constexpr static const auto _var = _outputs; };

	constexpr static const uint32_t NumManifestsAccum = NumManifests + GetJunction<Nexts...>::NumManifestsAccum;
	constexpr static const uint32_t NumLinksAccum = NumLinks + GetJunction<Nexts...>::NumLinksAccum;
	constexpr static const uint32_t NumOutputsAccum = NumOutputs + GetJunction<Nexts...>::NumOutputsAccum;

	constexpr static const auto _links = MakeMergedOutput<NumManifestsAccum, NumLinksAccum, NumOutputsAccum, LinkTempSrc, OffsetTempSrc, OutputTempSrc>::_var;
	constexpr static const auto _precedingCount = First::_precedingCount;
};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes, typename... OtherArgumentTypes, typename... Nexts>
struct GetJunction<GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, std::tuple<OtherArgumentTypes...>, Nexts...>
	: GetJunction<GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, Nexts...>
{};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes>
struct GetJunction<GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>>
{
	using First = GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>;

	constexpr static const uint32_t NumManifestsAccum = NumManifests;
	constexpr static const uint32_t NumLinksAccum = NumLinks;
	constexpr static const uint32_t NumOutputsAccum = NumOutputs;

	constexpr static const auto _offsets = First::_offsets;
	constexpr static const auto _links = First::_links;
	constexpr static const auto _inputs = First::_inputs;
	constexpr static const auto _outputs = First::_outputs;
	constexpr static const auto _precedingCount = First::_precedingCount;
};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... OtherArgumentTypes, typename... Pathes>
struct GetJunction<GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, std::tuple<OtherArgumentTypes...>>
	: GetJunction<GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>>
{};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... Pathes>
struct GetNext<NumManifests, NumLinks, NumInputs, NumOutputs, false, Pathes...>
{
	constexpr static const auto _offsets = GetJunction<Pathes...>::_offsets;
	constexpr static const auto _links = GetJunction<Pathes...>::_links;
	constexpr static const auto _inputs = GetJunction<Pathes...>::_inputs;
	constexpr static const auto _outputs = GetJunction<Pathes...>::_outputs;
	constexpr static const auto _precedingCount = GetJunction<Pathes...>::_precedingCount;
};

template<typename T, uint32_t I>
struct __copyToVec
{
	constexpr static void func(uint32_t* out)
	{
		__copyToVec<T, I - 1>::func(out);
		out[I] = std::get<I>(T::_var);
	}
};


template<typename T>
struct __copyToVec<T, 0>
{
	constexpr static void func(uint32_t* out)
	{
		out[0] = std::get<0>(T::_var);
	}
};

template<typename T>
std::vector<uint32_t> copyToVec()
{
	constexpr uint32_t Size = std::tuple_size<decltype(T::_var)>::value;
	std::vector<uint32_t> vec(Size);
	__copyToVec<T, Size - 1>::func(vec.data());
	return vec;
}


#include <functional>
#include <array>

using TaskMeta = GetNext<1, 1, 1, 1, true>;

enum class TaskResult : uint32_t;
//using TaskExecution = std::function<TaskResult(uint32_t, uint32_t)>;

struct TaskDefine
{
	const char* _taskName;
	uint32_t _testValue[16]{};
	//TaskExecution _execution;
	//TaskExecutePoint _executePoint = {};
};


struct FunctionTag {};
struct LambdaTag {};

template<bool isCallableClass = true>
struct CallableAccessor { using Tag = LambdaTag; };

template<>
struct CallableAccessor<false> { using Tag = FunctionTag; };


template <typename RetType, typename ... ParamTypes>
constexpr std::integral_constant<unsigned, sizeof ...(ParamTypes)> getParameterCount(RetType(*f)(ParamTypes ...), FunctionTag)
{
	return std::integral_constant<unsigned, sizeof ...(ParamTypes)>{};
}

template <typename Type, typename RetType, typename ... ParamTypes>
constexpr std::integral_constant<unsigned, sizeof ...(ParamTypes)> getParameterCount(RetType(Type::*f)(ParamTypes ...), FunctionTag)
{
	return std::integral_constant<unsigned, sizeof ...(ParamTypes) + 1>{};
}

template <typename Type, typename RetType, typename ... ParamTypes>
constexpr std::integral_constant<unsigned, sizeof ...(ParamTypes)> getParameterCount(RetType(Type::* f)(ParamTypes ...) const, FunctionTag)
{
	return std::integral_constant<unsigned, sizeof ...(ParamTypes) + 1>{};
}

template<typename Lambda>
constexpr std::integral_constant<unsigned, lambda_details<std::remove_reference_t<Lambda>>::minimum_argument_count> getParameterCount(Lambda&& lambda, LambdaTag)
{
	return std::integral_constant<unsigned, lambda_details<std::remove_reference_t<Lambda>>::minimum_argument_count>{};
}

template <typename RetType, typename ... ParamTypes>
constexpr std::tuple<ParamTypes...> checkArgumentTypes(RetType(*f)(ParamTypes ...), ParamTypes... args)
{
	return std::tuple<ParamTypes...> {};
}

struct TaskWriter
{
	static auto task(const char* taskName)
	{
		return std::make_tuple(TaskDefine{ taskName }, TaskMeta{}, NullCallableSignature{});
	}

	template<typename Func, typename... ArgTypes>
	static auto task(Func&& func, ArgTypes&&... args)
	{
		return task<BindingKey_None>(std::forward<Func>(func), std::forward<ArgTypes>(args)...);
	}

	template<typename Key, typename Func, typename... ArgTypes>
	static auto task(Func&& func, ArgTypes&&... args)
	{
		//static_assert(decltype(getParameterCount(func, typename CallableAccessor<std::is_class_v<std::remove_reference_t<Func>>>::Tag{}))::value == sizeof...(ArgTypes), "Num of arguments are different for given task function");
		//static_assert(sizeof(decltype(checkArgumentTypes(func, std::forward<ArgTypes>(args)...))), "Arguments are not able to pass to task function");
		using CallableSignature = decltype(makeCallableSignature<Key>(std::forward<Func>(func), std::forward<ArgTypes>(args)...));
		return std::make_tuple(TaskDefine{}, TaskMeta{}, CallableSignature{});
	}

	template<typename... TaskList>
	static auto chain(TaskList&&... list);

	template<typename... TaskList>
	static auto junction(TaskList&&... list);
};

template<typename... TaskList>
struct CalcNumTaskManifests;

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes>
struct CalcNumTaskManifests<GetNext<M, N, NumInputs, NumOutputs, DefType, Pathes...>>
{
	constexpr static const uint32_t value = M;

	constexpr static const uint32_t numInputs = NumInputs;
	constexpr static const uint32_t numOutputs = NumOutputs;

	constexpr static const uint32_t sumInputsInJunction = NumInputs;
	constexpr static const uint32_t sumOutputsInJunction = NumOutputs;
};

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes, typename... TaskList>
struct CalcNumTaskManifests<GetNext<M, N, NumInputs, NumOutputs, DefType, Pathes...>, TaskList...>
{
	constexpr static const uint32_t value = M + CalcNumTaskManifests<TaskList...>::value;

	constexpr static const uint32_t numInputs = NumInputs;
	constexpr static const uint32_t numOutputs = CalcNumTaskManifests<TaskList...>::numOutputs;

	constexpr static const uint32_t sumInputsInJunction = NumInputs + CalcNumTaskManifests<TaskList...>::sumInputsInJunction;
	constexpr static const uint32_t sumOutputsInJunction = NumOutputs + CalcNumTaskManifests<TaskList...>::sumOutputsInJunction;
};


template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes, typename... OtherArgumentTypes>
struct CalcNumTaskManifests<GetNext<M, N, NumInputs, NumOutputs, DefType, Pathes...>, std::tuple<OtherArgumentTypes...>>
	: CalcNumTaskManifests<GetNext<M, N, NumInputs, NumOutputs, DefType, Pathes...>>
{};

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes, typename... OtherArgumentTypes, typename... TaskList>
struct CalcNumTaskManifests<GetNext<M, N, NumInputs, NumOutputs, DefType, Pathes...>, std::tuple<OtherArgumentTypes...>, TaskList...>
	: CalcNumTaskManifests<GetNext<M, N, NumInputs, NumOutputs, DefType, Pathes...>, TaskList...>
{};


template<typename... TaskList>
struct CalcNumNextTasks;

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes>
struct CalcNumNextTasks<GetNext<M, N, NumInputs, NumOutputs, DefType, Pathes...>> {
	constexpr static const uint32_t valueInChain = N; // M=1, NumIn/Out=1 -> N=1
	constexpr static const uint32_t valueInJunction = N;
};

template<
	uint32_t MCur, uint32_t NCur, uint32_t NumInputsCur, uint32_t NumOutputsCur, bool DefTypeCur, typename... PathesCur,
	uint32_t MNext, uint32_t NNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, bool DefTypeNext, typename... PathesNext, typename... TaskList>
struct CalcNumNextTasks<
	GetNext<MCur, NCur, NumInputsCur, NumOutputsCur, DefTypeCur, PathesCur...>,
	GetNext<MNext, NNext, NumInputsNext, NumOutputsNext, DefTypeNext, PathesNext...>, TaskList...>
{
	using First = GetNext<MCur, NCur, NumInputsCur, NumOutputsCur, DefTypeCur, PathesCur...>;
	using Second = GetNext<MNext, NNext, NumInputsNext, NumOutputsNext, DefTypeNext, PathesNext...>;

	constexpr static const uint32_t valueInChain = NCur + (NumOutputsCur * (NumInputsNext - 1)) + CalcNumNextTasks<Second, TaskList...>::valueInChain;
	constexpr static const uint32_t valueInJunction = NCur + CalcNumNextTasks<Second, TaskList...>::valueInJunction;
};

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes, typename... OtherArgumentTypes>
struct CalcNumNextTasks<GetNext<M, N, NumInputs, NumOutputs, DefType, Pathes...>, std::tuple<OtherArgumentTypes...>>
	: CalcNumNextTasks< GetNext<M, N, NumInputs, NumOutputs, DefType, Pathes...>>
{};

template<
	uint32_t MCur, uint32_t NCur, uint32_t NumInputsCur, uint32_t NumOutputsCur, bool DefTypeCur, typename... PathesCur, typename... OtherArgumentTypes,
	uint32_t MNext, uint32_t NNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, bool DefTypeNext, typename... PathesNext, typename... TaskList>
struct CalcNumNextTasks<
	GetNext<MCur, NCur, NumInputsCur, NumOutputsCur, DefTypeCur, PathesCur...>, std::tuple<OtherArgumentTypes...>,
	GetNext<MNext, NNext, NumInputsNext, NumOutputsNext, DefTypeNext, PathesNext...>, TaskList...>
	: CalcNumNextTasks<
	GetNext<MCur, NCur, NumInputsCur, NumOutputsCur, DefTypeCur, PathesCur...>,
	GetNext<MNext, NNext, NumInputsNext, NumOutputsNext, DefTypeNext, PathesNext...>, TaskList...>
{};


template<typename Tuple>
struct MakeTaskMetaChain;

template<typename... TaskMetaList>
struct MakeTaskMetaChain<std::tuple<TaskMetaList...>>
{
	constexpr static const uint32_t M = CalcNumTaskManifests<TaskMetaList...>::value;
	constexpr static const uint32_t N = CalcNumNextTasks<TaskMetaList...>::valueInChain;
	constexpr static const uint32_t NumInputs = CalcNumTaskManifests<TaskMetaList...>::numInputs;
	constexpr static const uint32_t NumOutputs = CalcNumTaskManifests<TaskMetaList...>::numOutputs;

	using TaskMetaChain = GetNext<M, N, NumInputs, NumOutputs, true, TaskMetaList...>;
};

template<typename Tuple>
using TaskMetaChain = typename MakeTaskMetaChain<Tuple>::TaskMetaChain;


template<typename Tuple>
struct MakeTaskMetaJunction;

template<typename... TaskMetaList>
struct MakeTaskMetaJunction<std::tuple<TaskMetaList...>>
{
	constexpr static const uint32_t M = CalcNumTaskManifests<TaskMetaList...>::value;
	constexpr static const uint32_t N = CalcNumNextTasks<TaskMetaList...>::valueInJunction;
	constexpr static const uint32_t NumInputs = CalcNumTaskManifests<TaskMetaList...>::sumInputsInJunction;
	constexpr static const uint32_t NumOutputs = CalcNumTaskManifests<TaskMetaList...>::sumOutputsInJunction;

	using TaskMetaJunction = GetNext<M, N, NumInputs, NumOutputs, false, TaskMetaList...>;
};

template<typename Tuple>
using TaskMetaJunction = typename MakeTaskMetaJunction<Tuple>::TaskMetaJunction;


template<typename T>
struct is_callable_signature : false_type {};

template<typename Callable, typename Ret, typename... Args>
struct is_callable_signature<CallableSignature<Callable, Ret, Args...>> : true_type {};

template<typename Key, typename Callable, typename Ret, typename... Args>
struct is_callable_signature<CallableSignatureWithKey<Key, Callable, Ret, Args...>> : is_callable_signature<CallableSignature<Callable, Ret, Args...>> {};

template<typename... Keys>
struct is_callable_signature<BindingKeyList<Keys...>> : true_type {};

template<typename T>
constexpr bool is_callable_signature_v = is_callable_signature<T>::value;

template<typename... TaskList>
struct SeparateTaskList
{
private:
	constexpr static const uint32_t IndexTaskDefine = 0;
	constexpr static const uint32_t IndexTaskMeta = 1;
	constexpr static const uint32_t IndexTaskCallable = 2;


private:
	template<typename T>
	constexpr static auto make_tuple_if_not_void(T&& arg) { return std::make_tuple(std::forward<T>(arg)); }

	constexpr static auto make_tuple_if_not_void(pseudo_void&& void_arg) { return tuple<> {}; }
	constexpr static auto make_tuple_if_not_void(pseudo_void& void_arg) { return tuple<> {}; }
	constexpr static auto make_tuple_if_not_void(const pseudo_void& void_arg) { return tuple<> {}; }

public:
	constexpr static auto getTaskDefines(TaskList&&... list) {
		return std::tuple_cat(make_tuple_if_not_void(std::get<IndexTaskDefine>(std::forward<TaskList>(list)))...);
	}

	using TaskMetaTuple = typename std::remove_const_t<decltype(
		std::tuple_cat(make_tuple_if_not_void(typename std::tuple_element_t<IndexTaskMeta, TaskList>{})...))>; /* Resolved(1): remove_const를 깜빡함. */

	using TaskCallableTuple = typename std::remove_const_t<decltype(
		std::tuple_cat(
			conditional_t<is_callable_signature_v<typename std::tuple_element_t<IndexTaskCallable, TaskList>>,
			decltype(std::make_tuple(typename std::tuple_element_t<IndexTaskCallable, TaskList>{})),
			typename std::tuple_element_t<IndexTaskCallable, TaskList >> {} ...)
		)>;
};

template<typename... TaskList>
constexpr auto getTaskDefines(TaskList&&... list) { return SeparateTaskList<TaskList...>::getTaskDefines(std::forward<TaskList>(list)...); }

template<typename... TaskList>
using TaskMetaTuple = typename SeparateTaskList<TaskList...>::TaskMetaTuple;

template<typename... TaskList>
using TaskCallableTuple = typename SeparateTaskList<TaskList...>::TaskCallableTuple;

struct TaskWriterUtil
{
private:
	template<typename TaskDefineTuple, uint32_t Max, uint32_t Index>
	struct __MoveTaskDefine
	{
		using TaskDefineArray = typename std::tuple_element<Index, TaskDefineTuple>::type;

		static void moveTo(TaskDefine* outDefines, TaskDefineTuple&& tuple)
		{
			auto&& defines = std::move(std::get<Index>(tuple));

			constexpr uint32_t M = std::tuple_size<TaskDefineArray>::value;
			for (uint32_t i = 0; i < M; ++i)
				outDefines[i] = std::move(defines[i]);

			__MoveTaskDefine<TaskDefineTuple, Max, Index + 1>::moveTo(outDefines + M, std::forward<TaskDefineTuple>(tuple));
		}
	};

	template<typename TaskDefineTuple, uint32_t Max>
	struct __MoveTaskDefine<TaskDefineTuple, Max, Max>
	{
		static void moveTo(TaskDefine* outDefines, TaskDefineTuple&& tuple) {}
	};

public:
	template<typename TaskDefineTuple>
	static void moveTaskDefine(TaskDefine* outDefines, TaskDefineTuple&& tuple)
	{
		constexpr const uint32_t Max = std::tuple_size<TaskDefineTuple>::value;
		return __MoveTaskDefine<TaskDefineTuple, Max, 0>::moveTo(outDefines, std::forward<TaskDefineTuple>(tuple));
	}
};

template<typename... CallableSignatures>
auto makeCallableInfoByTuple(tuple<CallableSignatures...>&& tuple)
{
	return makeCallableInfo(CallableSignatures{} ...);
}

template<typename... TaskList>
auto TaskWriter::chain(TaskList&&... list)
{
	makeCallableInfoByTuple(TaskCallableTuple<TaskList...>{});
	return std::make_tuple(getTaskDefines(std::forward<TaskList>(list)...), TaskMetaChain<TaskMetaTuple<TaskList...>>{}, TaskCallableTuple<TaskList...>{});
}

template<typename... TaskList>
auto TaskWriter::junction(TaskList&&... list)
{
	makeCallableInfoByTuple(TaskCallableTuple<TaskList...>{});
	return std::make_tuple(getTaskDefines(std::forward<TaskList>(list)...), TaskMetaJunction<TaskMetaTuple<TaskList...>>{}, TaskCallableTuple<TaskList...>{});
}


#include <stdio.h>