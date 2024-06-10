#include "pch.h"

#include "TaskSystem.h"

#include <utility>
#include <limits.h>

struct TaskImpl : public ITask
{
public:
	std::shared_ptr<TaskCommitInfo>& getCommitInfo() { return _commitInfo; }
};

std::ostream& operator<<(std::ostream& os, const ITask& taskKey)
{
	auto& taskKeyImpl = static_cast<const TaskImpl&>(taskKey);
	return os << "task key output";
}

class TaskManagerImpl : public ITaskManager
{
	bool commitTask(ITask* taskKey) const override;
};

bool TaskManagerImpl::commitTask(ITask* taskKey) const
{
	auto commitInfo = static_cast<TaskImpl*>(taskKey)->getCommitInfo();
	
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
