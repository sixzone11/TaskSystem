# TaskSystem Interface

Task 작성을 위한 인터페이스 구성

``` c++
////// In thread X
TaskProcessor taskProcessorA, taskProcessorB, taskProcessorC;

// ...
if (required)
	TaskManager::createTask(
		Chain(
			Task(taskProcessorA, isExist, filePath), // thread X
			Task<KeyA::First>(openFile, filePath), // any thread
			Task<KeyA::Second>(getSize, KeyA::First()), // any thread
			Task<KeyA::Third>(taskProcessorB, allocateMemory, KeyA::Second()), // thread X
			Task<KeyA::Forth>(readFile, KeyA::First(), KeyA::Third(), KeyA::Second()), // any thread
			Task( taskProcessorC, TaskBlock() { // thread X
				int readResult = GetResult(KeyA::Forth);
				std::vector<char>& memory = GetResult(KeyA::Third);

				if (readResult != 0)
					return std::vector<char>();
				else
					return memory;
			})
		)
	);

// ...
const uint32 maxCountInLoop = 100;
taskProcessorA.process( maxCountInLoop );
// ... any other processes
taskProcessorB.process( maxCountInLoop );
// ... any other processes
taskProcessorC.process( maxCountInLoop );
```
