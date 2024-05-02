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

template<uint32_t NumManifests_, uint32_t NumLinks_, uint32_t NumInputs_, uint32_t NumOutputs_>
struct TaskManifestWritten
{
	constexpr static const uint32_t NumManifests = NumManifests_;
	constexpr static const uint32_t NumLinks = NumLinks_;
	constexpr static const uint32_t NumInputs = NumInputs_;
	constexpr static const uint32_t NumOutputs = NumOutputs_;

	TaskDefined _taskDefined[NumManifests];
	uint32_t _offsets[NumManifests];
	uint32_t _links[NumLinks];
	uint32_t _inputs[NumInputs] = {};
	uint32_t _outputs[NumOutputs] = {};
	uint32_t _precedingCount[NumManifests + 1] = {};
};


template<typename... TaskManifestList>
struct CalcNumTaskManifests;

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
struct CalcNumTaskManifests<TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>>
{
	constexpr static const uint32_t value = NumManifests;

	constexpr static const uint32_t numInputs = NumInputs;
	constexpr static const uint32_t numOutputs = NumOutputs;

	constexpr static const uint32_t sumInputsInJunction = NumInputs;
	constexpr static const uint32_t sumOutputsInJunction = NumOutputs;
};

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
struct CalcNumTaskManifests<TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>, TaskManifestList...>
{
	constexpr static const uint32_t value = NumManifests + CalcNumTaskManifests<TaskManifestList...>::value;

	constexpr static const uint32_t numInputs = NumInputs;
	constexpr static const uint32_t numOutputs = CalcNumTaskManifests<TaskManifestList...>::numOutputs;

	constexpr static const uint32_t sumInputsInJunction = NumInputs + CalcNumTaskManifests<TaskManifestList...>::sumInputsInJunction;
	constexpr static const uint32_t sumOutputsInJunction = NumOutputs + CalcNumTaskManifests<TaskManifestList...>::sumOutputsInJunction;
};


template<typename... TaskManifestList>
struct CalcNumNextTasks;

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
struct CalcNumNextTasks<TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>> {
	constexpr static const uint32_t valueInChain = NumLinks; // NumManifests=1, NumIn/Out=1 -> NumLinks=1
	constexpr static const uint32_t valueInJunction = NumLinks;
};

template<
	uint32_t NumManifestsCur, uint32_t NumLinksCur, uint32_t NumInputsCur, uint32_t NumOutputsCur,
	uint32_t NumManifestsNext, uint32_t NumLinksNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, typename... TaskManifestList>
struct CalcNumNextTasks<
	TaskManifestWritten<NumManifestsCur, NumLinksCur, NumInputsCur, NumOutputsCur>,
	TaskManifestWritten<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext>, TaskManifestList...>
{
	constexpr static const uint32_t valueInChain = NumLinksCur + (NumOutputsCur * (NumInputsNext - 1)) + CalcNumNextTasks<TaskManifestWritten<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext>, TaskManifestList...>::valueInChain;
	constexpr static const uint32_t valueInJunction = NumLinksCur + CalcNumNextTasks<TaskManifestWritten<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext>, TaskManifestList...>::valueInJunction;
};



struct TaskManifestWriter
{
	template<typename... TaskManifestList>
	constexpr static auto defineTaskChain(TaskManifestList&&... list);

	template<typename... TaskManifestList>
	constexpr static auto defineJunction(TaskManifestList&&... list);

	constexpr static TaskManifestWritten<1, 1, 1, 1> defineTask(const char* taskName, TaskExecution&& execution);
	constexpr static TaskManifestWritten<1, 1, 1, 1> defineTask(const char* taskName, TaskCommitFlag flag, TaskExecution&& execution);

private:
	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
	constexpr static auto __defineTaskChain_inputs(uint32_t* inputs, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined, TaskManifestList&&... list);

	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
	constexpr static auto __defineTaskChain_taskDefined(TaskDefined* yetWriting, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined, TaskManifestList&&... list);
	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
	constexpr static auto __defineTaskChain_taskDefined(TaskDefined* yetWriting, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined);

	template<
		uint32_t NumManifestsCur, uint32_t NumLinksCur, uint32_t NumInputsCur, uint32_t NumOutputsCur,
		uint32_t NumManifestsNext, uint32_t NumLinksNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, typename... TaskManifestList>
	constexpr static auto __defineTaskChain_links(
		uint32_t* const offsets, const uint32_t baseOffset, uint32_t* const nexts, const uint32_t startId,
		TaskManifestWritten<NumManifestsCur, NumLinksCur, NumInputsCur, NumOutputsCur>&& curTaskDefined, TaskManifestWritten<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext>&& nextTaskDefined, TaskManifestList&&... list);

	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
	constexpr static auto __defineTaskChain_links(
		uint32_t* const offsets, const uint32_t baseOffset, uint32_t* const nexts, const uint32_t startId,
		TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined);

	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
	constexpr static auto __defineTaskChain_outputs(uint32_t* outputs, const uint32_t accumManifests, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined, TaskManifestList&&... list);
	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
	constexpr static auto __defineTaskChain_outputs(uint32_t* outputs, const uint32_t accumManifests, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined);

private:
	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
	constexpr static auto __defineJunction_inputs(uint32_t* inputs, const uint32_t offset, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined, TaskManifestList&&... list);
	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
	constexpr static auto __defineJunction_inputs(uint32_t* inputs, const uint32_t offset, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined);

	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
	constexpr static auto __defineJunction_taskDefined(TaskDefined* yetWriting, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined, TaskManifestList&&... list);
	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
	constexpr static auto __defineJunction_taskDefined(TaskDefined* yetWriting, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined);


	template<
		uint32_t NumManifestsCur, uint32_t NumLinksCur, uint32_t NumInputsCur, uint32_t NumOutputsCur,
		uint32_t NumManifestsNext, uint32_t NumLinksNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, typename... TaskManifestList>
	constexpr static auto __defineJunction_links(
		uint32_t* offsets, const uint32_t baseOffset, uint32_t* nexts, const uint32_t startId,
		TaskManifestWritten<NumManifestsCur, NumLinksCur, NumInputsCur, NumOutputsCur>&& curTaskDefined, TaskManifestWritten<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext>&& nextTaskDefined, TaskManifestList&&... list);

	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
	constexpr static auto __defineJunction_links(
		uint32_t* offsets, const uint32_t baseOffset, uint32_t* nexts, const uint32_t startId,
		TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined);


	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
	constexpr static auto __defineJunction_outputs(
		uint32_t* outputs, const uint32_t accumManifests, uint32_t* offsets, uint32_t* nexts, const uint32_t numManifests,
		TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined, TaskManifestList... list);

	template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
	constexpr static auto __defineJunction_outputs(
		uint32_t* outputs, const uint32_t accumManifests, uint32_t* offsets, uint32_t* nexts, const uint32_t numManifests,
		TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined);
};

template<typename... TaskManifestList>
constexpr auto TaskManifestWriter::defineTaskChain(TaskManifestList&&... list)
{
	constexpr uint32_t NumManifests = CalcNumTaskManifests<TaskManifestList...>::value;
	constexpr uint32_t NumLinks = CalcNumNextTasks<TaskManifestList...>::valueInChain;
	constexpr uint32_t NumInputs = CalcNumTaskManifests<TaskManifestList...>::numInputs;
	constexpr uint32_t NumOutputs = CalcNumTaskManifests<TaskManifestList...>::numOutputs;
	TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs> yetWriting;

	__defineTaskChain_inputs(yetWriting._inputs, std::forward<TaskManifestList>(list)...);
	__defineTaskChain_taskDefined(yetWriting._taskDefined, std::forward<TaskManifestList>(list)...);
	__defineTaskChain_links(yetWriting._offsets, 0, yetWriting._links, 0, std::forward<TaskManifestList>(list)...);
	__defineTaskChain_outputs(yetWriting._outputs, 0, std::forward<TaskManifestList>(list)...);

	for (uint32_t i = 0; i < NumLinks; ++i)
		yetWriting._precedingCount[yetWriting._links[i]]++;

	return yetWriting;
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
constexpr auto TaskManifestWriter::__defineTaskChain_inputs(uint32_t* inputs, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined, TaskManifestList&&... list)
{
	for (uint32_t i = 0; i < NumInputs; ++i)
		inputs[i] = taskDefined._inputs[i];
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
constexpr auto TaskManifestWriter::__defineTaskChain_taskDefined(TaskDefined* yetWriting, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined, TaskManifestList&&... list)
{
	for (uint32_t i = 0; i < NumManifests; ++i)
		yetWriting[i] = std::forward<TaskDefined>(taskDefined._taskDefined[i]);

	__defineTaskChain_taskDefined(yetWriting + NumManifests, std::forward<TaskManifestList>(list)...);
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
constexpr auto TaskManifestWriter::__defineTaskChain_taskDefined(TaskDefined* yetWriting, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined)
{
	for (uint32_t i = 0; i < NumManifests; ++i)
		yetWriting[i] = std::forward<TaskDefined>(taskDefined._taskDefined[i]);
}

template<
	uint32_t NumManifestsCur, uint32_t NumLinksCur, uint32_t NumInputsCur, uint32_t NumOutputsCur,
	uint32_t NumManifestsNext, uint32_t NumLinksNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, typename... TaskManifestList>
constexpr auto TaskManifestWriter::__defineTaskChain_links(
	uint32_t* const offsets, const uint32_t baseOffset, uint32_t* const nexts, const uint32_t startId,
	TaskManifestWritten<NumManifestsCur, NumLinksCur, NumInputsCur, NumOutputsCur>&& curTaskDefined, TaskManifestWritten<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext>&& nextTaskDefined, TaskManifestList&&... list)
{
	uint32_t offsetBias[NumManifestsCur];
	for (uint32_t i = 0, j = 0; i < NumManifestsCur; ++i)
		offsetBias[i] = (i < curTaskDefined._outputs[j]) ? j : j++;

	for (uint32_t i = 0; i < NumManifestsCur; ++i)
		offsets[i] = baseOffset + curTaskDefined._offsets[i] + offsetBias[i] * (NumInputsNext - 1);

	uint32_t nextBias[NumLinksCur];
	for (uint32_t i = 0, j = 0; i < NumLinksCur; ++i)
		nextBias[i] = (i != curTaskDefined._offsets[curTaskDefined._outputs[j]]) ? 1 : (j++, NumInputsNext);

	for (uint32_t i = 0, j = 0; i < NumLinksCur; ++i)
	{
		nexts[j] = startId + curTaskDefined._links[i];
		j += nextBias[i];
	}

	for (uint32_t i = 0; i < NumOutputsCur; ++i)
	{
		for (uint32_t j = 0; j < NumInputsNext; ++j)
			nexts[offsets[curTaskDefined._outputs[i]] - baseOffset + j] = startId + NumManifestsCur + nextTaskDefined._inputs[j];
	}

	// for (uint32_t i = 0, bias = 0; i < NumLinksCur; ++i)
	// {
	//     if (curTaskDefined._links[i] != NumOutputsCur)
	//         nexts[i + bias] = startId + curTaskDefined._links[i];
	//     else
	//     {
	//         for(uint32_t j = 0; j < NumInputsNext; ++j)
	//             nexts[i + bias + j] = startId + NumManifestsCur + nextTaskDefined._inputs[j];

	//         bias += NumInputsNext - 1;
	//     }
	// }

	__defineTaskChain_links(offsets + NumManifestsCur, baseOffset + NumLinksCur + NumOutputsCur * (NumInputsNext - 1),
		nexts + NumLinksCur + NumOutputsCur * (NumInputsNext - 1), startId + NumManifestsCur, std::forward<TaskManifestWritten<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext>>(nextTaskDefined), std::forward<TaskManifestList>(list)...);
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
constexpr auto TaskManifestWriter::__defineTaskChain_links(
	uint32_t* const offsets, const uint32_t baseOffset, uint32_t* const nexts, const uint32_t startId,
	TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined)
{
	for (uint32_t i = 0; i < NumManifests; ++i)
		offsets[i] = baseOffset + taskDefined._offsets[i];

	for (uint32_t i = 0; i < NumLinks; ++i)
		nexts[i] = startId + taskDefined._links[i];
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
constexpr auto TaskManifestWriter::__defineTaskChain_outputs(uint32_t* outputs, const uint32_t accumManifests, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined, TaskManifestList&&... list)
{
	__defineTaskChain_outputs(outputs, accumManifests + NumManifests, std::forward<TaskManifestList>(list)...);
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
constexpr auto TaskManifestWriter::__defineTaskChain_outputs(uint32_t* outputs, const uint32_t accumManifests, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined)
{
	for (uint32_t i = 0; i < NumOutputs; ++i)
		outputs[i] = accumManifests + i;
}


template<typename... TaskManifestList>
constexpr auto TaskManifestWriter::defineJunction(TaskManifestList&&... list)
{
	constexpr uint32_t NumManifests = CalcNumTaskManifests<TaskManifestList...>::value;
	constexpr uint32_t NumLinks = CalcNumNextTasks<TaskManifestList...>::valueInJunction;
	constexpr uint32_t NumInputs = CalcNumTaskManifests<TaskManifestList...>::sumInputsInJunction;
	constexpr uint32_t NumOutputs = CalcNumTaskManifests<TaskManifestList...>::sumOutputsInJunction;
	TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs> yetWriting;

	__defineJunction_inputs(yetWriting._inputs, 0, std::forward<TaskManifestList>(list)...);
	__defineJunction_taskDefined(yetWriting._taskDefined, std::forward<TaskManifestList>(list)...);
	__defineJunction_links(yetWriting._offsets, 0, yetWriting._links, 0, std::forward<TaskManifestList>(list)...);
	__defineJunction_outputs(yetWriting._outputs, 0, yetWriting._offsets, yetWriting._links, NumManifests, std::forward<TaskManifestList>(list)...);

	for (uint32_t i = 0; i < NumLinks; ++i)
		yetWriting._precedingCount[yetWriting._links[i]]++;

	return yetWriting;
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
constexpr auto TaskManifestWriter::__defineJunction_inputs(uint32_t* inputs, const uint32_t offset, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined)
{
	for (uint32_t i = 0; i < NumInputs; ++i)
		inputs[i] = offset + taskDefined._inputs[i];
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
constexpr auto TaskManifestWriter::__defineJunction_inputs(uint32_t* inputs, const uint32_t offset, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined, TaskManifestList&&... list)
{
	__defineJunction_inputs(inputs, offset, std::forward<TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>>(taskDefined));
	__defineJunction_inputs(inputs + NumInputs, offset + NumManifests, std::forward<TaskManifestList>(list)...);
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
constexpr auto TaskManifestWriter::__defineJunction_taskDefined(TaskDefined* yetWriting, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined)
{
	for (uint32_t i = 0; i < NumManifests; ++i)
		yetWriting[i] = std::forward<TaskDefined>(taskDefined._taskDefined[i]);
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
constexpr auto TaskManifestWriter::__defineJunction_taskDefined(TaskDefined* yetWriting, TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined, TaskManifestList&&... list)
{
	__defineJunction_taskDefined(yetWriting, std::forward<TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>>(taskDefined));
	__defineJunction_taskDefined(yetWriting + NumManifests, std::forward<TaskManifestList>(list)...);
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
constexpr auto TaskManifestWriter::__defineJunction_links(
	uint32_t* offsets, const uint32_t baseOffset, uint32_t* nexts, const uint32_t startId,
	TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined)
{
	for (uint32_t i = 0; i < NumManifests; ++i)
		offsets[i] = baseOffset + taskDefined._offsets[i];

	for (uint32_t i = 0; i < NumLinks; ++i)
		nexts[i] = startId + taskDefined._links[i];
}

template<
	uint32_t NumManifestsCur, uint32_t NumLinksCur, uint32_t NumInputsCur, uint32_t NumOutputsCur,
	uint32_t NumManifestsNext, uint32_t NumLinksNext, uint32_t NumInputsNext, uint32_t NumOutputsNext, typename... TaskManifestList>
constexpr auto TaskManifestWriter::__defineJunction_links(
	uint32_t* offsets, const uint32_t baseOffset, uint32_t* nexts, const uint32_t startId,
	TaskManifestWritten<NumManifestsCur, NumLinksCur, NumInputsCur, NumOutputsCur>&& curTaskDefined, TaskManifestWritten<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext>&& nextTaskDefined, TaskManifestList&&... list)
{
	__defineJunction_links(offsets, baseOffset, nexts, startId, std::forward<TaskManifestWritten<NumManifestsCur, NumLinksCur, NumInputsCur, NumOutputsCur>>(curTaskDefined));
	__defineJunction_links(offsets + NumManifestsCur, baseOffset + NumLinksCur, nexts + NumLinksCur, startId + NumManifestsCur, std::forward<TaskManifestWritten<NumManifestsNext, NumLinksNext, NumInputsNext, NumOutputsNext>>(nextTaskDefined), std::forward<TaskManifestList>(list)...);
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs>
constexpr auto TaskManifestWriter::__defineJunction_outputs(
	uint32_t* outputs, const uint32_t accumManifests, uint32_t* offsets, uint32_t* nexts, const uint32_t numManifests,
	TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined)
{
	for (uint32_t i = 0; i < NumOutputs; ++i)
		outputs[i] = accumManifests + taskDefined._outputs[i];

	for (uint32_t i = 0; i < NumOutputs; ++i)
		nexts[offsets[outputs[i]]] = numManifests;
}

template<uint32_t NumManifests, uint32_t NumLinks, uint32_t NumInputs, uint32_t NumOutputs, typename... TaskManifestList>
constexpr auto TaskManifestWriter::__defineJunction_outputs(
	uint32_t* outputs, const uint32_t accumManifests, uint32_t* offsets, uint32_t* nexts, const uint32_t numManifests,
	TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>&& taskDefined, TaskManifestList... list)
{
	__defineJunction_outputs(outputs, accumManifests, offsets, nexts, numManifests, std::forward<TaskManifestWritten<NumManifests, NumLinks, NumInputs, NumOutputs>>(taskDefined));
	__defineJunction_outputs(outputs + NumOutputs, accumManifests + NumManifests, offsets, nexts, numManifests, std::forward<TaskManifestList>(list)...);
}

constexpr TaskManifestWritten<1, 1, 1, 1> TaskManifestWriter::defineTask(const char* taskName, TaskExecution&& execution)
{
	return TaskManifestWritten<1, 1, 1, 1>{ TaskDefined{ taskName, /*move(execution)*/ }, {}, { 1 }, {}, {} };
}

constexpr TaskManifestWritten<1, 1, 1, 1> TaskManifestWriter::defineTask(const char* taskName, TaskCommitFlag flag, TaskExecution&& execution)
{
	return TaskManifestWritten<1, 1, 1, 1>{ TaskDefined{ taskName, /*move(execution)*/ }, {}, { 1 }, {}, {} };
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