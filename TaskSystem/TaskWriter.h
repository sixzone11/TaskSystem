#pragma once

#include <stdint.h>
#include <tuple>
#include <vector>
#include <memory>
#include <iostream>
#include <utility>

#include "TaskCallableDescription.h"


///////////////////////////////////////////////////////////////////////
//
// TaskWriter
//
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
// TaskNode

template<uint32_t NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes> 
struct MetaNode;

using TaskNode = MetaNode<1, 1, 1, 1, true>;


///////////////////////////////////////////////////////////////////////
// TaskNodeSerial

template<typename Tuple> struct MakeMetaNodeSerial;

template<typename Tuple>
using TaskNodeSerial = typename MakeMetaNodeSerial<Tuple>::MetaNodeSerial;


///////////////////////////////////////////////////////////////////////
// TaskNodeParallel

template<typename Tuple> struct MakeMetaNodeParallel;

template<typename Tuple>
using TaskNodeParallel = typename MakeMetaNodeParallel<Tuple>::MetaNodeParallel;


///////////////////////////////////////////////////////////////////////
// TaskDesc

struct TaskDesc
{
	const char* _taskName;
};


 ///////////////////////////////////////////////////////////////////////
 // TaskWriter

/**
 * Task Define Layout
 *
 * TaskDefine - { TaskDesc, TaskNode, TaskCallable }
 * TaskDependencyDefine - { { TaskDefine, ... }, TaskNodeSerial, { TaskCallable, ...} }
 * TaskConcurrencyDefine - { { TaskDefine, ... }, TaskNodeParallel, { TaskCallable, ...} }
 */

struct TaskWriter
{
	inline static auto task(const char* taskName);

	// Note(jiman): concept을 쓸 수 없으니 비슷하게라도 만들고 싶은데...
	// #define TaskControl(T) std::enable_if_t<std::is_base_of_v<__Task_Control, T>, T>
	// #define TaskNoControl(T) std::enable_if_t<std::is_base_of_v<__Task_Control, T> == false, T>

	template<typename BindingKeyT = BindingKey_None, typename Expression0, typename Expression1, typename Func, typename... ArgTypes>
	static auto task(__Task_ConditionCancel, Expression0&& expression0, __Task_WaitWhile, Expression1&& expression1, Func&& func, ArgTypes&&... args) {
		return task<BindingKeyT>(std::forward<Func>(func), std::forward<ArgTypes>(args)...);
	}
	template<typename BindingKeyT = BindingKey_None, typename Expression0, typename Expression1, typename Func, typename... ArgTypes>
	static auto task(__Task_Condition, Expression0&& expression0, __Task_WaitWhile, Expression1&& expression1, Func&& func, ArgTypes&&... args) {
		return task<BindingKeyT>(std::forward<Func>(func), std::forward<ArgTypes>(args)...);
	}
	template<typename BindingKeyT = BindingKey_None, typename Expression0, typename Func, typename... ArgTypes>
	static auto task(__Task_Condition, Expression0&& expression0, Func&& func, ArgTypes&&... args) {
		return task<BindingKeyT>(std::forward<Func>(func), std::forward<ArgTypes>(args)...);
	}
	template<typename BindingKeyT = BindingKey_None, typename Expression0, typename Func, typename... ArgTypes>
	static auto task(__Task_ConditionCancel, Expression0&& expression0, Func&& func, ArgTypes&&... args) {
		return task<BindingKeyT>(std::forward<Func>(func), std::forward<ArgTypes>(args)...);
	}
	template<typename BindingKeyT = BindingKey_None, typename Expression0, typename Func, typename... ArgTypes>
	static auto task(__Task_WaitWhile, Expression0&& expression0, Func&& func, ArgTypes&&... args) {
		return task<BindingKeyT>(std::forward<Func>(func), std::forward<ArgTypes>(args)...);
	}

	template<typename BindingKeyT = BindingKey_None, typename Func, typename... ArgTypes>
	static auto task(Func&& func, ArgTypes&&... args);

	template<typename... TaskList>
	static auto taskDependency(TaskList&&... list);

	template<typename... TaskList>
	static auto taskConcurrency(TaskList&&... list);
};


///////////////////////////////////////////////////////////////////////
//
// TaskCommitInfo
//
///////////////////////////////////////////////////////////////////////

struct ITask;

struct TaskCommitInfo
{
	std::vector<TaskDesc> _taskDescs;
	std::vector<bool> _taskUsed;
	std::vector<uint32_t> _offsets;
	std::vector<uint32_t> _links;
	std::vector<uint32_t> _inputs;
	std::vector<uint32_t> _outputs;
	std::vector<uint32_t> _precedingCount;
	std::vector<std::unique_ptr<ITask>> _taskKeys;
	std::vector<uint8_t> _returnTupleMemory;

	TaskCommitInfo(std::vector<TaskDesc>&& taskDescs, std::vector<uint32_t>&& offsets, std::vector<uint32_t>&& links, std::vector<uint32_t>&& inputs, std::vector<uint32_t>&& outputs, size_t sizeOfReturnTypeTuple)
		: _taskDescs(std::move(taskDescs))
		, _taskUsed(_taskDescs.size(), false)
		, _offsets(std::move(offsets))
		, _links(std::move(links))
		, _inputs(std::move(inputs))
		, _outputs(std::move(outputs))
		, _precedingCount(_taskDescs.size() + 1, 1u)
		, _taskKeys(_taskDescs.size())
		, _returnTupleMemory(sizeOfReturnTypeTuple)
	{
		for (const uint32_t& link : _links)
			_precedingCount[link]++;

		_offsets.back() = uint32_t(_links.size());

		//_taskKeys.reserve(_taskDescs.size());
	}

	virtual ~TaskCommitInfo() {}
};

///////////////////////////////////////////////////////////////////////
//
// Utilities
//
///////////////////////////////////////////////////////////////////////

template<size_t... Iseq1, size_t... Iseq2>
static constexpr auto index_sequence_cat(std::index_sequence<Iseq1...>, std::index_sequence<Iseq2...>) { return std::index_sequence<Iseq1..., Iseq2...>{}; }

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

template<uint32_t NumNodes, uint32_t NumLinks, uint32_t NumOutputs, uint32_t NumInputsNext, typename Links, typename Offsets, typename Outputs, typename InputsNext>
struct MakeExpandedLink
{
private:
	static_assert(NumLinks == std::tuple_size<decltype(Links::_var)>::value, "Expected NumLinks is mismatched with Links");
	static_assert(NumOutputs == std::tuple_size<decltype(Outputs::_var)>::value, "Expected NumOutputs is mismatched with Outputs");
	static_assert(NumInputsNext == std::tuple_size<decltype(InputsNext::_var)>::value, "Expected NumInputsNext is mismatched with InputsNext");

	constexpr static const auto _linkedInputs = AddByN<InputsNext, NumNodes>::_var;

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

template<uint32_t NumNodes, uint32_t NumLinks, uint32_t NumOutputs, typename Links, typename Offsets, typename Outputs>
struct MakeMergedOutput
{
	template<typename Links_, uint32_t L, uint32_t O>
	struct __MakeMergedOutput
	{
		constexpr static const bool cond = (L != std::get<std::get<O>(Outputs::_var)>(Offsets::_var));
		constexpr static const auto _var = std::tuple_cat(
			std::make_tuple(cond ? std::get<L>(Links::_var) : NumNodes),
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


///////////////////////////////////////////////////////////////////////
//
// MetaNode Family
//
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
// MetaNode

// Delete ambiguity between <1,1,1,1,DefType> and <M,L,I,O,true/false>
template<>
struct MetaNode<1, 1, 1, 1, true>
{
public:
	constexpr static const std::tuple<uint32_t> _offsets{ 0 };
	constexpr static const std::tuple<uint32_t> _links{ 1 };
	constexpr static const std::tuple<uint32_t> _inputs{ 0 };
	constexpr static const std::tuple<uint32_t> _outputs{ 0 };
	constexpr static const std::tuple<uint32_t, uint32_t> _precedingCount{ 0, 1 };

	constexpr static const uint32_t NumNodes = 1;
};

template<>
struct MetaNode<1, 1, 1, 1, false> : MetaNode<1, 1, 1, 1, true> {};


template<typename... TaskList> struct CalcNumMetaNodes;
template<typename... TaskList> struct CalcNumMetaNodes_Next;

///////////////////////////////////////////////////////////////////////
// MetaNodeSerial

template<typename... MetaNodeList>
struct MakeMetaNodeSerial<std::tuple<MetaNodeList...>>
{
	constexpr static const uint32_t M = CalcNumMetaNodes<MetaNodeList...>::value;
	constexpr static const uint32_t N = CalcNumMetaNodes_Next<MetaNodeList...>::valueInSerial;
	constexpr static const uint32_t NumInputs = CalcNumMetaNodes<MetaNodeList...>::numInputs;
	constexpr static const uint32_t NumOutputs = CalcNumMetaNodes<MetaNodeList...>::numOutputs;

	using MetaNodeSerial = MetaNode<M, N, NumInputs, NumOutputs, true, MetaNodeList...>;
};


///////////////////////////////////////////////////////////////////////
// MetaNodeParallel

template<typename... MetaNodeList>
struct MakeMetaNodeParallel<std::tuple<MetaNodeList...>>
{
	constexpr static const uint32_t M = CalcNumMetaNodes<MetaNodeList...>::value;
	constexpr static const uint32_t N = CalcNumMetaNodes_Next<MetaNodeList...>::valueInParallel;
	constexpr static const uint32_t NumInputs = CalcNumMetaNodes<MetaNodeList...>::sumInputsInParallel;
	constexpr static const uint32_t NumOutputs = CalcNumMetaNodes<MetaNodeList...>::sumOutputsInParallel;

	using MetaNodeParallel = MetaNode<M, N, NumInputs, NumOutputs, false, MetaNodeList...>;
};


///////////////////////////////////////////////////////////////////////
// BuildMetaNodeSerial

template<typename First, typename... Nexts>
struct BuildMetaNodeSerial;

template<typename First, typename Second, typename... Nexts>
struct BuildMetaNodeSerial_Next;

template<
	uint32_t NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes,
	uint32_t NumNodesNext, uint32_t NumLinksNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, bool DepTypeNext, typename... PathesNext, typename... Nexts>
	struct BuildMetaNodeSerial_Next<
	MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>,
	MetaNode<NumNodesNext, NumLinksNext, NumInputsNext, NumOutputsNext, DepTypeNext, PathesNext...>, Nexts...>
{
	using First = MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>;
	using Second = MetaNode<NumNodesNext, NumLinksNext, NumInputsNext, NumOutputsNext, DepTypeNext, PathesNext...>;

	struct OutputFirstSrc { constexpr static const auto _var = First::_outputs; };
	struct LinkFirstSrc { constexpr static const auto _var = First::_links; };
	struct OffsetFirstSrc { constexpr static const auto _var = First::_offsets; };

	struct InputSecondSrc { constexpr static const auto _var = Second::_inputs; };

	struct OffsetBias { constexpr static const auto _var = MultiplyByN<MakeOffsetBias<NumNodes, OutputFirstSrc>, NumInputsNext - 1>::_var; };
	struct LinkedToNext { constexpr static const auto _var = MakeExpandedLink<NumNodes, NumLinks, NumOutputs, NumInputsNext, LinkFirstSrc, OffsetFirstSrc, OutputFirstSrc, InputSecondSrc>::_var; };

	constexpr static const uint32_t NumAddsToLink = NumLinks + NumOutputs * (NumInputsNext - 1);
};

template<
	uint32_t NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes, typename... OtherArgumentTypes,
	uint32_t NumNodesNext, uint32_t NumLinksNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, bool DepTypeNext, typename... PathesNext, typename... Nexts>
	struct BuildMetaNodeSerial_Next<
	MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, std::tuple<OtherArgumentTypes...>,
	MetaNode<NumNodesNext, NumLinksNext, NumInputsNext, NumOutputsNext, DepTypeNext, PathesNext...>, Nexts...>
	: BuildMetaNodeSerial_Next<
	MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>,
	MetaNode<NumNodesNext, NumLinksNext, NumInputsNext, NumOutputsNext, DepTypeNext, PathesNext...>, Nexts...>
{};

template<uint32_t NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes, typename... Nexts>
struct BuildMetaNodeSerial<MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, Nexts...>
{
	using First = MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>;

	struct OffsetFirstSrc { constexpr static const auto _var = First::_offsets; };

	struct LinkNextsSrc { constexpr static const auto _var = BuildMetaNodeSerial<Nexts...>::_links; };
	struct OutputNextsSrc { constexpr static const auto _var = BuildMetaNodeSerial<Nexts...>::_outputs; };
	struct OffsetNextsSrc { constexpr static const auto _var = BuildMetaNodeSerial<Nexts...>::_offsets; };

	constexpr static const auto _offsets = std::tuple_cat(AddTwo<OffsetFirstSrc, typename BuildMetaNodeSerial_Next<First, Nexts...>::OffsetBias>::_var, AddByN<OffsetNextsSrc, BuildMetaNodeSerial_Next<First, Nexts...>::NumAddsToLink>::_var);
	constexpr static const auto _links = std::tuple_cat(BuildMetaNodeSerial_Next<First, Nexts...>::LinkedToNext::_var, AddByN<LinkNextsSrc, NumNodes>::_var);

	constexpr static const auto _inputs = First::_inputs;
	constexpr static const auto _outputs = AddByN<OutputNextsSrc, NumNodes>::_var;
	constexpr static const auto _precedingCount = First::_precedingCount;
};

template<uint32_t NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes, typename... OtherArgumentTypes, typename... Nexts>
struct BuildMetaNodeSerial<MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, std::tuple<OtherArgumentTypes...>, Nexts...>
	: BuildMetaNodeSerial<MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, Nexts...>
{};

template<uint32_t NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes>
struct BuildMetaNodeSerial<MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>>
{
	using First = MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>;

	constexpr static const auto _offsets = First::_offsets;
	constexpr static const auto _links = First::_links;
	constexpr static const auto _inputs = First::_inputs;
	constexpr static const auto _outputs = First::_outputs;
	constexpr static const auto _precedingCount = First::_precedingCount;
};

template<uint32_t NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes, typename... OtherArgumentTypes>
struct BuildMetaNodeSerial<MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, std::tuple<OtherArgumentTypes...>>
	: BuildMetaNodeSerial<MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>>
{};

template<uint32_t _NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... Pathes>
struct MetaNode<_NumNodes, NumLinks, NumInputs, NumOutputs, true, Pathes...>
{
	constexpr static const auto _offsets = BuildMetaNodeSerial<Pathes...>::_offsets;
	constexpr static const auto _links = BuildMetaNodeSerial<Pathes...>::_links;
	constexpr static const auto _inputs = BuildMetaNodeSerial<Pathes...>::_inputs;
	constexpr static const auto _outputs = BuildMetaNodeSerial<Pathes...>::_outputs;
	constexpr static const auto _precedingCount = BuildMetaNodeSerial<Pathes...>::_precedingCount;

	constexpr static const uint32_t NumNodes = _NumNodes;
};


///////////////////////////////////////////////////////////////////////
// BuildMetaNodeParallel

template<typename First, typename... Nexts>
struct BuildMetaNodeParallel;

template<typename First, typename Second, typename... Nexts>
struct BuildMetaNodeParallel_Next;


template<uint32_t NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes, typename... Nexts>
struct BuildMetaNodeParallel<MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, Nexts...>
{
	using First = MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>;

	struct LinkNextsSrc { constexpr static const auto _var = BuildMetaNodeParallel<Nexts...>::_links; };
	struct OffsetNextsSrc { constexpr static const auto _var = BuildMetaNodeParallel<Nexts...>::_offsets; };
	struct InputNextsSrc { constexpr static const auto _var = BuildMetaNodeParallel<Nexts...>::_inputs; };
	struct OutputNextsSrc { constexpr static const auto _var = BuildMetaNodeParallel<Nexts...>::_outputs; };

	constexpr static const auto _inputs = std::tuple_cat(First::_inputs, AddByN<InputNextsSrc, NumNodes>::_var);
	constexpr static const auto _outputs = std::tuple_cat(First::_outputs, AddByN<OutputNextsSrc, NumNodes>::_var);

	constexpr static const auto _offsets = std::tuple_cat(First::_offsets, AddByN<OffsetNextsSrc, NumLinks>::_var);

	struct LinkTempSrc { constexpr static const auto _var = std::tuple_cat(First::_links, AddByN<LinkNextsSrc, NumNodes>::_var); };
	struct OffsetTempSrc { constexpr static const auto _var = _offsets; };
	struct OutputTempSrc { constexpr static const auto _var = _outputs; };

	constexpr static const uint32_t NumNodesAccum = NumNodes + BuildMetaNodeParallel<Nexts...>::NumNodesAccum;
	constexpr static const uint32_t NumLinksAccum = NumLinks + BuildMetaNodeParallel<Nexts...>::NumLinksAccum;
	constexpr static const uint32_t NumOutputsAccum = NumOutputs + BuildMetaNodeParallel<Nexts...>::NumOutputsAccum;

	constexpr static const auto _links = MakeMergedOutput<NumNodesAccum, NumLinksAccum, NumOutputsAccum, LinkTempSrc, OffsetTempSrc, OutputTempSrc>::_var;
	constexpr static const auto _precedingCount = First::_precedingCount;
};

template<uint32_t NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes, typename... OtherArgumentTypes, typename... Nexts>
struct BuildMetaNodeParallel<MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, std::tuple<OtherArgumentTypes...>, Nexts...>
	: BuildMetaNodeParallel<MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, Nexts...>
{};

template<uint32_t NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... Pathes>
struct BuildMetaNodeParallel<MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>>
{
	using First = MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>;

	constexpr static const uint32_t NumNodesAccum = NumNodes;
	constexpr static const uint32_t NumLinksAccum = NumLinks;
	constexpr static const uint32_t NumOutputsAccum = NumOutputs;

	constexpr static const auto _offsets = First::_offsets;
	constexpr static const auto _links = First::_links;
	constexpr static const auto _inputs = First::_inputs;
	constexpr static const auto _outputs = First::_outputs;
	constexpr static const auto _precedingCount = First::_precedingCount;
};

template<uint32_t NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, bool DepType, typename... OtherArgumentTypes, typename... Pathes>
struct BuildMetaNodeParallel<MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>, std::tuple<OtherArgumentTypes...>>
	: BuildMetaNodeParallel<MetaNode<NumNodes, NumLinks, NumInputs, NumOutputs, DepType, Pathes...>>
{};

template<uint32_t _NumNodes, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... Pathes>
struct MetaNode<_NumNodes, NumLinks, NumInputs, NumOutputs, false, Pathes...>
{
	constexpr static const auto _offsets = BuildMetaNodeParallel<Pathes...>::_offsets;
	constexpr static const auto _links = BuildMetaNodeParallel<Pathes...>::_links;
	constexpr static const auto _inputs = BuildMetaNodeParallel<Pathes...>::_inputs;
	constexpr static const auto _outputs = BuildMetaNodeParallel<Pathes...>::_outputs;
	constexpr static const auto _precedingCount = BuildMetaNodeParallel<Pathes...>::_precedingCount;

	constexpr static const uint32_t NumNodes = _NumNodes;
};


///////////////////////////////////////////////////////////////////////
// MetaGraphDesc

template<typename T>
struct MetaGraphDesc {
	struct Offsets { constexpr static const auto _var = T::_offsets; };
	struct Links { constexpr static const auto _var = T::_links; };
	struct Inputs { constexpr static const auto _var = T::_inputs; };
	struct Outputs { constexpr static const auto _var = T::_outputs; };
};


///////////////////////////////////////////////////////////////////////
// CalcNumMetaNodes

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes>
struct CalcNumMetaNodes<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>>
{
	constexpr static const uint32_t value = M;

	constexpr static const uint32_t numInputs = NumInputs;
	constexpr static const uint32_t numOutputs = NumOutputs;

	constexpr static const uint32_t sumInputsInParallel = NumInputs;
	constexpr static const uint32_t sumOutputsInParallel = NumOutputs;
};

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes, typename... TaskList>
struct CalcNumMetaNodes<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>, TaskList...>
{
	constexpr static const uint32_t value = M + CalcNumMetaNodes<TaskList...>::value;

	constexpr static const uint32_t numInputs = NumInputs;
	constexpr static const uint32_t numOutputs = CalcNumMetaNodes<TaskList...>::numOutputs;

	constexpr static const uint32_t sumInputsInParallel = NumInputs + CalcNumMetaNodes<TaskList...>::sumInputsInParallel;
	constexpr static const uint32_t sumOutputsInParallel = NumOutputs + CalcNumMetaNodes<TaskList...>::sumOutputsInParallel;
};


template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes, typename... OtherArgumentTypes>
struct CalcNumMetaNodes<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>, std::tuple<OtherArgumentTypes...>>
	: CalcNumMetaNodes<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>>
{};

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes, typename... OtherArgumentTypes, typename... TaskList>
struct CalcNumMetaNodes<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>, std::tuple<OtherArgumentTypes...>, TaskList...>
	: CalcNumMetaNodes<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>, TaskList...>
{};


///////////////////////////////////////////////////////////////////////
// CalcNumMetaNodes_Next

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes>
struct CalcNumMetaNodes_Next<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>> {
	constexpr static const uint32_t valueInSerial = N; // M=1, NumIn/Out=1 -> N=1
	constexpr static const uint32_t valueInParallel = N;
};

template<
	uint32_t MCur, uint32_t NCur, uint32_t NumInputsCur, uint32_t NumOutputsCur, bool DefTypeCur, typename... PathesCur,
	uint32_t MNext, uint32_t NNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, bool DefTypeNext, typename... PathesNext, typename... TaskList>
	struct CalcNumMetaNodes_Next<
	MetaNode<MCur, NCur, NumInputsCur, NumOutputsCur, DefTypeCur, PathesCur...>,
	MetaNode<MNext, NNext, NumInputsNext, NumOutputsNext, DefTypeNext, PathesNext...>, TaskList...>
{
	using First = MetaNode<MCur, NCur, NumInputsCur, NumOutputsCur, DefTypeCur, PathesCur...>;
	using Second = MetaNode<MNext, NNext, NumInputsNext, NumOutputsNext, DefTypeNext, PathesNext...>;

	constexpr static const uint32_t valueInSerial = NCur + (NumOutputsCur * (NumInputsNext - 1)) + CalcNumMetaNodes_Next<Second, TaskList...>::valueInSerial;
	constexpr static const uint32_t valueInParallel = NCur + CalcNumMetaNodes_Next<Second, TaskList...>::valueInParallel;
};

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes, typename... OtherArgumentTypes>
struct CalcNumMetaNodes_Next<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>, std::tuple<OtherArgumentTypes...>>
	: CalcNumMetaNodes_Next< MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>>
{};

template<
	uint32_t MCur, uint32_t NCur, uint32_t NumInputsCur, uint32_t NumOutputsCur, bool DefTypeCur, typename... PathesCur, typename... OtherArgumentTypes,
	uint32_t MNext, uint32_t NNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, bool DefTypeNext, typename... PathesNext, typename... TaskList>
	struct CalcNumMetaNodes_Next<
	MetaNode<MCur, NCur, NumInputsCur, NumOutputsCur, DefTypeCur, PathesCur...>, std::tuple<OtherArgumentTypes...>,
	MetaNode<MNext, NNext, NumInputsNext, NumOutputsNext, DefTypeNext, PathesNext...>, TaskList...>
	: CalcNumMetaNodes_Next<
	MetaNode<MCur, NCur, NumInputsCur, NumOutputsCur, DefTypeCur, PathesCur...>,
	MetaNode<MNext, NNext, NumInputsNext, NumOutputsNext, DefTypeNext, PathesNext...>, TaskList...>
{};


///////////////////////////////////////////////////////////////////////
// SeparateTaskList

constexpr const uint32_t IndexTaskDesc = 0;
constexpr const uint32_t IndexTaskGraph = 1;
constexpr const uint32_t IndexTaskCallable = 2;

template<typename... TaskList>
struct SeparateTaskList
{
private:
	template<typename T>
	constexpr static auto make_tuple_if_not_void(T&& arg) { return std::make_tuple(std::forward<T>(arg)); }

	constexpr static auto make_tuple_if_not_void(pseudo_void&& void_arg) { return std::tuple<> {}; }
	constexpr static auto make_tuple_if_not_void(pseudo_void& void_arg) { return std::tuple<> {}; }
	constexpr static auto make_tuple_if_not_void(const pseudo_void& void_arg) { return std::tuple<> {}; }

public:
	constexpr static auto getTaskDescs(TaskList&&... list) {
		return std::tuple_cat( std::get<IndexTaskDesc>(std::forward<TaskList>(list)) ...);
	}

	using TaskNodeTuple = typename std::remove_const_t<decltype(
		std::tuple_cat(make_tuple_if_not_void(typename std::tuple_element_t<IndexTaskGraph, TaskList>{})...)) > ; /* Resolved(1): remove_const를 깜빡함. */

	constexpr static auto getTaskCallableAsTuple(TaskList&&... list) {
		return std::tuple_cat( std::get<IndexTaskCallable>(std::forward<TaskList>(list)) ... );
	}
};

template<typename... TaskList>
constexpr auto getTaskDescs(TaskList&&... list) { return SeparateTaskList<TaskList...>::getTaskDescs(std::forward<TaskList>(list)...); }

template<typename... TaskList>
using TaskNodeTuple = typename SeparateTaskList<TaskList...>::TaskNodeTuple;

template<typename... TaskList>
constexpr auto getTaskCallables(TaskList&&... list) { return SeparateTaskList<TaskList...>::getTaskCallableAsTuple(std::forward<TaskList>(list)...); }


///////////////////////////////////////////////////////////////////////
//
// TaskWriter Definitions
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Simple TaskDefine

inline auto TaskWriter::task(const char* taskName)
{
	return std::make_tuple(std::make_tuple(TaskDesc{ taskName }), TaskNode{}, std::make_tuple(NullCallableSignature{}));
}


struct FunctionTag {};
struct LambdaTag {};

template<bool isCallableClass = true>
struct CallableAccessor { using Tag = LambdaTag; };

template<>
struct CallableAccessor<false> { using Tag = FunctionTag; };

template <typename RetType, typename ... ParamTypes>
constexpr auto getParameterCount(RetType(*f)(ParamTypes ...), FunctionTag) { return std::integral_constant<unsigned, sizeof ...(ParamTypes)>{}; }

template <typename Type, typename RetType, typename ... ParamTypes>
constexpr auto getParameterCount(RetType(Type::* f)(ParamTypes ...), FunctionTag) { return std::integral_constant<unsigned, sizeof ...(ParamTypes) + 1>{}; }

template <typename Type, typename RetType, typename ... ParamTypes>
constexpr auto getParameterCount(RetType(Type::* f)(ParamTypes ...) const, FunctionTag) { return std::integral_constant<unsigned, sizeof ...(ParamTypes) + 1>{}; }

template<typename Lambda>
constexpr auto getParameterCount(Lambda&& lambda, LambdaTag) { return std::integral_constant<unsigned, lambda_details<std::remove_reference_t<Lambda>>::minimum_argument_count>{}; }

template <typename RetType, typename ... ParamTypes>
constexpr auto checkArgumentTypes(RetType(*f)(ParamTypes ...), ParamTypes... args) { return std::tuple<ParamTypes...> {}; }

template<typename BindingKeyT, typename Func, typename... ArgTypes>
auto TaskWriter::task(Func&& func, ArgTypes&&... args)
{
	//static_assert(decltype(getParameterCount(func, typename CallableAccessor<std::is_class_v<std::remove_reference_t<Func>>>::Tag{}))::value == sizeof...(ArgTypes), "Num of arguments are different for given task function");
	//static_assert(sizeof(decltype(checkArgumentTypes(func, std::forward<ArgTypes>(args)...))), "Arguments are not able to pass to task function");
	// using CallableSignature = decltype(makeCallableSignature<BindingKeyT>(std::forward<Func>(func), std::forward<ArgTypes>(args)...));
	// return std::make_tuple(TaskDesc{}, TaskNode{}, CallableSignature{});
	return std::make_tuple(std::make_tuple(TaskDesc{}), TaskNode{}, std::make_tuple(makeCallableSignature<BindingKeyT>(std::forward<Func>(func), std::forward<ArgTypes>(args)...)));
}

template<typename... TaskList>
auto TaskWriter::taskDependency(TaskList&&... list)
{
	return std::make_tuple(getTaskDescs(std::forward<TaskList>(list)...), TaskNodeSerial<TaskNodeTuple<TaskList...>>{}, getTaskCallables(std::forward<TaskList>(list)...));
}

template<typename... TaskList>
auto TaskWriter::taskConcurrency(TaskList&&... list)
{
	return std::make_tuple(getTaskDescs(std::forward<TaskList>(list)...), TaskNodeParallel<TaskNodeTuple<TaskList...>>{}, getTaskCallables(std::forward<TaskList>(list)...));
}

#define GetResultOfTask(Task)		std::get<find_type_in_tuple<true, std::tuple_element_t<0, std::remove_reference_t<decltype(std::get<IndexTaskCallable>(Task))>>::KeyType, decltype(info)>::value>(resultTuple)


///////////////////////////////////////////////////////////////////////
//
// TaskDefine Move/Copy Utilities
//
///////////////////////////////////////////////////////////////////////

template<typename TaskDescTuple, uint32_t... I, std::enable_if_t<sizeof...(I) == 0, int> = 0>
constexpr static void __moveTaskDesc(TaskDesc* outDefines, TaskDescTuple&& tuple, std::integer_sequence<uint32_t, I...>) {}

template<typename TaskDescTuple, uint32_t... I, std::enable_if_t<sizeof...(I) != 0, int> = 0>
constexpr static void __moveTaskDesc(TaskDesc* outDefines, TaskDescTuple&& tuple, std::integer_sequence<uint32_t, I...>)
{
	((outDefines[I] = std::get<I>(std::forward<TaskDescTuple>(tuple))), ...);
}

template<typename TaskInfoList>
inline std::vector<TaskDesc> moveDefinesToVec(TaskInfoList&& taskInfoList)
{
	using TaskTuple = std::remove_reference_t<TaskInfoList>;
	using TaskGraph = typename std::tuple_element<IndexTaskGraph, TaskTuple>::type;

	std::vector<TaskDesc> vec(TaskGraph::NumNodes);
	__moveTaskDesc(vec.data(), std::get<IndexTaskDesc>(std::forward<TaskInfoList>(taskInfoList)), std::make_integer_sequence<uint32_t, TaskGraph::NumNodes>{});
	return vec;
}

template<typename T, uint32_t... I>
constexpr static void __copyToVec(uint32_t* out, std::integer_sequence<uint32_t, I...>) { ((out[I] = std::get<I>(T::_var)), ...); }

template<typename T, uint32_t AdditionalSize = 0>
std::vector<uint32_t> copyToVec()
{
	constexpr uint32_t Size = std::tuple_size<decltype(T::_var)>::value;
	std::vector<uint32_t> vec(Size + AdditionalSize);
	__copyToVec<T>(vec.data(), std::make_integer_sequence<uint32_t, Size - 1>{});
	return vec;
}


///////////////////////////////////////////////////////////////////////
//
// Tuple Utilities
//
///////////////////////////////////////////////////////////////////////

template<typename T>
void printTupleElement(const T& t);

template<class Tuple, std::size_t N>
struct TuplePrinter
{
	static void print(const Tuple& t)
	{
		TuplePrinter<Tuple, N - 1>::print(t);
		printTupleElement(std::get<N - 1>(t));
	}
};

template<class Tuple>
struct TuplePrinter<Tuple, 1>
{
	static void print(const Tuple& t)
	{
		printTupleElement(std::get<0>(t));
	}
};

template<typename... Args, std::enable_if_t<sizeof...(Args) == 0, int> = 0>
void printTupleElement(const std::tuple<Args...>& t)
{
	std::cout << "()\n";
}

template<typename... Args, std::enable_if_t<sizeof...(Args) != 0, int> = 0>
void printTupleElement(const std::tuple<Args...>& t)
{
	std::cout << "(";
	TuplePrinter<decltype(t), sizeof...(Args)>::print(t);
	std::cout << ")\n";
}

///////////////////////////////////////////////////////////////////////
// Print Task

static inline void print(std::vector<uint32_t>& v, const char* name)
{
	printf("%s: %zu\n\t[ ", name, v.size());
	for (auto& e : v)
	{
		printf("%u ", e);
	}
	printf("]\n");
}

inline static void printTupleElement(const TaskDesc& t)
{
	std::cout << "(" << t._taskName << ")\n";
}

template<typename TaskDefine>
void printTaskDefine(const TaskDefine& taskDefine)
{
	using TaskDescTuple = typename std::tuple_element<IndexTaskDesc, TaskDefine>::type;
	using TaskGraph = typename std::tuple_element<IndexTaskGraph, TaskDefine>::type;
	const auto& taskDescs = std::get<IndexTaskDesc>(taskDefine);

	auto offsets = copyToVec<typename MetaGraphDesc<TaskGraph>::Offsets>();
	auto links = copyToVec<typename MetaGraphDesc<TaskGraph>::Links>();
	auto inputs = copyToVec<typename MetaGraphDesc<TaskGraph>::Inputs>();
	auto outputs = copyToVec<typename MetaGraphDesc<TaskGraph>::Outputs>();

	print(offsets, "Offsets");
	print(links, "Links");
	print(inputs, "Inputs");
	print(outputs, "Outputs");
	printTupleElement(taskDescs);
}