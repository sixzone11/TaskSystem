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

enum class TaskCommitFlag
{
	ExecuteLater = 0x00,
	ExecuteImmediately = 0x01,
	CreateAsSubTask = 0x02,

	Default = ExecuteLater,
};

struct Event;
struct TaskProcessor;

struct TaskDesc
{
	TaskProcessor* _taskProcessor;
	Event* _event;
	const char* _taskName;
	TaskCommitFlag _commitFlags = TaskCommitFlag::Default;
};


 ///////////////////////////////////////////////////////////////////////
 // TaskWriter

/**
 * #TaskDefineLayout
 *
 * TaskDefine - { { TaskDesc }, TaskNode, { TaskCallable } }
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

	template<typename... TaskDefineList>
	static auto taskDependency(TaskDefineList&&... list);

	template<typename... TaskDefineList>
	static auto taskConcurrency(TaskDefineList&&... list);
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
	std::vector<uint32_t> _taskUsed;
	std::vector<uint32_t> _offsets;
	std::vector<uint32_t> _links;
	std::vector<uint32_t> _inputs;
	std::vector<uint32_t> _outputs;
	std::vector<uint32_t> _precedingCount;
	std::vector<std::unique_ptr<ITask>> _taskKeys;
	std::vector<uint8_t> _returnTupleMemory;

	TaskCommitInfo(std::vector<TaskDesc>&& taskDescs, std::vector<uint32_t>&& offsets, std::vector<uint32_t>&& links, std::vector<uint32_t>&& inputs, std::vector<uint32_t>&& outputs, size_t sizeOfReturnTypeTuple)
		: _taskDescs(std::move(taskDescs))
		, _taskUsed(_taskDescs.size(), 0)
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
// MetaNode Family
//
///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
// MetaNode

// Desc(jiman): Delete ambiguity between <1,1,1,1,DefType> and <M,L,I,O,true/false>
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


template<typename... MetaNodeList> struct CalcNumMetaNodes;
template<typename... MetaNodeList> struct CalcNumMetaNodes_Next;

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

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes, typename... MetaNodeList>
struct CalcNumMetaNodes<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>, MetaNodeList...>
{
	constexpr static const uint32_t value = M + CalcNumMetaNodes<MetaNodeList...>::value;

	constexpr static const uint32_t numInputs = NumInputs;
	constexpr static const uint32_t numOutputs = CalcNumMetaNodes<MetaNodeList...>::numOutputs;

	constexpr static const uint32_t sumInputsInParallel = NumInputs + CalcNumMetaNodes<MetaNodeList...>::sumInputsInParallel;
	constexpr static const uint32_t sumOutputsInParallel = NumOutputs + CalcNumMetaNodes<MetaNodeList...>::sumOutputsInParallel;
};


template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes, typename... OtherArgumentTypes>
struct CalcNumMetaNodes<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>, std::tuple<OtherArgumentTypes...>>
	: CalcNumMetaNodes<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>>
{};

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes, typename... OtherArgumentTypes, typename... MetaNodeList>
struct CalcNumMetaNodes<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>, std::tuple<OtherArgumentTypes...>, MetaNodeList...>
	: CalcNumMetaNodes<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>, MetaNodeList...>
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
	uint32_t MNext, uint32_t NNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, bool DefTypeNext, typename... PathesNext, typename... MetaNodeList>
	struct CalcNumMetaNodes_Next<
	MetaNode<MCur, NCur, NumInputsCur, NumOutputsCur, DefTypeCur, PathesCur...>,
	MetaNode<MNext, NNext, NumInputsNext, NumOutputsNext, DefTypeNext, PathesNext...>, MetaNodeList...>
{
	using First = MetaNode<MCur, NCur, NumInputsCur, NumOutputsCur, DefTypeCur, PathesCur...>;
	using Second = MetaNode<MNext, NNext, NumInputsNext, NumOutputsNext, DefTypeNext, PathesNext...>;

	constexpr static const uint32_t valueInSerial = NCur + (NumOutputsCur * (NumInputsNext - 1)) + CalcNumMetaNodes_Next<Second, MetaNodeList...>::valueInSerial;
	constexpr static const uint32_t valueInParallel = NCur + CalcNumMetaNodes_Next<Second, MetaNodeList...>::valueInParallel;
};

template<uint32_t M, uint32_t N, uint32_t NumInputs, uint32_t NumOutputs, bool DefType, typename... Pathes, typename... OtherArgumentTypes>
struct CalcNumMetaNodes_Next<MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>, std::tuple<OtherArgumentTypes...>>
	: CalcNumMetaNodes_Next< MetaNode<M, N, NumInputs, NumOutputs, DefType, Pathes...>>
{};

template<
	uint32_t MCur, uint32_t NCur, uint32_t NumInputsCur, uint32_t NumOutputsCur, bool DefTypeCur, typename... PathesCur, typename... OtherArgumentTypes,
	uint32_t MNext, uint32_t NNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, bool DefTypeNext, typename... PathesNext, typename... MetaNodeList>
	struct CalcNumMetaNodes_Next<
	MetaNode<MCur, NCur, NumInputsCur, NumOutputsCur, DefTypeCur, PathesCur...>, std::tuple<OtherArgumentTypes...>,
	MetaNode<MNext, NNext, NumInputsNext, NumOutputsNext, DefTypeNext, PathesNext...>, MetaNodeList...>
	: CalcNumMetaNodes_Next<
	MetaNode<MCur, NCur, NumInputsCur, NumOutputsCur, DefTypeCur, PathesCur...>,
	MetaNode<MNext, NNext, NumInputsNext, NumOutputsNext, DefTypeNext, PathesNext...>, MetaNodeList...>
{};

constexpr const uint32_t IndexTaskDesc = 0;
constexpr const uint32_t IndexTaskGraph = 1;
constexpr const uint32_t IndexTaskCallable = 2;


///////////////////////////////////////////////////////////////////////
// TaskDefine

template<typename _TaskDefineTuple>
struct TaskDefine : public _TaskDefineTuple
{
	using AsTuple = _TaskDefineTuple;

	// Note(jiman): 단일 TaskDefine을 나타낼 때 TaskDesc와 TaskCallable은 단일 요소 튜플로 구성되어 있음. #TaskDefineLayout 참고.
	std::tuple_element_t<0, std::tuple_element_t<IndexTaskCallable, _TaskDefineTuple>> operator() ();
};

template<typename TupleDerived>
using as_tuple_t = typename std::remove_reference_t<TupleDerived>::AsTuple;

///////////////////////////////////////////////////////////////////////
// SeparateTaskDefineList

template<typename... TaskDefineList>
struct SeparateTaskDefineList
{
private:
	template<typename T>
	constexpr static auto make_tuple_if_not_void(T&& arg) { return std::make_tuple(std::forward<T>(arg)); }

	constexpr static auto make_tuple_if_not_void(pseudo_void&& void_arg) { return std::tuple<> {}; }
	constexpr static auto make_tuple_if_not_void(pseudo_void& void_arg) { return std::tuple<> {}; }
	constexpr static auto make_tuple_if_not_void(const pseudo_void& void_arg) { return std::tuple<> {}; }

public:
	constexpr static auto getTaskDescs(TaskDefineList&&... list) {
		return std::tuple_cat( std::get<IndexTaskDesc>(std::forward<as_tuple_t<TaskDefineList>>(list)) ...);
	}

	using TaskNodeTuple = typename std::remove_const_t<decltype(
		std::tuple_cat(make_tuple_if_not_void(typename std::tuple_element_t<IndexTaskGraph, as_tuple_t<TaskDefineList>>{})...)) > ; /* Resolved(1): remove_const를 깜빡함. */

	constexpr static auto getTaskCallableAsTuple(TaskDefineList&&... list) {
		return std::tuple_cat( std::get<IndexTaskCallable>(std::forward<as_tuple_t<TaskDefineList>>(list)) ... );
	}
};

template<typename... TaskDefineList>
constexpr auto getTaskDescs(TaskDefineList&&... list) { return SeparateTaskDefineList<TaskDefineList...>::getTaskDescs(std::forward<TaskDefineList>(list)...); }

template<typename... TaskDefineList>
using TaskNodeTuple = typename SeparateTaskDefineList<TaskDefineList...>::TaskNodeTuple;

template<typename... TaskDefineList>
constexpr auto getTaskCallables(TaskDefineList&&... list) { return SeparateTaskDefineList<TaskDefineList...>::getTaskCallableAsTuple(std::forward<TaskDefineList>(list)...); }


///////////////////////////////////////////////////////////////////////
//
// TaskWriter Definitions
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Simple TaskDefine

inline auto TaskWriter::task(const char* taskName)
{
#define __define_task_define_tuple \
std::make_tuple(std::make_tuple(TaskDesc{ nullptr, nullptr, taskName }), TaskNode{}, std::make_tuple(NullCallableSignature{}))
	return TaskDefine<decltype(__define_task_define_tuple)>{ __define_task_define_tuple };
#undef __define_task_define_tuple
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
	
#define __define_task_define_tuple \
std::make_tuple(std::make_tuple(TaskDesc{}), TaskNode{}, std::make_tuple(makeCallableSignature<BindingKeyT>(std::forward<Func>(func), std::forward<ArgTypes>(args)...)))
	return TaskDefine<decltype(__define_task_define_tuple)> { __define_task_define_tuple };
#undef __define_task_define_tuple
}

template<typename... TaskDefineList>
auto TaskWriter::taskDependency(TaskDefineList&&... list)
{
#define __define_task_define_tuple \
std::make_tuple(getTaskDescs(std::forward<TaskDefineList>(list)...), TaskNodeSerial<TaskNodeTuple<TaskDefineList...>>{}, getTaskCallables(std::forward<TaskDefineList>(list)...))
	return TaskDefine<decltype(__define_task_define_tuple)> { __define_task_define_tuple };
#undef __define_task_define_tuple
}

template<typename... TaskDefineList>
auto TaskWriter::taskConcurrency(TaskDefineList&&... list)
{
#define __define_task_define_tuple \
std::make_tuple(getTaskDescs(std::forward<TaskDefineList>(list)...), TaskNodeParallel<TaskNodeTuple<TaskDefineList...>>{}, getTaskCallables(std::forward<TaskDefineList>(list)...))
	return TaskDefine<decltype(__define_task_define_tuple)> { __define_task_define_tuple };
#undef __define_task_define_tuple
}

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

template<typename TaskDefineList>
inline std::vector<TaskDesc> moveDescsToVec(TaskDefineList&& taskDefineList)
{
	using TaskTuple = as_tuple_t<TaskDefineList>;
	using TaskGraph = typename std::tuple_element<IndexTaskGraph, TaskTuple>::type;

	std::vector<TaskDesc> vec(TaskGraph::NumNodes);
	__moveTaskDesc(vec.data(), std::get<IndexTaskDesc>(std::forward<TaskTuple>(taskDefineList)), std::make_integer_sequence<uint32_t, TaskGraph::NumNodes>{});
	return vec;
}

template<typename T, uint32_t... I>
constexpr static void __copyToVec(uint32_t* out, std::integer_sequence<uint32_t, I...>) { ((out[I] = std::get<I>(T::_var)), ...); }

template<typename T, uint32_t AdditionalSize = 0>
std::vector<uint32_t> copyToVec()
{
	constexpr uint32_t Size = std::tuple_size<decltype(T::_var)>::value;
	std::vector<uint32_t> vec(Size + AdditionalSize);
	__copyToVec<T>(vec.data(), std::make_integer_sequence<uint32_t, Size>{});
	return vec;
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

template<typename _TaskDefine>
void printTaskDefine(const _TaskDefine& taskDefine)
{
	using TaskDescTuple = typename std::tuple_element_t<IndexTaskDesc, as_tuple_t<_TaskDefine>>;
	using TaskGraph = typename std::tuple_element_t<IndexTaskGraph, as_tuple_t<_TaskDefine>>;
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