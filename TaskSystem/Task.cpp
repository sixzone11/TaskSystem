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
