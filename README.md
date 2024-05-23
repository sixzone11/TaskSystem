# TaskSystem Interface

손쉬운 Task 작성을 위한 인터페이스를 제공합니다.

## 작성 예

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

## 작성된 순차 프로그램의 변환 예

``` diff
std::vector<char> openReadAndCopyFromIfExistsDirect(const char* filePath)
{
-    { // diff 전후의 시각적 차이를 줄이기 위한 dummy scope
+   TaskKey task = TaskManager::createTask( Chain(
-       const bool bExist = isExist(filePath);
-       if (bExist == false)
-           return std::vector<char>();
+       Task<KeyB::First>( ProcessBlock(filePath) {
+       return isExist(filePath) ? filePath : nullptr;
+    }),
+
+       Task<KeyB::Second>(
+           Condition_Cancel(GetResult(KeyB::First) == nullptr),
-
-       void* fileDescriptor = openFile(filePath);
+           ProcessBlock() {
+       return openFile(GetResult(KeyB::First));
+   }),

-       if (fileDescriptor == nullptr)
-           return std::vector<char>();
+       Task<KeyB::Third>(
+           Condition_Cancel(GetResult(KeyB::Second) == nullptr),
+           ProcessBlock()
+   {
+       void* fileDescriptor = GetResult(KeyB::Second);
        size_t fileSize = getSize(fileDescriptor);

        std::vector<char> memory = allocateMemory(fileSize);

        int result = readFile(fileDescriptor, memory, fileSize);
        if (result != 0)
            return std::vector<char>();

        return memory;
-   }
+   }));
+   TaskManager::waitForComplete(task);
+   return task.getResult<KeyB::Third>();
}
```
