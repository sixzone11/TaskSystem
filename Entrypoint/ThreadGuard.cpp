#include "pch.h"

#include <thread>
#include <chrono>
#include "ThreadGuard.h"

using namespace std::chrono_literals;

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

struct Base				{ virtual ~Base() {} };
struct Derived : Base	{ virtual ~Derived() {} };

struct Component
{
	REGISTER_THREAD_SATEFY(Component);
	SET_AS_OWNER_OF_TYPES();
};

struct Object
{
	REGISTER_THREAD_SATEFY(Object);
	SET_AS_OWNER_OF_TYPES(Component);

	std::vector<std::unique_ptr<Component>> _components;
};

struct Level
{
public:
	REGISTER_THREAD_SATEFY(Level);
	SET_AS_OWNER_OF_TYPES(Object);

public:
	Level(int a) {}


public:
	void workLV0();
	void workLV0_const() const;

private:
	std::unordered_map<std::string, std::shared_ptr<Object>> _loadedObjects;

	std::unique_ptr<Base> _derived;
};

void Level::workLV0_const() const
{
	// Do with 'ro'
	_loadedObjects;
}


struct LevelManager
{
public:
	REGISTER_THREAD_SATEFY(LevelManager);
	SET_AS_OWNER_OF_TYPES(Level);

public:
	void main_generateLevels();
	void main_LM0();

	void workLM0();

private:
	std::unordered_map<std::string, std::shared_ptr<Level>> _levels;
	ThreadGuard<std::unique_ptr<Level>> _instantLevel;
	std::unique_ptr<ThreadGuard<Level>> _instantLevelIn;
};

uint32_t g_step = 0;

void LevelManager::workLM0()
{
	for (int i = 0; i < 20; ++i)
	{
		{
			ReadAccessor ro(_instantLevel);

			// Do with 'ro'
			ro->workLV0_const();
		}
		//{
		//	ReadAccessor ro(*_instantLevelIn.get());
		//	ro->workLV0_const();
		//}
		std::this_thread::sleep_for(100ms);
	}
}

void LevelManager::main_LM0()
{
	WriteAccessor rw(_instantLevel);

	// do for _instantLevel;

	std::this_thread::sleep_for(20000ms);
}

void thread_work(ThreadGuard<LevelManager>* levelManager)
{
	{
		WriteAccessor rw(*levelManager);
		rw->workLM0();
	}

	while (g_step == 0);

	while (g_step == 1);

	while (g_step == 2);
}


void threadguard_test()
{
	ThreadGuard<LevelManager> levelManager;

	std::thread t1(thread_work, &levelManager);

	{
		WriteAccessor rw(levelManager);
		rw->main_generateLevels();

		rw->main_LM0();
	}

	g_step = 1;

	g_step = 2;

	g_step = 3;


	t1.join();
}

void LevelManager::main_generateLevels()
{
	int numLevels = int((rand() % (192+1)) + 64);
	for (int i = 0; i < numLevels; ++i)
	{
		_levels.insert({ generateName(), std::make_shared<Level>(i) });
	}

	_instantLevel = std::make_unique<Level>(5);
	
	WriteAccessor rwForInstant(_instantLevel);
	// ... initialize for instance level
}

























void wip_or_test_code()
{
	//static_assert(sizeof(ThreadGuard<Level>) == sizeof(Level), "same size");
	//static_assert(is_complete<TestTypeToCheck>::value, "this is not zero");
	//static_assert(std::tuple_size_v<decltype(to_tuple(std::declval<IntermediateType>()))>, "size is 0");
	//static_assert(FindFieldTraverseInTuple<decltype(to_tuple(std::declval<IntermediateType>()))>(), "is class?");
	//static_assert(FindTypeTraverseInStruct < TypeA, FindingType, true>::result, "Has no FindingType");
	//static_assert(FindFieldTraverseInStruct<TypeA, true>::result, "Has no FindingType");

	//TraverseInStruct<TypeA>::has_finding_field_tuple aba;
	//static_assert(std::tuple_size_v<decltype(aba)> == 1, "IntermediateType has 1");
	//static_assert(std::is_same_v<std::tuple_element_t<0, decltype(aba)>, FindingType<int>>, "That is FindingType<int>");


	{ // integral type fail
		//ThreadGuard<int> tg_int;
		//WriteAccessor rwa_int(tg_int);
		//rwa_int.get() = 5;
	}

	//using TypeB = TraverseInStruct<decltype(LevelManager::_instantLevel)>::has_finding_field_tuple;
	//static_assert(std::tuple_size_v<TypeB> == 0, "maybe ThreadGuard<Level>");

	//decltype(to_tuple(ThreadGuard<int>())) test;
	//static_assert(std::tuple_size_v<decltype(test)> > 0, "");

	//ThreadGuard<int> aadad;
	//aadad._internal;
	//using TypeC = TraverseInTuple<decltype(to_tuple(std::declval<ThreadGuard<int>>()))>::has_finding_field_tuple;
	//static_assert(std::tuple_size_v<TypeC> == 0, "maybe ThreadGuard<Level>");
}