#include "pch.h"

#include <TaskSystem/TaskSystem.h>


#define Dependency	TaskWriter::chain
#define Concurrency	TaskWriter::junction
#define Task		TaskWriter::task

#define SwitchTask	TaskWriter::junction

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

std::vector<char> loadDataFromFile(const char* filePath)
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

std::vector<char> loadDataFromFileByTask(const char* filePath)
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
		Condition_Cancel(GetResultOfTask(t1) == nullptr),
		WaitWhile(FileSystem::isFileIOJobAvailable() == false),)
	{
		return openFile(GetResultOfTask(t1));
	}
	TaskProcessNext(t3)//, Condition_Cancel(GetResultOfTask(t2) == nullptr))
	{
		//bool isFileExist = GetResultOfTask(e0);
		void* fileDescriptor = GetResultOfTask(t2);

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

	ITaskManager* taskManager = getDefaultTaskManager();
	ITaskKey* taskKey = taskManager->createTask(std::move(openFileTasks));

	taskManager->commitTask(taskKey);

	return std::vector<char>();
}

namespace KeyA {
	struct First : BindingKey {};
	struct Second : BindingKey {};
	struct Third : BindingKey {};
	struct Forth : BindingKey {};
	struct Fifth : BindingKey {};
}

namespace KeyB {
	struct First : BindingKey {};
	struct Second : BindingKey {};
	struct Third : BindingKey {};
}

std::vector<char> openReadAndCopyFromItIfExists(const char* filePath)
{
	ITaskManager* taskManager = getDefaultTaskManager();

	auto openFileTaskChainA = Dependency(
		Task(isExist, filePath),
		Task<KeyA::First>(openFile, filePath),
		Task<KeyA::Second>(WaitWhile(FileSystem::isFileIOJobAvailable()), getSize, KeyA::First()),
		Task<KeyA::Third>(allocateMemory, KeyA::Second()),
		Task<KeyA::Forth>(readFile, KeyA::First(), KeyA::Third(), KeyA::Second()),
		Task<KeyA::Fifth>( ProcessBlock() {
			int& readResult = GetResult(KeyA::Forth);
			std::vector<char>& memory = GetResult(KeyA::Third);

			if (readResult != 0)
				return std::vector<char>();
			else
				return memory;
		})
	);

	{
		ITaskKey* taskKey = taskManager->createTask(std::move(openFileTaskChainA));
		taskManager->commitTask(taskKey);
	}

	auto integral_expression = []() { return 5; };

	auto chainConnectedTaskWithArg = Dependency(
		Task<KeyB::First>( ProcessBlock(filePath)
	{

		return isExist(filePath) ? filePath : nullptr;

	}),
		Task<KeyB::Second>(
			Condition_Cancel(GetResult(KeyB::First) == nullptr),
			WaitWhile(FileSystem::isFileIOJobAvailable() == false),
			ProcessBlock()
	{

		return openFile(GetResult(KeyB::First));

	}),
		Task<KeyB::Third>(
			Condition_Cancel(GetResult(KeyB::Second) == nullptr),
			ProcessBlock()
	{

		void* fileDescriptor = GetResult(KeyB::Second);
		
		size_t fileSize = getSize(fileDescriptor);

		std::vector<char> memory = allocateMemory(fileSize);

		int result = readFile(fileDescriptor, memory, fileSize);
		if (result != 0)
			memory = std::vector<char>();

		return memory;
	}));


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

	return std::vector<char>();
}

void test_ver2()
{
	loadDataFromFileByTask("");
	auto debugTask = Task("Task");

	auto debugChain =
		Dependency(
			Task("0")
			, Task("1")
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

	printf("TaskWrittenSize: %zu\n", sizeof(T));

	print(r);

}

void MemberFunctionTest()
{
	ITaskManager* taskManager = getDefaultTaskManager();

	// Member Function Test
	struct MemberFunctionTest
	{
		bool func(const char*) { return false; }
	};
	MemberFunctionTest memberFunctionTest;
	auto testTask = Task(&MemberFunctionTest::func, &memberFunctionTest, "test code");

	//ITaskKey* taskKey = taskManager->createTask(Dependency(move(testTask)));
	ITaskKey* taskKey = taskManager->createTask(move(testTask));
	taskManager->commitTask(taskKey);
}