#include "pch.h"
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

//auto waitForGpuTask = [](uint32_t, uint32_t) {
//
//	return TaskResult::Succeeded;
//};


static void test()
{
}

void test_ver2();

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
	convertToStringList<Format::utf8>(3);

	//std::ostringstream stest(,);
	std::cout << "Hello World!" << std::endl;
	//std::cout << *taskKey << std::endl;

	test();
	test_ver2();
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
