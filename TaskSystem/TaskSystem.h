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
	virtual ~ITaskKey() {}

	friend TASKSYSTEM_API std::ostream& operator<<(std::ostream& os, const ITaskKey& taskKey);
};
TASKSYSTEM_API std::ostream& operator<<(std::ostream& os, const ITaskKey& taskKey);

struct ITaskManager
{
public:
	template<typename TaskInfoList>
	ITaskKey* createTask(TaskInfoList&& taskInfoList);

	virtual ~ITaskManager() {}

private:
	virtual ITaskKey* createTask(std::shared_ptr<TaskCommitInfo>&& taskCommitInfo) = 0;

	//virtual TaskExecutePoint createTaskExecutePoint(TaskExecutePointDesc&& taskExecutePointDesc) = 0;
};

extern "C" TASKSYSTEM_API ITaskManager * getDefaultTaskManager();



template<typename TaskInfoList>
ITaskKey* ITaskManager::createTask(TaskInfoList&& taskInfoList)
{
	using TaskTuple = std::remove_reference_t<TaskInfoList>;
	using TaskMeta = typename std::tuple_element<IndexTaskMeta, TaskTuple>::type;

	return createTask(std::make_shared<TaskCommitInfo>(
		moveDefinesToVec(std::forward<TaskInfoList>(taskInfoList)),
		copyToVec<typename TaskGetter<TaskMeta>::Offsets, 1>(),
		copyToVec<typename TaskGetter<TaskMeta>::Links>(),
		copyToVec<typename TaskGetter<TaskMeta>::Inputs>(),
		copyToVec<typename TaskGetter<TaskMeta>::Outputs>()
		));
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