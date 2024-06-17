#include "pch.h"

#include "ThreadGuard.h"

struct Component
{

};

struct Object
{
	std::vector<std::unique_ptr<Component>> _components;
};

struct Level
{
	SET_AS_OWNER_OF(Object);
	
	std::unordered_map<std::string, std::shared_ptr<Object>> _loadedObjects;
};

struct LevelManager
{
	std::unordered_map<std::string, std::shared_ptr<Level>> _levels;
	std::unique_ptr<ThreadGuard<Level>> _instantLevel;
};

void threadguard_test()
{
	//static_assert(sizeof(ThreadGuard<Level>) == sizeof(Level), "same size");
	//static_assert(is_complete<TestTypeToCheck>::value, "this is not zero");
	static_assert(std::tuple_size_v<decltype(to_tuple(std::declval<IntermediateType>()))>, "size is 0");
	static_assert(FindFieldTraverseInTuple<decltype(to_tuple(std::declval<IntermediateType>()))>(), "is class?");
	//static_assert(FindTypeTraverseInStruct < TypeA, FindingType, true>::result, "Has no FindingType");
	static_assert(FindFieldTraverseInStruct<TypeA, true>::result, "Has no FindingType");

	LevelManager levelManager;

	{
		ReadOnlyAccessorForGroup<Level> group1;
		ReadOnlyAccessorForGroup<OwnerOf<Object>> group2;
	}


	{
		ReadWriteAccessorForGroup<Level> group3;
	}
}