// Entrypoint.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <array>

#include <TaskSystem/TaskSystem.h>

enum class TextureLoadOption
{
	CreateIfNotExist,
	CreateByAsyncAndDependentTaskIfNotExist,
	CreateByAsyncTaskIfNotExist,
	ReturnNullIfNotExist,
};

struct TextureLoadDesc
{
	const char* _texturePath = nullptr;
	TextureLoadOption _loadOption = TextureLoadOption::CreateIfNotExist;
};

//auto loadDataForAllObjects = [](uint32_t, uint32_t) {
//	ITaskManager* taskManager = getDefaultTaskManager();
//
//	auto resourceContainer = nullptr;
//	ITaskKey* textureTaskKey = nullptr;
//	auto [ texture, textureTaskey ] = resourceContainer->getTexture(
//		TextureLoadDesc{
//			"path/to/texture",
//			TextureLoadOption::CreateByAsyncAndDependentTaskIfNotExist
//		});
//
//	return TaskResult::Succeeded;
//};

auto waitForGpuTask = [](uint32_t, uint32_t) {

	return TaskResult::Succeeded;
};


static void test()
{
	[](uint32_t, uint32_t) { return TaskResult::Succeeded; };

	auto result = TaskManifestWriter::defineTaskChain(
		TaskManifestWriter::defineTask("AsyncTask""1", TaskExecution()),
		TaskManifestWriter::defineJunction(
			TaskManifestWriter::defineTaskChain(
				TaskManifestWriter::defineTask("JunctionTask""1_1", TaskExecution()),
				TaskManifestWriter::defineTask("JunctionTask""1_2", TaskExecution())),
			TaskManifestWriter::defineTask("JunctionTask""2", TaskExecution()),
			TaskManifestWriter::defineJunction(
				TaskManifestWriter::defineTaskChain(
					TaskManifestWriter::defineTask("JunctionTask""1_1", TaskExecution()),
					TaskManifestWriter::defineTask("JunctionTask""1_2", TaskExecution()),
					TaskManifestWriter::defineJunction(
						TaskManifestWriter::defineTaskChain(
							TaskManifestWriter::defineTask("JunctionTask""1_1", TaskExecution()),
							TaskManifestWriter::defineTask("JunctionTask""1_2", TaskExecution())),
						TaskManifestWriter::defineTask("JunctionTask""2", TaskExecution())
					)),
				TaskManifestWriter::defineTask("JunctionTask""2", TaskExecution())
			)
		),
		TaskManifestWriter::defineTaskChain(
			TaskManifestWriter::defineTaskChain(
				TaskManifestWriter::defineTaskChain(
					TaskManifestWriter::defineTaskChain(
						TaskManifestWriter::defineTask("AsyncTask""3", TaskExecution()))))),
		TaskManifestWriter::defineTask("AsyncTask""4", TaskExecution()),
		TaskManifestWriter::defineTask("AsyncTask""5", TaskExecution())
	);

	constexpr uint32_t numTasks = sizeof(result._taskDefined) / sizeof(result._taskDefined[0]);
	std::vector<TaskDefined> taskDescs(numTasks);
	for (uint32_t i = 0; i < numTasks; ++i)
	{
		taskDescs[i] = std::move(result._taskDefined[i]);
	}
}

int main()
{
	//ITaskManager* taskManager = getDefaultTaskManager();
	//ITaskKey* taskKey = taskManager->createTask(TaskDefined{});
	//ITaskKey* taskKey2 = taskKey->createNextTask(TaskDefined{});
	//ITaskKey* taskKey3 = taskKey2->createNextTask(TaskDefined{});
	//ITaskKey* taskKey4 = taskKey2->createNextTask(TaskDefined{});
	//ITaskKey* taskKey5 = taskKey4->createNextTask(TaskDefined{});
	//
	//ITaskKey* forkedTaskKey = taskKey5->createNextTask(
	//	taskManager->createTask(TaskDefined{ "CreateSceneObjects" })
	//	->createNextTask(TaskDefined{ "LoadDataForAllObjects", loadDataForAllObjects })
	//	->createNextTask(TaskDefined{ "WaitForGpu", waitForGpuTask })
	//	->createNextTask(TaskDefined{ "PostLoadingProcess" }),
	//	taskManager->createTask(TaskDefined{})
	//	->createNextTask(TaskDefined{})
	//);
	//
	//ITaskKey* joinedTaskKey = forkedTaskKey->createNextTask(TaskDefined{ "Finalize" });
	//
	//taskManager->createTask();


	convertToStringList<Format::utf16>(5);
	convertToStringList<Format::utf8>(5.0);

	//std::ostringstream stest(,);
	std::cout << "Hello World!" << std::endl;
	//std::cout << *taskKey << std::endl;

	test();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
