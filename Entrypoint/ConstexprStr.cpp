#include "pch.h"

#if _MSVC_LANG < 202000L || __cplusplus < 202000L

void constexpr_str_test()
{
	printf("Passing a literal string to template parameter is not supported prior to C++ 20...\n");
}

#else

template <typename CharT, std::size_t N>
struct basic_fixed_string
{
	constexpr basic_fixed_string(const CharT(&foo)[N])
	{
		std::copy_n(foo, N, m_data);
	}
	auto operator<=>(const basic_fixed_string&) const = default;
	CharT m_data[N];
};

struct IResource {};
struct ITexture : public IResource {};
struct IBuffer : public IResource {};

void bindTexture(uint32_t bindingKey, ITexture* texture) {}
void bindTexture(uint32_t bindingKey, std::vector<ITexture*>& textures) {}
void bindBuffer(uint32_t bindingKey, ITexture* texture) {}

std::unordered_map<std::string, uint32_t> global_string_map;
uint32_t global_accum[3][256] = {};

template<basic_fixed_string binding_name, typename ResourceT, typename... Args>
struct Binding
{
	Binding(ResourceT& resource, Args&&... args)
		: _resource(resource)
		, _args(std::forward<Args>(args)...)
	{
	}

	ResourceT& _resource;
	std::tuple<Args...> _args;
};

template<basic_fixed_string binding_name>
auto Bind(ITexture& texture)
{ return Binding<binding_name, ITexture>(texture); }

template<basic_fixed_string binding_name>
auto Bind(std::vector<ITexture*>& textures)
{ return Binding<binding_name, std::vector<ITexture*>>(textures); }

template<basic_fixed_string binding_name>
auto Bind(IBuffer& buffer)
{ return Binding<binding_name, IBuffer>(buffer); }

template<typename... BindingTs>
struct BindingBlock
{
	std::tuple<BindingTs...> _bindings;
	bool _condition;
};

template<typename... BindingTs>
auto Condition(const bool condition, BindingTs&&... bindings)
{
	return BindingBlock<BindingTs...>{ std::forward_as_tuple<BindingTs>(bindings...), condition };
}

template<basic_fixed_string binding_name, typename ResourceT, typename... Args>
void bindResourceInternal(uint32_t bindingKey, Binding<binding_name, ResourceT, Args...>&& binding)
{
	static_assert(false, "Not implemented for ResourceT");
	// call a bind function for a specific type of a resource, passed by arg
}

template<basic_fixed_string binding_name, typename... Args>
void bindResourceInternal(uint32_t bindingKey, Binding<binding_name, ITexture, Args...>&& binding)
{
	// call a bind function for a specific type of a resource, passed by arg
	global_accum[0][bindingKey]++;
}

template<basic_fixed_string binding_name, typename... Args>
void bindResourceInternal(uint32_t bindingKey, Binding<binding_name, std::vector<ITexture*>, Args...>&& binding)
{
	// call a bind function for a specific type of a resource, passed by arg
	global_accum[1][bindingKey]++;
}

template<basic_fixed_string binding_name, typename... Args>
void bindResourceInternal(uint32_t bindingKey, Binding<binding_name, IBuffer, Args...>&& binding)
{
	// call a bind function for a specific type of a resource, passed by arg
	global_accum[2][bindingKey]++;
}

template<typename... BindingTs>
void bindResourceInternal(uint32_t bindingKey, BindingBlock<BindingTs...>&& bindingBlock)
{
	// call a bind function for a specific type of a resource, passed by arg
	static_assert(false, "not yet");
}

template<basic_fixed_string binding_name, typename ResourceT, typename... Args>
uint32_t getBindingKey(Binding<binding_name, ResourceT, Args...>& /*binding*/)
{
	auto result = global_string_map.insert({ binding_name.m_data, uint32_t(-1) });
	if (result.second == true)
	{
		auto& resultPair = result.first;
		resultPair->second = uint32_t(global_string_map.size() - 1);
	}

	return result.first->second;
};

template<typename... BindingTs>
std::initializer_list<uint32_t> getBindingKey(BindingBlock<BindingTs...>& bindingBlock )
{
	using BindingTuple = decltype(BindingBlock<BindingTs...>::_bindings);
	constexpr size_t NumBindingsInBlock = std::tuple_size_v<BindingTuple>;
	
	return [] <std::size_t... IndexPack> (BindingBlock<BindingTs...>& bindingBlock, std::index_sequence<IndexPack...>)
	{
		return { getBindingKey(std::get<IndexPack>(bindingBlock._bindings))..., };
	}( bindingBlock, std::make_index_sequence<NumBindingsInBlock>() );
};

template<typename T>
struct BindingMeta;

template<basic_fixed_string binding_name, typename ResourceT, typename... Args>
struct BindingMeta<Binding<binding_name, ResourceT, Args...>>
{
	static constexpr size_t _count = 1;
};

template<typename... BindingTs>
struct BindingMeta<BindingBlock<BindingTs...>>
{
	static constexpr size_t _count = (BindingMeta<BindingTs>::_count + ...);
};

template<typename... BindingTs>
constexpr size_t getNumBindings()
{
	return (BindingMeta<BindingTs>::_count + ...);
}

template<typename... BindingTs>
void bindResources(BindingTs&&... bindings)
{
	constexpr size_t NumBindings = getNumBindings<BindingTs...>();

	struct LocalBinder
	{
		uint32_t _bindingKeys[NumBindings];
	};

	static LocalBinder localBinder = [&bindings...](void) { return LocalBinder{ { getBindingKey(bindings)..., } }; } ();

	[] <std::size_t... IndexPack, typename... BindingTs> (LocalBinder& localBinder, std::index_sequence<IndexPack...>, BindingTs&&... bindings) {
		(bindResourceInternal(localBinder._bindingKeys[IndexPack], std::forward<BindingTs>(bindings)), ...);
	} (localBinder, std::make_index_sequence<NumBindings>(), std::forward<BindingTs>(bindings)...);
}

void constexpr_str_test()
{
	ITexture tex1;
	ITexture tex2;
	ITexture tex3;
	IBuffer buf1;
	IBuffer buf2;
	IBuffer buf3;

	std::vector<ITexture*> textures{ &tex1, &tex2, &tex3 };

	uint32_t count = rand() * rand() % 50;
	printf("count: %d\n", count);
	for (uint32_t i = 0; i < count; ++i)
	{
		bindResources( // New combination
			Bind<"g_texA">(tex1),
			Bind<"g_texB">(tex2),
			Bind<"g_texC">(tex3));
	}

	bindResources( // Same as above in both Literals: <"g_texA", "g_texB", "g_texC"> and ResourceT: <T, T, T>
		Bind<"g_texA">(tex3),
		Bind<"g_texB">(tex2),
		Bind<"g_texC">(tex1));


	bindResources( // New combination in ResourceT: <T, B, B>
		Bind<"g_texA">(tex1),
		Bind<"g_texB">(buf2),
		Bind<"g_texC">(buf3));

	bindResources( // New combination Literals: <"g_texA", "g_bufA"> and ResourceT: <T, B>
		Bind<"g_texA">(tex1),
		Bind<"g_bufA">(buf1));

	bindResources( // New combination Literals: <"g_texA", "g_texArray", "g_bufA"> and ResourceT: <T, v<T>, B>
		Bind<"g_texA">(tex1),
		Bind<"g_texArray">(textures),
		Bind<"g_bufA">(buf1));
}

#endif