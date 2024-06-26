﻿#include "pch.h"

#include <TaskSystem/TaskSystem.h>

std::vector<char>	test_loadDataFromFile(const char* filePath);

std::vector<char>	test_loadDataFromFileByTask(const char* filePath);
std::vector<char>	test_loadDataFromFileByTask2(const char* filePath);

void				test_member_function();
void				test_functions(const char* filePath);

void				test_examples_for_meta();

void entrypoint_test()
{
	test_loadDataFromFile("no_task");

	test_loadDataFromFileByTask("lambda_task");
	test_loadDataFromFileByTask2("lambda_task_with_key");

	test_member_function();
	test_functions("");
	test_examples_for_meta();
}


#define Dependency	TaskWriter::taskDependency
#define Concurrency	TaskWriter::taskConcurrency
#define Task		TaskWriter::task

#define SwitchTask	TaskWriter::taskConcurrency

#define KeyList(Key, ...) make_tuple(pseudo_void{}, pseudo_void{}, BindingKeys(Key, __VA_ARGS__))

bool isExist(const char* filePath) { return true; }
void* openFile(const char* filePath) { return (void*)(filePath); }
size_t getSize(void* fileDescriptor) { return 0; }
std::vector<char> allocateMemory(size_t size) { return std::vector<char>(size); }
int readFile(void* fileDescriptor, std::vector<char>& dstMemory, size_t size) { return 0; }

namespace FileSystem
{
	bool isFileIOJobAvailable() { return false; }
}

// [x] 1. 중첩 체인, 정션에서 어떻게 CallableInfo 를 구성하고 그 제약을 설정하게 할 것인지.
// [ ] 2. 실제 데이터가 오가기 위한 메모리 확보 및 공간 연결 구성

//auto openReadAndCopyFromItIfExist = Dependency(
//	Task(isExist, filePath),
//	Branch(
//		ConditionalTask(TaskResult::Succeeded, "OpenFile", filePath),
//		ConditionalFail(TaskResult::Failed)
//	),
//	Branch(
//		ConditionalTask(TaskResult::Succeeded, "AllocateMemory", getTaskResult ),
//		ConditionalFail(TaskResult::Failed)
//	),
//	Branch(
//		ConditionalTask(TaskResult::Succeeded, "CopyData", getTaskResult),
//		ConditionalFail(TaskResult::Failed)
//	)
//);

std::vector<char> test_loadDataFromFile(const char* filePath)
{
	const bool bExist = isExist(filePath);
	if (bExist == false)
		return std::vector<char>();

	void* fileDescriptor = openFile(filePath);
	if (fileDescriptor == nullptr)
		return std::vector<char>();

	size_t fileSize = getSize(fileDescriptor);

	std::vector<char> memory = allocateMemory(fileSize);

	int result = readFile(fileDescriptor, memory, fileSize);
	if (result != 0)
		return std::vector<char>();

	return memory;
}

std::vector<char> test_loadDataFromFileByTask(const char* filePath)
{
	auto e0 = Task(ProcessBlock(filePath)
	{
		return isExist(filePath);
	});
	auto e1 = 5;

	TaskProcessBeginCapture(t1, Capture(filePath, e1))
	{
		return isExist(filePath) ? filePath : nullptr;
	}
	TaskProcessNext(t2,
		Condition_Cancel(GetReturn(t1) == nullptr),
		WaitWhile(FileSystem::isFileIOJobAvailable() == false),)
	{
		return openFile(GetReturn(t1));
	}
	TaskProcessNext(t3)//, Condition_Cancel(GetReturn(t2) == nullptr))
	{
		//bool isFileExist = GetReturn(e0);
		void* fileDescriptor = GetReturn(t2);

		if (fileDescriptor == nullptr)
			return std::vector<char>();

		size_t fileSize = getSize(fileDescriptor);

		std::vector<char> memory = allocateMemory(fileSize);

		int result = readFile(fileDescriptor, memory, fileSize);
		if (result != 0)
			return std::vector<char>();

		return memory;
	}
	TaskProcessEnd();

	auto openFileTasks = Dependency( std::move(t1), std::move(t2), std::move(t3));

	ITask* taskKey = create_task(std::move(openFileTasks));

	ITaskManager* taskManager = getDefaultTaskManager();
	taskManager->commitTask(taskKey);

	return std::vector<char>();
}

namespace KeyA {
	DefineBindingKey(First);
	DefineBindingKey(Second);
	DefineBindingKey(Third);
	DefineBindingKey(Forth);
	DefineBindingKey(Fifth);
}

namespace KeyB {
	DefineBindingKey(First);
	DefineBindingKey(Second);
	DefineBindingKey(Third);
}

std::vector<char> test_loadDataFromFileByTask2(const char* filePath)
{
	auto integral_expression = []() { return 5; };

	auto chainConnectedTaskWithArg = Dependency(
		Task<KeyB::First>( ProcessBlock(filePath)
	{

		return isExist(filePath) ? filePath : nullptr;

	}),
		Task<KeyB::Second>(
			Condition_Cancel(GetReturn(KeyB::First) == nullptr),
			WaitWhile(FileSystem::isFileIOJobAvailable() == false),
			ProcessBlock()
	{

		return openFile(GetReturn(KeyB::First));

	}),
		Task<KeyB::Third>(
			Condition_Cancel(GetReturn(KeyB::Second) == nullptr),
			ProcessBlock()
	{

		void* fileDescriptor = GetReturn(KeyB::Second);
		
		size_t fileSize = getSize(fileDescriptor);

		std::vector<char> memory = allocateMemory(fileSize);

		int result = readFile(fileDescriptor, memory, fileSize);
		if (result != 0)
			memory = std::vector<char>();

		return memory;
	}));

	ITask* taskKey = create_task(std::move(chainConnectedTaskWithArg));

	ITaskManager* taskManager = getDefaultTaskManager();
	taskManager->commitTask(taskKey);

	return std::vector<char>();
}

void test_member_function()
{
	ITaskManager* taskManager = getDefaultTaskManager();

	struct MemberFunctionTest
	{
		bool func(const char*) { return false; }
	};
	MemberFunctionTest memberFunctionTest;
	auto testTask = Task(&MemberFunctionTest::func, &memberFunctionTest, "test code");

	ITask* taskKey = create_task(move(testTask));
	taskManager->commitTask(taskKey);
}

void test_functions(const char* filePath)
{
	ITaskManager* taskManager = getDefaultTaskManager();

	auto openFileTaskChainA = Dependency(
		Task(isExist, filePath),
		Task<KeyA::First>(openFile, filePath),
		Task<KeyA::Second>(WaitWhile(FileSystem::isFileIOJobAvailable()), getSize, KeyA::First()),
		Task<KeyA::Third>(allocateMemory, KeyA::Second()),
		Task<KeyA::Forth>(readFile, KeyA::First(), KeyA::Third(), KeyA::Second()),
		Task<KeyA::Fifth>( ProcessBlock() {
			int& readResult = GetReturn(KeyA::Forth);
			std::vector<char>& memory = GetReturn(KeyA::Third);

			if (readResult != 0)
				return std::vector<char>();
			else
				return memory;
		})
	);

	ITask* taskKey = create_task(std::move(openFileTaskChainA));
	taskManager->commitTask(taskKey);
}

void test_examples_for_meta()
{
	auto debugTask = Task("Task");

	auto debugChain =
		Dependency(
			Task("0")
			, Task("1")
			, Task("2")
			, Task("3")
			, Task("4")
			, Task("5")
			, Task("6")
		);

	auto debugJunction =
		Concurrency(
			Task("0")
			, Task("1")
		);

	auto debugChainAndJunction =
		Dependency(
			Task("0")
			, Concurrency(
				Task("1")
				, Task("2")
			)
		);

	auto testChain3 =
		Dependency(
			Task("0")
			, Task("1")
			, Task("2")
			, Task("3")
			, Task("4")
		);

	auto testChainOfJunctions =
		Dependency(
			Task("0"),
			Concurrency(
				Task("1")
				, Task("2")
				, Task("3")
			)
			, Concurrency(
				Task("4")
				, Task("5")
			)
		);

	auto testTasks =
		Concurrency(
			std::move(testChainOfJunctions),
			Dependency(
				Task("6")
				, Concurrency(
					Task("7")
					, Task("8")
				)
			)
			, Dependency(
				Task("9")
				, Task("10")
				, Task("11")
			)
		);

	auto testTasks2 =
		Dependency(
			Task("00")
			,
			Concurrency(
				std::move(testChainOfJunctions),
				Dependency(
					Task("7")
					, Concurrency(
						Task("8")
						, Task("9")
					)
				)
				, Dependency(
					Task("10")
					, Task("11")
					, Task("12")
				)
			)
			, Concurrency(
				Task("13")
				, Task("14")
				, Task("15")
			)
		);

	auto result = Dependency(
		Task("AsyncTask""1"),
		Concurrency(
			Dependency(
				Task("JunctionTask""1_1"),
				Task("JunctionTask""1_2")),
			Task("JunctionTask""2"),
			Concurrency(
				Dependency(
					Task("JunctionTask""1_1"),
					Task("JunctionTask""1_2"),
					Concurrency(
						Dependency(
							Task("JunctionTask""1_1"),
							Task("JunctionTask""1_2")),
						Task("JunctionTask""2")
					)),
				Task("JunctionTask""2")
			)
		),
		Dependency(
			Dependency(
				Dependency(
					Dependency(
						Task("AsyncTask""3"))))),
		Task("AsyncTask""4"),
		Task("AsyncTask""5")
	);


	auto testDependenciesOfTaskss =
		Dependency(
			Task("0"),
			Concurrency(
				Task("1")
				, Task("2")
				, Task("3")
			)
			, Concurrency(
				Task("4")
				, Task("5")
			)
		);

	auto testTask2s =
		Concurrency(
			std::move(testDependenciesOfTaskss),
			Dependency(
				Task("6")
				, Concurrency(
					Task("7")
					, Task("8")
				)
			)
			, Dependency(
				Task("9")
				, Task("10")
				, Task("11")
			)
		);

	auto& r = result;
	using T = std::remove_reference_t<decltype(r)>;

	printf("TaskDefineSize: %zu\n", sizeof(T));

	printTaskDefine(r);
}