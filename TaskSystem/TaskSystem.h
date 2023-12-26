#pragma once

#ifdef TASKSYSTEM_EXPORTS
#define TASKSYSTEM_API __declspec(dllexport)
#else
#define TASKSYSTEM_API __declspec(dllimport)
#endif

#include <iostream>
#include <functional>
#include <vector>

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

TaskExecutePoint g_taskExecutePointMainThread;
TaskExecutePoint g_taskExecutePointAsyncWorkerThread;

struct TaskDefined
{
	const char* _taskName;
	//TaskExecution _execution;
	TaskExecutePoint _executePoint = {};
};

template<uint32_t M>
struct TaskManifestWritten
{
	TaskDefined _taskDefined[M];
};


template<typename... TaskManifestList>
struct CalcNumTaskManifests;

template<>				struct CalcNumTaskManifests<TaskDefined>				{ constexpr static const uint32_t value = 1; };
template<uint32_t M>	struct CalcNumTaskManifests<TaskManifestWritten<M>>		{ constexpr static const uint32_t value = M; };

template<typename... TaskManifestList>
struct CalcNumTaskManifests<TaskDefined, TaskManifestList...>					{ constexpr static const uint32_t value = 1 + CalcNumTaskManifests<TaskManifestList...>::value; };
template<uint32_t M, typename... TaskManifestList>
struct CalcNumTaskManifests<TaskManifestWritten<M>, TaskManifestList...>		{ constexpr static const uint32_t value = M + CalcNumTaskManifests<TaskManifestList...>::value; };

struct TaskManifestWriter
{
	template<typename... TaskManifestList>
	static auto defineTaskChain(TaskManifestList&&... list);

	template<typename... TaskManifestList>
	static auto defineJunction(TaskManifestList&&... list);

	static TaskDefined defineTask(const char* taskName, TaskExecution&& execution);
	static TaskDefined defineTask(const char* taskName, TaskCommitFlag flag, TaskExecution&& execution);

private:
	template<typename... TaskManifestList>
	static auto __defineTaskChain(TaskDefined* yetWriting, TaskDefined&& taskDefined, TaskManifestList&&... list);
	static auto __defineTaskChain(TaskDefined* yetWriting, TaskDefined&& taskDefined);

	template<uint32_t M, typename... TaskManifestList>
	static auto __defineTaskChain(TaskDefined* yetWriting, TaskManifestWritten<M>&& taskDefined, TaskManifestList&&... list);
	template<uint32_t M>
	static auto __defineTaskChain(TaskDefined* yetWriting, TaskManifestWritten<M>&& taskDefined);


	template<typename... TaskManifestList>
	static auto __defineJunction(TaskDefined* yetWriting, TaskDefined&& taskDefined, TaskManifestList&&... list);
	static auto __defineJunction(TaskDefined* yetWriting, TaskDefined&& taskDefined);

	template<uint32_t M, typename... TaskManifestList>
	static auto __defineJunction(TaskDefined* yetWriting, TaskManifestWritten<M>&& taskDefined, TaskManifestList&&... list);
	template<uint32_t M>
	static auto __defineJunction(TaskDefined* yetWriting, TaskManifestWritten<M>&& taskDefined);
};

template<typename... TaskManifestList>
auto TaskManifestWriter::defineTaskChain(TaskManifestList&&... list)
{
	static constexpr uint32_t N = CalcNumTaskManifests<TaskManifestList...>::value;
	TaskManifestWritten<N> yetWriting;

	__defineTaskChain(yetWriting._taskDefined, std::forward<TaskManifestList>(list)...);

	return yetWriting;
}

template<typename... TaskManifestList>
auto TaskManifestWriter::__defineTaskChain(TaskDefined* yetWriting, TaskDefined&& taskDefined, TaskManifestList&&... list)
{
	yetWriting[0] = std::forward<TaskDefined>(taskDefined);
	__defineTaskChain(yetWriting + 1, std::forward<TaskManifestList>(list)...);
}

auto TaskManifestWriter::__defineTaskChain(TaskDefined* yetWriting, TaskDefined&& taskDefined)
{
	yetWriting[0] = std::forward<TaskDefined>(taskDefined);
}

template<uint32_t M, typename... TaskManifestList>
auto TaskManifestWriter::__defineTaskChain(TaskDefined* yetWriting, TaskManifestWritten<M>&& taskDefined, TaskManifestList&&... list)
{
	for (uint32_t i = 0; i < M; ++i)
		yetWriting[i] = std::forward<TaskDefined>(taskDefined._taskDefined[i]);

	__defineTaskChain(yetWriting + M, std::forward<TaskManifestList>(list)...);
}

template<uint32_t M>
auto TaskManifestWriter::__defineTaskChain(TaskDefined* yetWriting, TaskManifestWritten<M>&& taskDefined)
{
	for (uint32_t i = 0; i < M; ++i)
		yetWriting[i] = std::forward<TaskDefined>(taskDefined._taskDefined[i]);
}


template<typename... TaskManifestList>
auto TaskManifestWriter::defineJunction(TaskManifestList&&... list)
{
	static constexpr uint32_t M = CalcNumTaskManifests<TaskManifestList...>::value;
	TaskManifestWritten<M> yetWriting;

	__defineJunction(yetWriting._taskDefined, std::forward<TaskManifestList>(list)...);

	return yetWriting;
}

template<typename... TaskManifestList>
auto TaskManifestWriter::__defineJunction(TaskDefined* yetWriting, TaskDefined&& taskDefined, TaskManifestList&&... list)
{
	yetWriting[0] = std::forward<TaskDefined>(taskDefined);
	__defineJunction(yetWriting + 1, std::forward<TaskManifestList>(list)...);
}

auto TaskManifestWriter::__defineJunction(TaskDefined* yetWriting, TaskDefined&& taskDefined)
{
	yetWriting[0] = std::forward<TaskDefined>(taskDefined);
}

template<uint32_t M, typename... TaskManifestList>
auto TaskManifestWriter::__defineJunction(TaskDefined* yetWriting, TaskManifestWritten<M>&& taskDefined, TaskManifestList&&... list)
{
	for (uint32_t i = 0; i < M; ++i)
		yetWriting[i] = std::forward<TaskDefined>(taskDefined._taskDefined[i]);

	__defineJunction(yetWriting + M, std::forward<TaskManifestList>(list)...);
}

template<uint32_t M>
auto TaskManifestWriter::__defineJunction(TaskDefined* yetWriting, TaskManifestWritten<M>&& taskDefined)
{
	for (uint32_t i = 0; i < M; ++i)
		yetWriting[i] = std::forward<TaskDefined>(taskDefined._taskDefined[i]);
}

TaskDefined TaskManifestWriter::defineTask(const char* taskName, TaskExecution&& execution)
{
	return TaskDefined{ taskName, /*move(execution)*/ };
}

TaskDefined TaskManifestWriter::defineTask(const char* taskName, TaskCommitFlag flag, TaskExecution&& execution)
{
	return TaskDefined{ taskName, /*move(execution)*/ };
}


struct ITaskKey
{
	virtual ITaskKey* createNextTask(TaskDefined&& taskDefined) = 0;

	friend TASKSYSTEM_API std::ostream& operator<<(std::ostream& os, const ITaskKey& taskKey);
};
TASKSYSTEM_API std::ostream& operator<<(std::ostream& os, const ITaskKey& taskKey);

struct ITaskManager
{
	virtual ITaskKey* createTask(TaskDefined&& taskDefined) = 0;

	//virtual TaskExecutePoint createTaskExecutePoint(TaskExecutePointDesc&& taskExecutePointDesc) = 0;
};

extern "C" TASKSYSTEM_API ITaskManager * getDefaultTaskManager();

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

decltype(std::cout)& FormatMeta<Format::utf8>::cout = std::cout;

template<>
struct FormatMeta<Format::utf16>
{
	using type = wchar_t;

	FUN_ALIAS(sprintf, ::swprintf_s);

	static decltype(std::wcout)& cout;
};

decltype(std::wcout)& FormatMeta<Format::utf16>::cout = std::wcout;

template<Format F, typename T>
void convertToStringList(const T& var)
{
	FormatMeta<F>::cout << "Default" << std::endl;
}

template<Format F> TASKSYSTEM_API void convertToStringList(const int& var);