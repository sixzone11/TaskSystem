#include "pch.h"

#include "TaskSystem.h"

#include <utility>
#include <limits.h>

struct TaskKeyImpl : public ITaskKey
{
public:
	std::shared_ptr<TaskCommitInfo>& getCommitInfo() { return _commitInfo; }
};

std::ostream& operator<<(std::ostream& os, const ITaskKey& taskKey)
{
	auto& taskKeyImpl = static_cast<const TaskKeyImpl&>(taskKey);
	return os << "task key output";
}

class TaskManagerImpl : public ITaskManager
{
	bool commitTask(ITaskKey* taskKey) const override;
};

bool TaskManagerImpl::commitTask(ITaskKey* taskKey) const
{
	auto commitInfo = static_cast<TaskKeyImpl*>(taskKey)->getCommitInfo();
	
	int i = 0;
	for (auto& task : commitInfo->_taskKeys)
	{
		commitInfo->_taskUsed[i++] = true;
		task->process();

		task.reset();
	}

	return true;
}

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