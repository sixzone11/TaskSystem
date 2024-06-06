#pragma once

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

enum class TaskResult : uint32_t
{
	Succeeded,
	Failed,
};

enum class TaskCommitFlag
{
	ExecuteLater = 0x00,
	ExecuteImmediately = 0x01,
	CreateAsSubTask = 0x02,

	Default = ExecuteLater,
};

using TaskExecution = std::function<TaskResult(uint32_t, uint32_t)>;

struct TaskExecutePoint {};
struct TaskExecutePointDesc
{
	uint32_t _acceptableWorkerMask;
	uint32_t _s;
};

extern thread_local TaskExecutePoint g_defaultPoint;

extern TaskExecutePoint g_taskExecutePointMainThread;
extern TaskExecutePoint g_taskExecutePointAsyncWorkerThread;


struct ITaskKey
{
	ITaskKey() = default;
	ITaskKey(std::shared_ptr<TaskCommitInfo>& commitInfo)
		: _commitInfo(commitInfo) {}
	virtual ~ITaskKey() {}

	virtual void process() = 0;
	virtual void process() const = 0;

	friend TASKSYSTEM_API std::ostream& operator<<(std::ostream& os, const ITaskKey& taskKey);

protected:
	std::shared_ptr<TaskCommitInfo> _commitInfo;
};
TASKSYSTEM_API std::ostream& operator<<(std::ostream& os, const ITaskKey& taskKey);

struct ITaskManager
{
public:
	template<typename TaskInfoList>
	ITaskKey* createTask(TaskInfoList&& taskInfoList);
	
	virtual bool commitTask(ITaskKey* taskKey) const = 0;

	virtual ~ITaskManager() {}

private:
	//virtual TaskExecutePoint createTaskExecutePoint(TaskExecutePointDesc&& taskExecutePointDesc) = 0;
};

extern "C" TASKSYSTEM_API ITaskManager * getDefaultTaskManager();

template<typename Callable, typename RetType, typename SelectType>
struct CallableTaskKey;

template<typename Callable, typename RetType, typename... Args>
struct CallableTaskKey<Callable, RetType, std::tuple<Args...>> : public ITaskKey
{
	using ArgTypeTuple = std::tuple<Args...>;

	template<typename CallableSignature>
	CallableTaskKey(CallableSignature&& callableSignature, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, RetType& ret)
		: ITaskKey(taskCommitInfo)
		, _callable(std::move(callableSignature._callable))
		, _args(std::move(callableSignature._args))
		, _ret(ret)
	{}
	~CallableTaskKey() override {}

	void process() override
	{
		new (&_ret) RetType(delegate_call(std::conditional_t<is_pseudo_void_v<RetType>, std::true_type, std::false_type>{}, std::make_integer_sequence<uint32_t, std::tuple_size_v<ArgTypeTuple>>{}));
	}
	void process() const override
	{
		new (&_ret) RetType(delegate_call(std::conditional_t<is_pseudo_void_v<RetType>, std::true_type, std::false_type>{}, std::make_integer_sequence<uint32_t, std::tuple_size_v<ArgTypeTuple>>{}));
	}

	template<uint32_t... Iseq>
	auto delegate_call(std::false_type, std::integer_sequence<uint32_t, Iseq...>) { return _callable(std::get<Iseq>(_args)...); }

	template<uint32_t... Iseq>
	auto delegate_call(std::false_type, std::integer_sequence<uint32_t, Iseq...>) const { return _callable(std::get<Iseq>(_args)...); }
	
	template<uint32_t... Iseq>
	auto delegate_call(std::true_type, std::integer_sequence<uint32_t, Iseq...>) { _callable(std::get<Iseq>(_args)...); return pseudo_void{}; }

	template<uint32_t... Iseq>
	auto delegate_call(std::true_type, std::integer_sequence<uint32_t, Iseq...>) const { _callable(std::get<Iseq>(_args)...); return pseudo_void{}; }

private:
	Callable _callable;
	ArgTypeTuple _args;
	RetType& _ret;
};

template<typename Callable, typename RetType, typename CallableInfoType>
struct CallableTaskKey : public ITaskKey
{
	template<typename CallableSignature>
	CallableTaskKey(CallableSignature&& callableSignature, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, RetType& ret)
		: ITaskKey(taskCommitInfo)
		, _callable(std::move(callableSignature._callable))
		, _ret(ret)
	{}

	~CallableTaskKey() override {}

	using ReturnTypeTuple = typename CallableInfoType::ReturnTypeTuple;
	using KeyTypeTuple = typename CallableInfoType::KeyTypeTuple;

	void process() override
	{
		auto& returnTuple = *static_cast<ReturnTypeTuple*>(static_cast<void*>(_commitInfo->_returnTupleMemory.data()));
		new (&_ret) RetType(_callable(LambdaTaskIdentifier{}, KeyTypeTuple{}, std::move(returnTuple)));
	}

	void process() const override
	{
		auto& returnTuple = *static_cast<ReturnTypeTuple*>(static_cast<void*>(_commitInfo->_returnTupleMemory.data()));
		new (&_ret) RetType(_callable(LambdaTaskIdentifier{}, KeyTypeTuple{}, std::move(returnTuple)));
	}

private:
	Callable _callable;
	RetType& _ret;
};

template<typename CallableInfoType, typename CallableSignatureResolved, typename CallableSignature>
inline auto make_CallableTaskKey(CallableSignature&& collableSignature, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, typename CallableSignatureResolved::RetType& returnReference)
{
	// CallableSignature
	using Callable = typename CallableSignature::Callable;
	using ArgTypeTuple = typename CallableSignature::ArgTypeTuple;

	// CallableSignatureResolved
	using ParamTypeTuple = typename CallableSignatureResolved::ParamTypeTuple;
	using RetType = typename CallableSignatureResolved::RetType;

	static_assert(CallableSignature::is_resolved || is_same_v<LambdaTaskIdentifier, tuple_element_t<1, ParamTypeTuple>>, "This CallableSignature is not substituted!");

	using SelectType = conditional_t<CallableSignature::is_resolved, ArgTypeTuple, CallableInfoType>;
	return new CallableTaskKey<Callable, RetType, SelectType>(std::forward<CallableSignature>(collableSignature), taskCommitInfo, returnReference);
}

template<typename TaskCallableTuple, uint32_t... I, std::enable_if_t<sizeof...(I) == 0, int> E = -2>
constexpr static void __createCallableTasks(std::unique_ptr<ITaskKey>* outCallableTasks, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, TaskCallableTuple&& tuple, std::integer_sequence<uint32_t, I...>) {}

template<typename TaskCallableTuple, uint32_t... I, std::enable_if_t<sizeof...(I) != 0, int> E = -1>
constexpr static void __createCallableTasks(std::unique_ptr<ITaskKey>* outCallableTasks, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, TaskCallableTuple&& tuple, std::integer_sequence<uint32_t, I...>)
{
	using CallableInfoType = decltype(makeCallableInfoByTuple(std::declval<TaskCallableTuple>()));
	
	using CallableSignatureResolvedTuple = typename CallableInfoType::CallableSignatureResolvedTuple;

	using ReturnTypeTuple = typename CallableInfoType::ReturnTypeTuple;
	auto& returnTuple = *static_cast<ReturnTypeTuple*>(static_cast<void*>(taskCommitInfo->_returnTupleMemory.data()));

	(outCallableTasks[I].reset(make_CallableTaskKey<CallableInfoType, std::tuple_element_t<I, CallableSignatureResolvedTuple>>(std::get<I>(std::forward<TaskCallableTuple>(tuple)), taskCommitInfo, std::get<I>(returnTuple))), ...);
}

template<typename TaskInfoList>
inline void createCallableTasks(std::vector<std::unique_ptr<ITaskKey>>& outTaskKeys, std::shared_ptr<TaskCommitInfo>& taskCommitInfo, TaskInfoList&& taskInfoList)
{
	using TaskTuple = std::remove_reference_t<TaskInfoList>;
	using TaskMeta = typename std::tuple_element_t<IndexTaskMeta, TaskTuple>;

	__createCallableTasks(outTaskKeys.data(), taskCommitInfo, std::get<IndexTaskCallable>(std::forward<TaskInfoList>(taskInfoList)), std::make_integer_sequence<uint32_t, TaskMeta::NumManifests>{});
}

template<typename CallableInfoType>
struct TaskCommitInfoWithCallable : TaskCommitInfo
{
	using ReturnTypeTuple = typename CallableInfoType::ReturnTypeTuple;

	TaskCommitInfoWithCallable(std::vector<TaskDefine>&& taskDefines, std::vector<uint32_t>&& offsets, std::vector<uint32_t>&& links, std::vector<uint32_t>&& inputs, std::vector<uint32_t>&& outputs)
		: TaskCommitInfo(std::move(taskDefines), std::move(offsets), std::move(links), std::move(inputs), std::move(outputs), sizeof(ReturnTypeTuple))
	{}

	~TaskCommitInfoWithCallable() override
	{
		auto& returnTuple = *static_cast<ReturnTypeTuple*>(static_cast<void*>(_returnTupleMemory.data()));
		deleteReturns(returnTuple, std::make_index_sequence<tuple_size_v<ReturnTypeTuple>>{});
	}

	template<size_t... Iseq>
	void deleteReturns(ReturnTypeTuple& returnTuple, std::integer_sequence<size_t, Iseq...>)
	{
		(deleteReturn<Iseq>(std::get<Iseq>(returnTuple)), ...);
	}

	template<size_t I, typename ReturnType>
	void deleteReturn(ReturnType& returnRef) { if (_taskUsed[I]) returnRef.~ReturnType(); }
};

template<typename TaskInfoList>
ITaskKey* ITaskManager::createTask(TaskInfoList&& taskInfoList)
{
	using TaskTuple = std::remove_reference_t<TaskInfoList>;
	using TaskMeta = typename std::tuple_element_t<IndexTaskMeta, TaskTuple>;
	using CallableInfoType = decltype(makeCallableInfoByTuple(std::declval<typename std::tuple_element_t<IndexTaskCallable, TaskInfoList>>()));

	auto taskCommitInfo = std::shared_ptr<TaskCommitInfo>(
		new TaskCommitInfoWithCallable<CallableInfoType>(
			moveDefinesToVec(std::forward<TaskInfoList>(taskInfoList)),
			copyToVec<typename TaskGetter<TaskMeta>::Offsets, 1>(),
			copyToVec<typename TaskGetter<TaskMeta>::Links>(),
			copyToVec<typename TaskGetter<TaskMeta>::Inputs>(),
			copyToVec<typename TaskGetter<TaskMeta>::Outputs>()
		)
	);

	auto& taskKeys = taskCommitInfo->_taskKeys;
	createCallableTasks(taskKeys, taskCommitInfo, std::forward<TaskInfoList>(taskInfoList));

	return taskKeys.back().get();


#if 0
	auto& defines = taskCommitInfo->_taskDefines;
	auto& taskKeys = taskCommitInfo->_taskKeys;

	uint32_t i = 0;
	for (auto const& define : defines)
	{
		taskKeys[i].reset(new TaskKeyImpl(std::move(taskCommitInfo), i));
		++i;
	}
#endif
	
	//return createTask(std::move(taskCommitInfo));
}

#include <cwchar>

enum Format
{
	utf8,
	utf16
};

#define FStr(F, literal_string) \
[](auto arg) { \
	struct __str { \
		static constexpr inline const char* s(char) { return literal_string; } \
		static constexpr inline const wchar_t* s(wchar_t) { return L##literal_string; } \
	}; return __str::s(arg); \
} (typename FormatMeta<F>::type())


#define FUN_ALIAS_SPEC(SPECS, NEW_NAME, ...)                                    \
  template <typename... Args>                                                   \
  SPECS auto NEW_NAME(Args &&... args)                                          \
    noexcept(noexcept(__VA_ARGS__(std::forward<Args>(args)...)))                \
    -> decltype(__VA_ARGS__(std::forward<Args>(args)...)) {                     \
    return __VA_ARGS__(std::forward<Args>(args)...);                            \
  }

#define FUN_ALIAS(NEW_NAME, ...)                                                \
  FUN_ALIAS_SPEC(static inline, NEW_NAME, __VA_ARGS__)

template<Format F>
struct FormatMeta;

template<>
struct FormatMeta<Format::utf8>
{
	using type = char;

	FUN_ALIAS(sprintf, ::sprintf_s);

	static decltype(std::cout)& cout;
};

template<>
struct FormatMeta<Format::utf16>
{
	using type = wchar_t;

	FUN_ALIAS(sprintf, ::swprintf_s);

	static decltype(std::wcout)& cout;
};


template<Format F> TASKSYSTEM_API void convertToStringList(const int& var);