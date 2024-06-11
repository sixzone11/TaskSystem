#pragma once

#define REPORT_MEMORY_LAYOUTS

#ifdef _WIN32
#ifdef TASKSYSTEM_EXPORTS
#define TASKSYSTEM_API __declspec(dllexport)
#else
#define TASKSYSTEM_API __declspec(dllimport)
#endif

#else
#define TASKSYSTEM_API
#define sprintf_s sprintf
#define swprintf_s swprintf

#endif

#include "TaskWriter.h"

struct ITask
{
	ITask() = default;
	ITask(std::shared_ptr<TaskCommitInfo>& commitInfo)
		: _commitInfo(commitInfo) {}
	virtual ~ITask() {}

	virtual void process() = 0;
	virtual void process() const = 0;

	friend TASKSYSTEM_API std::ostream& operator<<(std::ostream& os, const ITask& taskKey);

protected:
	std::shared_ptr<TaskCommitInfo> _commitInfo;
};
TASKSYSTEM_API std::ostream& operator<<(std::ostream& os, const ITask& taskKey);

struct ITaskManager
{
public:
	template<typename TaskDefineList>
	ITask* createTask(TaskDefineList&& taskDefineList);
	
	virtual bool commitTask(ITask* taskKey) const = 0;

	virtual ~ITaskManager() {}

private:
	//virtual TaskExecutePoint createTaskExecutePoint(TaskExecutePointDesc&& taskExecutePointDesc) = 0;
};

extern "C" TASKSYSTEM_API ITaskManager * getDefaultTaskManager();

template<typename CallableInfoType>
struct TaskCommitInfoWithCallable;

template<typename TaskDefineList>
inline void createCallableTasks(std::vector<std::unique_ptr<ITask>>& outTaskKeys, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, TaskDefineList&& taskDefineList);

template<typename TaskDefineList>
ITask* ITaskManager::createTask(TaskDefineList&& taskDefineList)
{
	using TaskTuple = std::remove_reference_t<TaskDefineList>;
	using TaskGraph = typename std::tuple_element_t<IndexTaskGraph, TaskTuple>;
	using CallableInfoType = decltype(decl_CallableInfoByTuple(std::declval<typename std::tuple_element_t<IndexTaskCallable, TaskDefineList>>()));

	auto taskCommitInfo = std::shared_ptr<TaskCommitInfo>(
		new TaskCommitInfoWithCallable<CallableInfoType>(
			moveDescsToVec(std::forward<TaskDefineList>(taskDefineList)),
			copyToVec<typename MetaGraphDesc<TaskGraph>::Offsets, 1>(),
			copyToVec<typename MetaGraphDesc<TaskGraph>::Links>(),
			copyToVec<typename MetaGraphDesc<TaskGraph>::Inputs>(),
			copyToVec<typename MetaGraphDesc<TaskGraph>::Outputs>()
		)
	);

	auto& taskKeys = taskCommitInfo->_taskKeys;
	createCallableTasks(taskKeys, taskCommitInfo, std::forward<TaskDefineList>(taskDefineList));

	return taskKeys.back().get();
}

template<typename CallableInfoType>
struct TaskCommitInfoWithCallable : TaskCommitInfo
{
	using ReturnTypeTuple = typename CallableInfoType::ReturnTypeTuple;

	TaskCommitInfoWithCallable(std::vector<TaskDesc>&& taskDescs, std::vector<uint32_t>&& offsets, std::vector<uint32_t>&& links, std::vector<uint32_t>&& inputs, std::vector<uint32_t>&& outputs)
		: TaskCommitInfo(std::move(taskDescs), std::move(offsets), std::move(links), std::move(inputs), std::move(outputs), sizeof(ReturnTypeTuple))
	{}

	~TaskCommitInfoWithCallable() override
	{
		auto& returnTuple = *static_cast<ReturnTypeTuple*>(static_cast<void*>(_returnTupleMemory.data()));
		deleteReturns(returnTuple, std::make_index_sequence<std::tuple_size_v<ReturnTypeTuple>>{});
	}

	template<size_t... Iseq>
	void deleteReturns(ReturnTypeTuple& returnTuple, std::integer_sequence<size_t, Iseq...>)
	{
		(deleteReturn<Iseq>(std::get<Iseq>(returnTuple)), ...);
	}

	template<size_t I, typename ReturnType>
	void deleteReturn(ReturnType& returnRef) { if (_taskUsed[I]) returnRef.~ReturnType(); }
};

template<typename CallableInfoType, size_t I, typename Callable, typename SelectTaskIdentifier>
struct CallableTask;

template<typename CallableInfoType, size_t I, typename CallableSignatureT>
constexpr std::size_t size_CallableTask()
{
	using SelectTaskIdentifier = std::conditional_t<CallableSignatureT::is_resolved, DefaultTaskIdentifier, LambdaTaskIdentifier>;
	return sizeof(CallableTask<CallableInfoType, I, typename CallableSignatureT::Callable, SelectTaskIdentifier>);
}

template<typename CallableInfoType, size_t I, typename CallableSignatureT>
inline auto make_CallableTask(CallableSignatureT&& collableSignature, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, typename std::tuple_element_t<I, typename CallableInfoType::CallableSignatureResolvedTuple>::RetType& returnReference)
{
	using SelectTaskIdentifier = std::conditional_t<CallableSignatureT::is_resolved, DefaultTaskIdentifier, LambdaTaskIdentifier>;
	return new CallableTask<CallableInfoType, I, typename CallableSignatureT::Callable, SelectTaskIdentifier>(std::forward<CallableSignatureT>(collableSignature), taskCommitInfo, returnReference);
}

template<typename TaskCallableTuple, uint32_t... I, std::enable_if_t<sizeof...(I) == 0, int> E = -2>
inline void __createCallableTasks(std::unique_ptr<ITask>* outCallableTasks, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, TaskCallableTuple&& tuple, std::integer_sequence<uint32_t, I...>) {}

template<typename TaskCallableTuple, uint32_t... I, std::enable_if_t<sizeof...(I) != 0, int> E = -1>
inline void __createCallableTasks(std::unique_ptr<ITask>* outCallableTasks, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, TaskCallableTuple&& tuple, std::integer_sequence<uint32_t, I...>)
{
	using CallableInfoType = decltype(decl_CallableInfoByTuple(std::declval<TaskCallableTuple>()));

	using ReturnTypeTuple = typename CallableInfoType::ReturnTypeTuple;
	auto& returnTuple = *static_cast<ReturnTypeTuple*>(static_cast<void*>(taskCommitInfo->_returnTupleMemory.data()));

#if defined(REPORT_MEMORY_LAYOUTS)
	constexpr size_t sizeOfAllTasks = (size_CallableTask<CallableInfoType, I, std::tuple_element_t<I, TaskCallableTuple>>() + ...);

	static const bool once = [&taskCommitInfo]() {
		std::cout
			<< "TaskType: " << std::endl
			<< "TaskCommitInfo Size: " << sizeof(TaskCommitInfo) << std::endl
			<< "Task Descriptions Size: " << taskCommitInfo->_taskDescs.capacity() * sizeof(taskCommitInfo->_taskDescs[0]) << std::endl
			<< "Task Used Size: " << taskCommitInfo->_taskUsed.capacity() * sizeof(taskCommitInfo->_taskUsed[0]) << std::endl
			<< "Task Node Offsets Size: " << taskCommitInfo->_offsets.capacity() * sizeof(taskCommitInfo->_offsets[0]) << std::endl
			<< "Task Node Links Size: " << taskCommitInfo->_links.capacity() * sizeof(taskCommitInfo->_links[0]) << std::endl
			<< "Task Node Outputs Size: " << taskCommitInfo->_outputs.capacity() * sizeof(taskCommitInfo->_outputs[0]) << std::endl
			<< "Task Node Preceding Count Size: " << taskCommitInfo->_precedingCount.capacity() * sizeof(taskCommitInfo->_precedingCount[0]) << std::endl
			<< "Task Keys Size: " << taskCommitInfo->_taskKeys.capacity() * sizeof(taskCommitInfo->_taskKeys[0]) << std::endl
			<< "Task Argument Tuple Size: " << taskCommitInfo->_returnTupleMemory.capacity() * sizeof(taskCommitInfo->_returnTupleMemory[0]) << std::endl
			<< "Allocated All Task Sizes: " << sizeOfAllTasks << std::endl
			<< std::endl;
		return true;
	} ();
#endif // REPORT_MEMORY_LAYOUTS

	(outCallableTasks[I].reset(make_CallableTask<CallableInfoType, I>(std::get<I>(std::forward<TaskCallableTuple>(tuple)), taskCommitInfo, std::get<I>(returnTuple))), ...);
}

template<typename TaskDefineList>
inline void createCallableTasks(std::vector<std::unique_ptr<ITask>>& outTaskKeys, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, TaskDefineList&& taskDefineList)
{
	using TaskTuple = std::remove_reference_t<TaskDefineList>;
	using TaskGraph = typename std::tuple_element_t<IndexTaskGraph, TaskTuple>;

	__createCallableTasks(outTaskKeys.data(), taskCommitInfo, std::get<IndexTaskCallable>(std::forward<TaskDefineList>(taskDefineList)), std::make_integer_sequence<uint32_t, TaskGraph::NumNodes>{});
}

template<typename CallableInfoType, std::size_t I, typename Callable>
struct CallableTask<CallableInfoType, I, Callable, LambdaTaskIdentifier> : public ITask
{
	using CallableSignatureResolved = typename std::tuple_element_t<I, typename CallableInfoType::CallableSignatureResolvedTuple>;

	using ReturnTypeTuple = typename CallableInfoType::ReturnTypeTuple;
	using KeyTypeTuple = typename CallableInfoType::KeyTypeTuple;

	//using Callable = typename CallableSignatureResolved::Callable;
	using RetType = typename CallableSignatureResolved::RetType;

	template<typename CallableSignature>
	CallableTask(CallableSignature&& callableSignature, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, RetType& ret)
		: ITask(taskCommitInfo)
		, _callable(std::forward<Callable>(callableSignature._callable))
		, _ret(ret)
	{
		//static_assert(is_same_v<LambdaTaskIdentifier, tuple_element_t<1, ParamTypeTuple>>, "This CallableSignature is not substituted!");
	}

	~CallableTask() override {}

	void process() override { invoke(is_void_return_type{}); }
	void process() const override { invoke(is_void_return_type{}); }

private:
	using is_void_return_type = std::conditional_t<is_pseudo_void_v<RetType>, std::true_type, std::false_type>;

	void invoke(std::false_type)
	{
		auto& returnTuple = *static_cast<ReturnTypeTuple*>(static_cast<void*>(_commitInfo->_returnTupleMemory.data()));
		new (&_ret) RetType(_callable(LambdaTaskIdentifier{}, KeyTypeTuple{}, std::move(returnTuple)));
	}
	
	void invoke(std::false_type) const
	{
		auto& returnTuple = *static_cast<ReturnTypeTuple*>(static_cast<void*>(_commitInfo->_returnTupleMemory.data()));
		new (&_ret) RetType(_callable(LambdaTaskIdentifier{}, KeyTypeTuple{}, std::move(returnTuple)));
	}

	void invoke(std::true_type)
	{
		auto& returnTuple = *static_cast<ReturnTypeTuple*>(static_cast<void*>(_commitInfo->_returnTupleMemory.data()));
		_callable(LambdaTaskIdentifier{}, KeyTypeTuple{}, std::move(returnTuple));
	}

	void invoke(std::true_type) const
	{
		auto& returnTuple = *static_cast<ReturnTypeTuple*>(static_cast<void*>(_commitInfo->_returnTupleMemory.data()));
		_callable(LambdaTaskIdentifier{}, KeyTypeTuple{}, std::move(returnTuple));
	}


private:
	Callable _callable;
	RetType& _ret;
};

template<typename CallableInfoType, size_t I, typename Callable>
struct CallableTask<CallableInfoType, I, Callable, DefaultTaskIdentifier> : public ITask
{
	// CallableInfoType
	using CallableSignatureResolved = typename std::tuple_element_t<I, typename CallableInfoType::CallableSignatureResolvedTuple>;

	using ReturnTypeTuple = typename CallableInfoType::ReturnTypeTuple;
	using KeyTypeTuple = typename CallableInfoType::KeyTypeTuple;

	using RetType = typename CallableSignatureResolved::RetType;
	using ArgTypeTupleResolved = typename CallableSignatureResolved::ArgTypeTuple;

	template<typename CallableSignature>
	CallableTask(CallableSignature&& callableSignature, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, RetType& ret)
		: ITask(taskCommitInfo)
		, _callable(std::forward<Callable>(callableSignature._callable))
		, _args(mapArgTuple(
			std::forward<typename CallableSignature::ArgTypeTuple>(callableSignature._args),
			typename CallableSignature::BindingSlotIndexSequence(),
			getOrderedBindingKeyIndexSequence<CallableSignature>(),
			*static_cast<ReturnTypeTuple*>(static_cast<void*>(taskCommitInfo->_returnTupleMemory.data()))))
		, _ret(ret)
	{
		static_assert(std::is_same_v<typename CallableSignature::Callable, Callable>, "'Callable's passed and given are must be same");
		static_assert(std::is_same_v<typename CallableSignature::Callable, typename CallableSignatureResolved::Callable>, "'Callable's passed and resolved are must be same");
	}
	~CallableTask() override {}

	void process() override
	{
		new (&_ret) RetType(delegate_call(is_void_return_type{}, is_member_function_type{}, std::make_integer_sequence<uint32_t, std::tuple_size_v<ArgTypeTupleResolved>>{}));
	}
	void process() const override
	{
		new (&_ret) RetType(delegate_call(is_void_return_type{}, is_member_function_type{}, std::make_integer_sequence<uint32_t, std::tuple_size_v<ArgTypeTupleResolved>>{}));
	}


private:

	using is_void_return_type = std::conditional_t<is_pseudo_void_v<RetType>, std::true_type, std::false_type>;
	using is_member_function_type = std::conditional_t<std::is_member_function_pointer_v<Callable>, std::true_type, std::false_type>;

	template<uint32_t... Iseq>
	auto delegate_call(std::false_type, std::false_type, std::integer_sequence<uint32_t, Iseq...>) { return _callable(std::get<Iseq>(_args)...); }

	template<uint32_t... Iseq>
	auto delegate_call(std::false_type, std::false_type, std::integer_sequence<uint32_t, Iseq...>) const { return _callable(std::get<Iseq>(_args)...); }

	template<uint32_t Izero, uint32_t... Iseq>
	auto delegate_call(std::false_type, std::true_type, std::integer_sequence<uint32_t, Izero, Iseq...>) { return (std::get<Izero>(_args)->*_callable)(std::get<Iseq>(_args)...); }

	template<uint32_t Izero, uint32_t... Iseq>
	auto delegate_call(std::false_type, std::true_type, std::integer_sequence<uint32_t, Izero, Iseq...>) const { return (std::get<Izero>(_args)->*_callable)(std::get<Iseq>(_args)...); }

	template<uint32_t... Iseq>
	auto delegate_call(std::true_type, std::false_type, std::integer_sequence<uint32_t, Iseq...>) { _callable(std::get<Iseq>(_args)...); return pseudo_void{}; }

	template<uint32_t... Iseq>
	auto delegate_call(std::true_type, std::false_type, std::integer_sequence<uint32_t, Iseq...>) const { _callable(std::get<Iseq>(_args)...); return pseudo_void{}; }

	template<uint32_t Izero, uint32_t... Iseq>
	auto delegate_call(std::true_type, std::true_type, std::integer_sequence<uint32_t, Izero, Iseq...>) { return (std::get<Izero>(_args)->*_callable)(std::get<Iseq>(_args)...); }

	template<uint32_t Izero, uint32_t... Iseq>
	auto delegate_call(std::true_type, std::true_type, std::integer_sequence<uint32_t, Izero, Iseq...>) const { return (std::get<Izero>(_args)->*_callable)(std::get<Iseq>(_args)...); }


private:

	template<typename... ArgsGiven, size_t... BindingSlotIndex, size_t... OrderedBindingKeyIndex>
	static ArgTypeTupleResolved mapArgTuple(std::tuple<ArgsGiven...>&& givenArgTuple, std::index_sequence<BindingSlotIndex...>, std::index_sequence<OrderedBindingKeyIndex...>, ReturnTypeTuple& returnTuple)
	{
		using MatchedKeyIndexSequence = decltype(makeMatchedKeyIndexSequence(std::make_index_sequence<sizeof...(ArgsGiven)>(), std::index_sequence<BindingSlotIndex...>(), std::index_sequence<OrderedBindingKeyIndex...>()));
		return resolveArgTuple(std::forward<std::tuple<ArgsGiven...>>(givenArgTuple), std::make_index_sequence<sizeof...(ArgsGiven)>(), MatchedKeyIndexSequence(), returnTuple);
	}

	template<typename ArgTypeTupleGiven, size_t... ArgIndices, size_t... KeyIndices>
	static ArgTypeTupleResolved resolveArgTuple(ArgTypeTupleGiven&& givenArgTuple, std::index_sequence<ArgIndices...>, std::index_sequence<KeyIndices...>, ReturnTypeTuple& returnTuple)
	{
		return std::forward_as_tuple((ArgumentSelector<ArgIndices, KeyIndices>::forwardOrReference(std::forward<ArgTypeTupleGiven>(givenArgTuple), returnTuple)) ...);
	}

	template<size_t ArgIndex, size_t KeyIndex>
	struct ArgumentSelector
	{
		template<typename ArgTypeTuple>
		static auto& forwardOrReference(ArgTypeTuple&&, ReturnTypeTuple& returnTuple) { return std::get<KeyIndex>(returnTuple); }
	};

	template<size_t ArgIndex>
	struct ArgumentSelector<ArgIndex, size_t(-1)>
	{
		template<typename ArgTypeTuple>
		static auto forwardOrReference(ArgTypeTuple&& givenArgTuple, ReturnTypeTuple&)
		{
			return std::forward<std::tuple_element_t<ArgIndex, ArgTypeTuple>>(std::get<ArgIndex>(std::forward<ArgTypeTuple>(givenArgTuple)));
		}
	};

	template<typename CallableSignatureT>
	static auto getOrderedBindingKeyIndexSequence()
	{
		using OrderedBindingSlotArgTypeTupleT = decltype(mapTuple(std::declval<typename CallableSignatureT::ArgTypeTuple>(), std::declval<typename CallableSignatureT::BindingSlotIndexSequence>()));
		using OrderedBindingKeyIndexSequence = typename FindType<true, KeyTypeTuple, OrderedBindingSlotArgTypeTupleT>::FoundIndexSeq;
		return OrderedBindingKeyIndexSequence{};
	}

	template<size_t ArgIndex, size_t BindingSlotIndex, size_t OrderedBindingKeyIndex, size_t... ArgIndices, size_t... BindingSlotIndices, size_t... OrderedBindingKeyIndices>
	static constexpr auto makeMatchedKeyIndexSequence(std::index_sequence<ArgIndex, ArgIndices...>, std::index_sequence<BindingSlotIndex, BindingSlotIndices...>, std::index_sequence<OrderedBindingKeyIndex, OrderedBindingKeyIndices...>)
	{
		return index_sequence_cat(
			std::index_sequence<MatchedKeyIndex<ArgIndex, BindingSlotIndex, OrderedBindingKeyIndex>::value >{},
			CompareAndNext<ArgIndex, BindingSlotIndex>::next(std::index_sequence<ArgIndices...>{}, std::index_sequence<BindingSlotIndex, BindingSlotIndices...>{}, std::index_sequence<OrderedBindingKeyIndex, OrderedBindingKeyIndices...>{}));
	}

	static auto makeMatchedKeyIndexSequence(std::index_sequence<>, std::index_sequence<>, std::index_sequence<>) { return std::index_sequence<>{}; }

	template<size_t... ArgIndices>
	static auto makeMatchedKeyIndexSequence(std::index_sequence<ArgIndices...>, std::index_sequence<>, std::index_sequence<>) { return std::index_sequence<(ArgIndices, size_t(-1))...>{}; }

	template<size_t... BindingSlotIndices, size_t... OrderedBindingKeyIndices>
	static auto makeMatchedKeyIndexSequence(std::index_sequence<>, std::index_sequence<BindingSlotIndices...>, std::index_sequence<OrderedBindingKeyIndices...>) IS_NOT_REQUIRED;

		template<size_t ArgIndex, size_t BindingSlotIndex, size_t OrderedBindingKeyIndex>
	struct MatchedKeyIndex { constexpr static size_t value = ArgIndex == BindingSlotIndex ? OrderedBindingKeyIndex : -1; };

	template<size_t ArgIndex, size_t BindingSlotIndex>
	struct CompareAndNext
	{
		template<size_t... ArgIndices, size_t... BindingSlotIndices, size_t... OrderedBindingKeyIndices>
		static constexpr auto next(std::index_sequence<ArgIndices...>, std::index_sequence<BindingSlotIndices...>, std::index_sequence<OrderedBindingKeyIndices...>)
		{
			return makeMatchedKeyIndexSequence(std::index_sequence<ArgIndices...>{}, std::index_sequence<BindingSlotIndices...>{}, std::index_sequence<OrderedBindingKeyIndices...>{});
		}
	};

	template<size_t ArgIndex>
	struct CompareAndNext<ArgIndex, ArgIndex>
	{
		template<size_t BindingSlotIndex, size_t OrderedBindingKeyIndex, size_t... ArgIndices, size_t... BindingSlotIndices, size_t... OrderedBindingKeyIndices>
		static constexpr auto next(std::index_sequence<ArgIndices...>, std::index_sequence<BindingSlotIndex, BindingSlotIndices...>, std::index_sequence<OrderedBindingKeyIndex, OrderedBindingKeyIndices...>)
		{
			return makeMatchedKeyIndexSequence(std::index_sequence<ArgIndices...>{}, std::index_sequence<BindingSlotIndices...>{}, std::index_sequence<OrderedBindingKeyIndices...>{});
		}
	};

private:
	Callable _callable;
	ArgTypeTupleResolved _args;
	RetType& _ret;
};