#include "pch.h"

#include "TaskSystem/TaskSystem.h"

template<typename T>
struct Print {
	struct Offsets { constexpr static const auto _var = T::_offsets; };
	struct Links { constexpr static const auto _var = T::_links; };
	struct Inputs { constexpr static const auto _var = T::_inputs; };
	struct Outputs { constexpr static const auto _var = T::_outputs; };
};

void print(std::vector<uint32_t>& v, const char* name)
{
	printf("%s: %zu\n\t[ ", name, v.size());
	for (auto& e : v)
	{
		printf("%u ", e);
	}
	printf("]\n");
}

// helper function to print a tuple of any size
template<class Tuple, std::size_t N>
struct TuplePrinter
{
	static void print(const Tuple& t)
	{
		TuplePrinter<Tuple, N - 1>::print(t);
		std::cout << ", " << std::get<N - 1>(t)._taskName;
	}
};

template<class Tuple>
struct TuplePrinter<Tuple, 1>
{
	static void print(const Tuple& t)
	{
		std::cout << std::get<0>(t)._taskName;
	}
};

template<typename... Args, std::enable_if_t<sizeof...(Args) == 0, int> = 0>
void printDefines(const std::tuple<Args...>& t)
{
	std::cout << "()\n";
}

template<typename... Args, std::enable_if_t<sizeof...(Args) != 0, int> = 0>
void printDefines(const std::tuple<Args...>& t)
{
	std::cout << "(";
	TuplePrinter<decltype(t), sizeof...(Args)>::print(t);
	std::cout << ")\n";
}

void printDefines(const TaskDefine& t)
{
	std::cout << "(" << t._taskName << ")\n";
}
// end helper function


template<typename TaskWritten>
void print(const TaskWritten& taskWritten)
{
	using TaskDefineTuple = typename std::tuple_element<0, TaskWritten>::type;
	using TaskMeta = typename std::tuple_element<1, TaskWritten>::type;
	const auto& taskDefines = std::get<0>(taskWritten);

	auto offsets = copyToVec<typename Print<TaskMeta>::Offsets>();
	auto links = copyToVec<typename Print<TaskMeta>::Links>();
	auto inputs = copyToVec<typename Print<TaskMeta>::Inputs>();
	auto outputs = copyToVec<typename Print<TaskMeta>::Outputs>();

	print(offsets, "Offsets");
	print(links, "Links");
	print(inputs, "Inputs");
	print(outputs, "Outputs");
	printDefines(taskDefines);

	//printf("TaskNames:\n");
	//for (const TaskDefine& taskDefine : taskDefines)
	//{
	//	printf("\t%s\n", taskDefine._taskName);
	//}
}


#define Chain       TaskWriter::chain
#define Junction    TaskWriter::junction
#define Task        TaskWriter::task


bool isExist(const char* filePath) { return true; }
void* openFile(const char* filePath) { return nullptr; }
size_t getSize(void* fileDescriptor) { return 0; }
std::vector<char> allocateMemory(size_t size) { return std::vector<char>(size); }
int readFile(void* fileDescriptor, std::vector<char>& dstMemory, size_t size) { return 0; }

std::vector<char> openReadAndCopyFromIfExistsDirect(const char* filePath)
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

template<typename T>
auto bind(T&& binding)
{
	
}

template<typename T>
struct FutureResult
{
	T value;

	operator T && ()
	{
		return value;
	}
};

template<typename RetType, typename ... ParamTypes>
FutureResult<RetType> getFutureResult(RetType(*func)(ParamTypes...))
{
	return FutureResult<RetType>();
};

template<typename T>
auto getFutureResult(T&& arg)
{
	return FutureResult<typename lambda_details<T>::RetType>();
}

template<typename PrevFunc>
auto result(PrevFunc&& func)
{
	return getFutureResult(func);
}

template<size_t... Index>
auto delayed()
{
	return std::make_tuple(ResultHolder{}, std::array<ResultHolder, sizeof...(Index)> {});
}

bool testMultiParameters(const char*, int, bool, float) { return false; }

struct MemberFunctionTest
{
	uint32_t MemberFunctionA() { return 0; }
};

std::vector<char> openReadAndCopyFromItIfExists(const char* filePath)
{
	std::function<decltype(isExist)> a;

	struct Test
	{
		bool func(const char*) { return false; }
	};

	//auto testLambda = [result(allocateMemory)]()
	auto testLambda = [filePath]()
	{

	};

	// Lambda 에 대한 FutureResult를 만들어내는데에 함수를 적용하면 문제가 있으므로 우회시켜야 함.
	//auto resultedValue = result(&MemberFunctionTest::MemberFunctionA);

	lambda_details<decltype(testLambda)>::minimum_argument_count;
	//decltype(std::declval<decltype(testLambda)>()((void)0, any_argument{}), void());

	Task(readFile, result(openFile), result(allocateMemory), result(getSize));

	if (isExist(filePath) == false)
		return std::vector<char>();

	auto delayedValue = delayed<0, 1>();

	Chain(
		Task(isExist, filePath),
		Task(openFile, filePath),
		Task(getSize, result(openFile)),
		Task(allocateMemory, result(getSize)),
		Task(readFile, result(openFile), result(allocateMemory), result(getSize)),
		Task([](int readResult, std::vector<char>& memory)
			{
				if (readResult != 0)
					return std::vector<char>();
				else
					return memory;
			}, result(readFile), result(allocateMemory))
		);

	auto getFilePath = [](const char* path) { return path; };

	auto chainConnectedTaskWithArg = Chain(
		Task([filePath, getFilePath]()
			{
				if (filePath == nullptr)
					return (const char*)nullptr;

				if (isExist(filePath) == false)
					return (const char*)nullptr;

				return getFilePath(filePath);
			}),
		Task(openFile, result(getFilePath)), delayed<0>(),
		Task([fd = result(openFile), r = result(allocateMemory)]()
			{
				size_t size = getSize(fd.value);
				
				std::vector<char> copied = r.value;
				if (readFile(fd.value, copied, size) == 0)
					return r.value;
			}), delayed<0, 1>()
		);

	Task(isExist, filePath);

	//auto openReadAndCopyFromItIfExist = Chain(
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

void test2()
{
	auto debugTask = Task("Task");

	auto debugChain =
		Chain(
			Task("0")
			, Task("1")
		);

	auto debugJunction =
		Junction(
			Task("0")
			, Task("1")
		);

	auto debugChainAndJunction =
		Chain(
			Task("0")
			, Junction(
				Task("1")
				, Task("2")
			)
		);

	auto testChain3 =
		Chain(
			Task("0")
			, Task("1")
			, Task("2")
			, Task("3")
			, Task("4")
		);

	auto testChainOfJunctions =
		Chain(
			Task("0"),
			Junction(
				Task("1")
				, Task("2")
				, Task("3")
			)
			, Junction(
				Task("4")
				, Task("5")
			)
		);

	 auto testTasks =
	     Junction(
	         std::move(testChainOfJunctions),
	         Chain(
	             Task("6")
	             , Junction(
	                 Task("7")
	                 , Task("8")
	             )
	         )
	         , Chain(
	             Task("9")
	             , Task("10")
	             , Task("11")
	         )
	     );

	auto testTasks2 =
		Chain(
			Task("00")
			,
			Junction(
				std::move(testChainOfJunctions),
				Chain(
					Task("7")
					, Junction(
						Task("8")
						, Task("9")
					)
				)
				, Chain(
					Task("10")
					, Task("11")
					, Task("12")
				)
			)
			, Junction(
				Task("13")
				, Task("14")
				, Task("15")
			)
		);

	 auto result = Chain(
	 	Task("AsyncTask""1"),
	 	Junction(
	 		Chain(
	 			Task("JunctionTask""1_1"),
	 			Task("JunctionTask""1_2")),
	 		Task("JunctionTask""2"),
	 		Junction(
	 			Chain(
	 				Task("JunctionTask""1_1"),
	 				Task("JunctionTask""1_2"),
	 				Junction(
	 					Chain(
	 						Task("JunctionTask""1_1"),
	 						Task("JunctionTask""1_2")),
	 					Task("JunctionTask""2")
	 				)),
	 			Task("JunctionTask""2")
	 		)
	 	),
	 	Chain(
	 		Chain(
	 			Chain(
	 				Chain(
	 					Task("AsyncTask""3"))))),
	 	Task("AsyncTask""4"),
	 	Task("AsyncTask""5")
	 );

	auto& r = debugJunction;
	using T = std::remove_reference_t<decltype(r)>;

	printf("TaskWrittenSize: %zu\n", sizeof(T));

	print(r);

}