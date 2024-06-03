#include "pch.h"

#include "TaskSystem.h"

#include <utility>
#include <limits.h>

struct TaskKeyImpl : public ITaskKey
{
	TaskKeyImpl(std::shared_ptr<TaskCommitInfo>&& commitInfo, const uint32_t definedIndex)
		: _commitInfo(std::move(commitInfo))
		, _definedIndex(definedIndex)
	{
	}

	std::shared_ptr<TaskCommitInfo> _commitInfo;
	uint32_t _definedIndex;
};

std::ostream& operator<<(std::ostream& os, const ITaskKey& taskKey)
{
	auto& taskKeyImpl = static_cast<const TaskKeyImpl&>(taskKey);
	return os << "task key output";
}

class TaskManagerImpl : public ITaskManager
{
	ITaskKey* createTask(std::shared_ptr<TaskCommitInfo>&& taskCommitInfo) override
	{
		auto& defines = taskCommitInfo->_taskDefines;
		auto& taskKeys = taskCommitInfo->_taskKeys;

		for( uint32_t i = 0; auto const& define : defines )
		{
			taskKeys.emplace_back(new TaskKeyImpl(std::move(taskCommitInfo), i));
			++i;
		}

		return taskKeys.front().get();
	}
};

ITaskManager* getDefaultTaskManager()
{
	static TaskManagerImpl _default;
	return &_default;
}


//template<Format F, typename T>
//void convertToStringList(const T& var)
//{
//	FormatMeta<F>::cout << "Default" << std::endl;
//}

template<Format F>
void convertToStringList(const int& var)
{
	typename FormatMeta<F>::type out_buffer[256];
	FormatMeta<F>::sprintf(out_buffer, FStr(F, "this is a integer %d"), var);

	//FormatMeta<F>::sprintf(out_buffer, FormatMeta<F>::F("this is a integer: %d"), var);

	FormatMeta<F>::cout << out_buffer << std::endl;
}
template TASKSYSTEM_API void convertToStringList<Format::utf8>(const int& var);
template TASKSYSTEM_API void convertToStringList<Format::utf16>(const int& var);