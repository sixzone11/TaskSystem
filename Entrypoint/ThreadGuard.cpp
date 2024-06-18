#include "pch.h"

#include <thread>
#include <chrono>
#include "ThreadGuard.h"

using namespace std::chrono_literals;

struct Base				{ virtual ~Base() {} };
struct Derived : Base	{ virtual ~Derived() {} };

struct Component		{};

struct Object
{
	std::vector<std::unique_ptr<Component>> _components;
};

struct Level
{
private:
	SET_AS_OWNER_OF(Object);

public:
	Level(int a) {}
	
	std::unordered_map<std::string, std::shared_ptr<Object>> _loadedObjects;

	std::unique_ptr<Base> _derived;
};

struct LevelManager
{
	std::unordered_map<std::string, std::shared_ptr<Level>> _levels;
	ThreadGuard<std::unique_ptr<Level>> _instantLevel;
	std::unique_ptr<ThreadGuard<Level>> _instantLevelIn;
};

uint32_t g_step = 0;

void thread_work(LevelManager* levelManager)
{
	for (int i = 0; i < 20; ++i)
	{
		{
			ReadOnlyAccessor ro(levelManager->_instantLevel);

			// Do with 'ro'
			ro.get()->_loadedObjects;
			ro->_loadedObjects;
		}
		{
			ReadOnlyAccessor ro(*levelManager->_instantLevelIn.get());
			ro.get()._loadedObjects;
		}
		std::this_thread::sleep_for(100ms);
	}

	while (g_step == 0);

	while (g_step == 1);

	while (g_step == 2);
}

void generateLevels(LevelManager& levelManager);

void threadguard_test()
{
	//static_assert(sizeof(ThreadGuard<Level>) == sizeof(Level), "same size");
	//static_assert(is_complete<TestTypeToCheck>::value, "this is not zero");
	static_assert(std::tuple_size_v<decltype(to_tuple(std::declval<IntermediateType>()))>, "size is 0");
	static_assert(FindFieldTraverseInTuple<decltype(to_tuple(std::declval<IntermediateType>()))>(), "is class?");
	//static_assert(FindTypeTraverseInStruct < TypeA, FindingType, true>::result, "Has no FindingType");
	static_assert(FindFieldTraverseInStruct<TypeA, true>::result, "Has no FindingType");

	TraverseInStruct<TypeA>::has_finding_field_tuple aba;
	static_assert(std::tuple_size_v<decltype(aba)> == 1, "IntermediateType has 1");
	static_assert(std::is_same_v<std::tuple_element_t<0, decltype(aba)>, FindingType<int>>, "That is FindingType<int>");

	LevelManager levelManager;

	generateLevels(levelManager);

	std::thread t1(thread_work, &levelManager);

	{
		ReadWriteAccessor rw(levelManager._instantLevel);
		levelManager._instantLevel;
		std::this_thread::sleep_for(2000ms);
	}

	g_step = 1;

	{
		ReadWriteAccessorForGroup<Level> group3;
	}

	g_step = 2;


	g_step = 3;


	t1.join();
}

std::string generateName()
{
	constexpr static char alphabetdigit[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	constexpr size_t max_alphabetdigit = sizeof(alphabetdigit) - 1;

	char randomName[128];
	uint32_t length = (rand() % (128 - 16)) + 16;
	for (uint32_t j = 0; j < length; ++j)
		randomName[j] = alphabetdigit[rand() % max_alphabetdigit];

	return std::string(randomName, length);
}

void generateLevels(LevelManager& levelManager)
{
	uint32_t numLevels = uint32_t((rand() % (192+1)) + 64);
	for (uint32_t i = 0; i < numLevels; ++i)
	{
		levelManager._levels.insert({ generateName(), {} });
	}
}