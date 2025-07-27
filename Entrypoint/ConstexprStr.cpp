#include "pch.h"

#include "TaskSystem/tuple_utility.h"

///////////////////////////////////////////////////////////////////////
//
// Bulk Literal String Binding
//
///////////////////////////////////////////////////////////////////////

// Note(jiman): Bulk literal-string binding.
void constexpr_str_test();

#if _MSVC_LANG < 202000L || __cplusplus < 202000L

void constexpr_str_test()
{
	printf("Passing a literal string to template parameter is not supported prior to C++ 20...\n");
}

#else

///////////////////////////////////////////////////////////////////////
// basic_fixed_string

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

template <typename CharT>
struct basic_fixed_string<CharT, 0>
{
	constexpr basic_fixed_string(nullptr_t) {};
	auto operator<=>(const basic_fixed_string&) const = default;
};

constexpr basic_fixed_string<char, 0> null_fixed_string{ nullptr };
constexpr basic_fixed_string<wchar_t, 0> null_fixed_wstring{ nullptr };


///////////////////////////////////////////////////////////////////////
// Sample RHI-related and commands

struct IResource
{
	uint32_t _type;
	uint32_t _usage;
	uint32_t _size;
	uint32_t _format;
};

struct ITexture : public IResource
{
	uint32_t _numMips;
	uint32_t _numArrays;
	uint32_t _numDepth;
};

struct IBuffer : public IResource
{
	uint32_t _stride;
};

void bindTexture(uint32_t bindingKey, ITexture* texture) {}
void bindTexture(uint32_t bindingKey, std::vector<ITexture*>& textures) {}
void bindBuffer(uint32_t bindingKey, ITexture* texture) {}

struct ShaderResource
{
	uint32_t _bindingInformationA = 0;
	uint32_t _bindingInformationB = 0;
	uint32_t _bindingInformationC = 0;
	uint32_t _bindingInformationD = 0;
};

struct ShaderResourceBindingMap
{
	std::unordered_map<uint32_t, ShaderResource>		_shaderResourceMap;
};

struct RenderResourceViewBindingHandle {
	uint32_t											_offset = uint32_t(-1);
	uint32_t											_variableBindingKey = uint32_t(-1);
	const ShaderResource*								_shaderResource;
};

struct AutoBindingContext
{
	struct AutoBindingIdentifier
	{
		const ShaderResourceBindingMap*					_shaderResourceBindingMap = nullptr;
		std::vector<RenderResourceViewBindingHandle>	_bindingHandles;
		std::vector<RenderResourceViewBindingHandle>	_bindingHandlesForVariable;
		uint32_t										_index = uint32_t(-1);
	};

	std::vector<AutoBindingIdentifier>					_autoBindingIdentifiers;
};


///////////////////////////////////////////////////////////////////////
// global_string_map

std::unordered_map<std::string, uint32_t> global_string_map;
uint32_t global_accum[3][256] = {};

uint32_t getBindingKey(const std::string& bindingName)
{
	auto result = global_string_map.insert({ bindingName, uint32_t(-1) });
	if (result.second == true)
	{
		auto& resultPair = result.first;
		resultPair->second = uint32_t(global_string_map.size() - 1);
	}

	return result.first->second;
};


///////////////////////////////////////////////////////////////////////
// Binding Primitive

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

template<typename ResourceT, typename... Args>
struct Binding<null_fixed_string, ResourceT, Args...>
{
	Binding(const uint32_t& bindingKey, ResourceT& resource, Args&&... args)
		: _bindingKey(bindingKey)
		, _resource(resource)
		, _args(std::forward<Args>(args)...)
	{
	}

	const uint32_t& _bindingKey;
	ResourceT& _resource;
	std::tuple<Args...> _args;
};

template<basic_fixed_string binding_name>
auto Bind(ITexture& texture, uint32_t viewIndex = 0, bool nullBindIfInvalid = false) { return Binding<binding_name, ITexture, uint32_t, bool>(texture, std::forward<uint32_t>(viewIndex), std::forward<bool>(nullBindIfInvalid)); }
auto Bind(const uint32_t& bindingKey, ITexture& texture, uint32_t viewIndex = 0, bool nullBindIfInvalid = false) { return Binding<null_fixed_string, ITexture, uint32_t, bool>(bindingKey, texture, std::forward<uint32_t>(viewIndex), std::forward<bool>(nullBindIfInvalid)); }

void setTexture(RenderResourceViewBindingHandle& bindingHandle, ITexture& texture, uint32_t viewIndex, bool nullBindIfInvalid)
{
	std::cout << "Bind texture: " << texture._type << " size: " << texture._size << " viewIndex: " << viewIndex << std::endl;
}

template<basic_fixed_string binding_name>
auto Bind(std::vector<ITexture*>& textures) { return Binding<binding_name, std::vector<ITexture*>>(textures); }
auto Bind(const uint32_t& bindingKey, std::vector<ITexture*>& textures) { return Binding<null_fixed_string, std::vector<ITexture*>>(bindingKey, textures); }

template<basic_fixed_string binding_name>
auto Bind(IBuffer& buffer) { return Binding<binding_name, IBuffer>(buffer); }
auto Bind(const uint32_t& bindingKey, IBuffer& buffer) { return Binding<null_fixed_string, IBuffer>(bindingKey, buffer); }

template<typename... BindingTs>
struct BindingBlock
{
	std::tuple<BindingTs...> _bindings;
	bool _condition;
};

template<typename... BindingTs>
auto BindIf(const bool condition, BindingTs&&... bindings)
{
	return BindingBlock<BindingTs...>{ {std::forward<BindingTs>(bindings)...}, condition };
}

template<size_t Index, typename BindingT, typename... BindingTs>
auto&& getBinding(BindingT&& binding, BindingTs&&... bindings)
{
	if constexpr (Index == 0)
		return std::forward<BindingT>(binding);
	else
		return getBinding<Index - 1>(std::forward<BindingTs>(bindings) ...);
}

template<size_t Index, typename... BindingInBlockTs, typename... BindingTs>
auto&& getBinding(BindingBlock<BindingInBlockTs...>&& binding, BindingTs&&... bindings)
{
	if constexpr (Index < sizeof...(BindingInBlockTs))
		return getBinding<Index>(std::get<Index>(std::forward<std::tuple<BindingInBlockTs...>>(binding._bindings)));
	else
		return getBinding<Index - sizeof...(BindingInBlockTs)>(std::forward<BindingTs>(bindings) ...);
}


///////////////////////////////////////////////////////////////////////
// Binding Meta

template<typename T>
struct BindingMeta;

template<basic_fixed_string binding_name, typename ResourceT, typename... Args>
struct BindingMeta<Binding<binding_name, ResourceT, Args...>>
{
	using FlattenTupleT = std::tuple<Binding<binding_name, ResourceT, Args...>>;
	constexpr static const std::tuple<size_t> _offsets{ 0 };

	static constexpr size_t _count = 1;
	static constexpr bool _isBlock = false;
	static constexpr bool _isVariableStringBinding = false;

	static uint32_t getBindingKey() { return ::getBindingKey(binding_name.m_data);}
};

template<typename ResourceT, typename... Args>
struct BindingMeta<Binding<null_fixed_string, ResourceT, Args...>>
{
	using FlattenTupleT = std::tuple<Binding<null_fixed_string, ResourceT, Args...>>;
	constexpr static const std::tuple<size_t> _offsets{ 0 };

	static constexpr size_t _count = 1;
	static constexpr bool _isBlock = false;
	static constexpr bool _isVariableStringBinding = true;

	static uint32_t getBindingKey() { return uint32_t(-1); }
};

template<typename... BindingTsInBlock>
struct MultipleBindingMeta;

template<typename FirstBindingT, typename... NextBindingTs>
struct MultipleBindingMeta<FirstBindingT, NextBindingTs...> : MultipleBindingMeta<NextBindingTs...>
{
	using FlattenTupleT = decltype(std::tuple_cat(std::declval<typename BindingMeta<FirstBindingT>::FlattenTupleT>(), std::declval<typename MultipleBindingMeta<NextBindingTs...>::FlattenTupleT>()));

	// 0,
	// BindingTs[0]._count,
	// BindingTs[0]._count + BindingTs[1]._count,
	// ...
	// BindingTs[0]._count + BindingTs[1]._count + ... + BindingTs[N-1]._count

	struct OffsetNextsSrc { constexpr static const auto _var = MultipleBindingMeta<NextBindingTs...>::_offsets; };

	constexpr static const auto _offsets = std::tuple_cat(
		BindingMeta<FirstBindingT>::_offsets,
		AddByN<OffsetNextsSrc, BindingMeta<FirstBindingT>::_count>::_var
	);
};

template<typename BindingT>
struct MultipleBindingMeta<BindingT>
{
	using FlattenTupleT = typename BindingMeta<BindingT>::FlattenTupleT;

	constexpr static const auto _offsets = BindingMeta<BindingT>::_offsets;
};


template<typename... BindingTs>
struct BindingMeta<BindingBlock<BindingTs...>>
{
	using FlattenTupleT = typename MultipleBindingMeta<BindingTs...>::FlattenTupleT;

	constexpr static const auto _offsetsInternal = MultipleBindingMeta<BindingTs...>::_offsets;

	constexpr static const std::tuple<size_t> _offsets{ 0 };

	static constexpr size_t _count = (BindingMeta<BindingTs>::_count + ...);
	static constexpr bool _isBlock = true;
	static constexpr bool _isVariableStringBinding = (BindingMeta<BindingTs>::_isVariableStringBinding || ...);
};


///////////////////////////////////////////////////////////////////////
// Binding Commands

template<basic_fixed_string binding_name, typename ResourceT, typename... Args>
void bindResourceInternal(RenderResourceViewBindingHandle& bindingKey, Binding<binding_name, ResourceT, Args...>&& binding)
{
	// Note(jiman): #Trick. sizeof(T) == 0 is impossible, but it could be avoid from compile error except that this function is really instantiated by any resason.
	static_assert(sizeof(ResourceT) == 0, "Not implemented for ResourceT");
	// call a bind function for a specific type of a resource, passed by arg
}

template<basic_fixed_string binding_name, typename... Args>
void bindResourceInternal(RenderResourceViewBindingHandle& bindingHandle, Binding<binding_name, ITexture, Args...>&& binding)
{
	// call a bind function for a specific type of a resource, passed by arg
	if (bindingHandle._offset == uint32_t(-1)) return;
	global_accum[0][bindingHandle._offset]++;
	[&] <std::size_t... IndexPack> (std::index_sequence<IndexPack...>) {
		setTexture(bindingHandle, static_cast<ITexture&>(binding._resource), std::get<IndexPack>(std::forward<std::tuple<Args...>>(binding._args)) ... );
	}(std::make_index_sequence<sizeof...(Args)>());
}

template<basic_fixed_string binding_name, typename... Args>
void bindResourceInternal(RenderResourceViewBindingHandle& bindingHandle, Binding<binding_name, std::vector<ITexture*>, Args...>&& binding)
{
	// call a bind function for a specific type of a resource, passed by arg
	if (bindingHandle._offset == uint32_t(-1)) return;
	global_accum[1][bindingHandle._offset]++;
}

template<basic_fixed_string binding_name, typename... Args>
void bindResourceInternal(RenderResourceViewBindingHandle& bindingHandle, Binding<binding_name, IBuffer, Args...>&& binding)
{
	// call a bind function for a specific type of a resource, passed by arg
	if (bindingHandle._offset == uint32_t(-1)) return;
	global_accum[2][bindingHandle._offset]++;
}

template<typename... BindingTs>
void bindResourceInternal(RenderResourceViewBindingHandle bindingHandles[], BindingBlock<BindingTs...>&& bindingBlock)
{
	// call a bind function for a specific type of a resource, passed by arg
	if (bindingBlock._condition == false)
		return;

	using BindingTupleT = std::tuple<BindingTs...>;

	[&bindingHandles] <std::size_t... IndexPack> (std::index_sequence<IndexPack...>, BindingTupleT&& bindings) {
		auto internalCall = [&] <std::size_t TupleIndex, typename BindingT> (BindingT && binding) {
			if constexpr (BindingMeta<std::tuple_element_t<TupleIndex, BindingTupleT>>::_isBlock)
				bindResourceInternal(bindingHandles + std::get<TupleIndex>(BindingMeta<BindingBlock<BindingTs...>>::_offsetsInternal), std::forward<BindingT>(binding));
			else
				bindResourceInternal(bindingHandles[std::get<TupleIndex>(BindingMeta<BindingBlock<BindingTs...>>::_offsetsInternal)], std::forward<BindingT>(binding));
		};

		((internalCall.template operator()<IndexPack>(std::get<IndexPack>(std::forward<BindingTupleT>(bindings)))), ...);
	} (std::make_index_sequence<sizeof...(BindingTs)>(), std::forward<BindingTupleT>(bindingBlock._bindings));
}

struct AutoBindingDescription
{
	std::vector<RenderResourceViewBindingHandle>& _bindingHandles;
	std::vector<RenderResourceViewBindingHandle>& _bindingHandlesForVariable;
	uint32_t _index = uint32_t(-1);

	AutoBindingDescription& fillBindingHandles(const size_t numBindingHandles, const uint32_t bindingKeys[], const ShaderResourceBindingMap& shaderResourceBindingMap);
};

template<size_t Index, bool IsVariableStringBinding>
struct RemapBindingHandle
{
	template<typename BindingT>
	RemapBindingHandle(AutoBindingDescription& autoBindingDescription, const ShaderResourceBindingMap& shaderResourceBindingMap, const BindingT& binding) {};
};

template<size_t Index>
struct RemapBindingHandle<Index, true>
{
	template<typename BindingT>
	RemapBindingHandle(AutoBindingDescription& autoBindingDescription, const ShaderResourceBindingMap& shaderResourceBindingMap, const BindingT& binding)
	{
		auto& knownBindingHandles = autoBindingDescription._bindingHandlesForVariable;
		auto found = std::find_if(knownBindingHandles.begin(), knownBindingHandles.end(),
			[&binding](const auto& knownBindingHandle) { return knownBindingHandle._variableBindingKey == binding._bindingKey; });

		if (found == knownBindingHandles.end())
		{
			knownBindingHandles.push_back({uint32_t(-1), binding._bindingKey, nullptr});
			found = knownBindingHandles.begin() + knownBindingHandles.size() - 1;
			
			RenderResourceViewBindingHandle& bindingHandle = *found;

			auto foundShaderResource = shaderResourceBindingMap._shaderResourceMap.find(binding._bindingKey);
			if (foundShaderResource != shaderResourceBindingMap._shaderResourceMap.end())
			{
				bindingHandle._shaderResource = &foundShaderResource->second;
				bindingHandle._offset = binding._bindingKey;
			}
		}

		autoBindingDescription._bindingHandles[Index] = *found;
	};
};

AutoBindingDescription& AutoBindingDescription::fillBindingHandles(const size_t numBindingHandles, const uint32_t bindingKeys[], const ShaderResourceBindingMap& shaderResourceBindingMap)
{
	for (uint32_t i = 0; i < numBindingHandles; ++i)
	{
		decltype(shaderResourceBindingMap._shaderResourceMap.end()) foundShaderResource;

		RenderResourceViewBindingHandle& bindingHandle = _bindingHandles[i];
		if (bindingKeys[i] == uint32_t(-1) ||
			(foundShaderResource = shaderResourceBindingMap._shaderResourceMap.find(bindingKeys[i])) == shaderResourceBindingMap._shaderResourceMap.end())
			continue;

		bindingHandle._shaderResource = &foundShaderResource->second;
		bindingHandle._offset = bindingKeys[i]; // Todo
	}

	return *this;
}

static AutoBindingDescription getCurrentAutoBindingDescription(AutoBindingContext& autoBindingContext, const size_t numBindingHandles, const uint32_t bindingKeys[], const ShaderResourceBindingMap& shaderResourceBindingMap)
{
	for (AutoBindingContext::AutoBindingIdentifier& autoBindingIdentifier : autoBindingContext._autoBindingIdentifiers)
	{
		if (autoBindingIdentifier._shaderResourceBindingMap == &shaderResourceBindingMap)
			return AutoBindingDescription{ autoBindingIdentifier._bindingHandles, autoBindingIdentifier._bindingHandlesForVariable, autoBindingIdentifier._index };
	}

	autoBindingContext._autoBindingIdentifiers.push_back({
		&shaderResourceBindingMap,
		std::vector<RenderResourceViewBindingHandle>(numBindingHandles),
		std::vector<RenderResourceViewBindingHandle>{},
		uint32_t(autoBindingContext._autoBindingIdentifiers.size())
		});
	AutoBindingContext::AutoBindingIdentifier& autoBindingIdentifier = autoBindingContext._autoBindingIdentifiers.back();
	return AutoBindingDescription{ autoBindingIdentifier._bindingHandles, autoBindingIdentifier._bindingHandlesForVariable, autoBindingIdentifier._index }.fillBindingHandles(numBindingHandles, bindingKeys, shaderResourceBindingMap);
}

template<typename... BindingTs>
void bindResources(const ShaderResourceBindingMap& shaderResourceBindingMap, BindingTs&&... bindings)
{
	using BindingMetaT = BindingMeta<BindingBlock<BindingTs...>>;
	constexpr size_t NumBindings = BindingMetaT::_count;
	using FlattenTupleT = typename BindingMetaT::FlattenTupleT;

	struct LocalBinder
	{
		uint32_t _bindingKeys[NumBindings];
	};

	static LocalBinder sLocalBinder = [] <std::size_t... TupleIndexPack> (std::index_sequence<TupleIndexPack...>) {
		return LocalBinder{ { BindingMeta<std::tuple_element_t<TupleIndexPack, FlattenTupleT>>::getBindingKey()..., } };
	} (std::make_index_sequence<std::tuple_size_v<FlattenTupleT>>());

	static thread_local AutoBindingContext tlsAutoBindingContext;
	AutoBindingDescription autoBindingDescription = getCurrentAutoBindingDescription(tlsAutoBindingContext, NumBindings, sLocalBinder._bindingKeys, shaderResourceBindingMap);

	if constexpr (BindingMetaT::_isVariableStringBinding)
	{
		[&autoBindingDescription, &shaderResourceBindingMap] <std::size_t... TupleIndexPack, typename...  BindingInnerTs> (LocalBinder& localBinder, std::index_sequence<TupleIndexPack...>, BindingInnerTs&&... bindings) {
			auto internalCall = [&] <std::size_t TupleIndex> () {
				RemapBindingHandle<TupleIndex, BindingMeta<std::tuple_element_t<TupleIndex, FlattenTupleT>>::_isVariableStringBinding>(
					autoBindingDescription, shaderResourceBindingMap, getBinding<TupleIndex>(std::forward<BindingInnerTs>(bindings)...) );
			};
			(internalCall.template operator()<TupleIndexPack>(), ...);
		} (sLocalBinder, std::make_index_sequence<std::tuple_size_v<FlattenTupleT>>(), std::forward<BindingTs>(bindings)...);
	}

	[&autoBindingDescription] <std::size_t... IndexPack, typename... BindingInnerTs> (std::integer_sequence<size_t, IndexPack...>, BindingInnerTs&&... bindings) {
		([&] {
			if constexpr (BindingMeta<BindingInnerTs>::_isBlock)
				bindResourceInternal(autoBindingDescription._bindingHandles.data() + std::get<IndexPack>(BindingMetaT::_offsetsInternal), std::forward<BindingInnerTs>(bindings));
			else
				bindResourceInternal(autoBindingDescription._bindingHandles[std::get<IndexPack>(BindingMetaT::_offsetsInternal)], std::forward<BindingInnerTs>(bindings));
			} (), ...);
	} (std::make_index_sequence<sizeof...(BindingTs)>(), std::forward<BindingTs>(bindings)...);
}


///////////////////////////////////////////////////////////////////////
// test entrypoint

void constexpr_str_test()
{
	ITexture tex1{{1, 2, 1 << 18, 3},  8, 1, 1};
	ITexture tex2{{1, 3, 1 << 19, 4},  9, 1, 3};
	ITexture tex3{{1, 2, 1 << 20, 5}, 10, 4, 1};
	IBuffer buf1{{2, 5,  8 << 20, 0},   256};
	IBuffer buf2{{2, 6,  4 << 20, 0},  4096};
	IBuffer buf3{{2, 7, 24 << 20, 0}, 65536};

	std::vector<ITexture*> textures{ &tex1, &tex2, &tex3 };

	ShaderResourceBindingMap bindingMapA {
		{
			{::getBindingKey("g_texA"), {}},
			{::getBindingKey("g_texB"), {}},
			{::getBindingKey("g_texC"), {}},
		}
	};

	ShaderResourceBindingMap bindingMapB{
		{
			{::getBindingKey("g_bufA"), {}},
			{::getBindingKey("g_bufB"), {}},
			{::getBindingKey("g_bufC"), {}},
		}
	};

	ShaderResourceBindingMap bindingMapC{
		{
			{::getBindingKey("g_texA"), {}},
			{::getBindingKey("g_texB"), {}},
			{::getBindingKey("g_bufC"), {}},
		}
	};

	ShaderResourceBindingMap bindingMapD{
		{
			{::getBindingKey("g_texA"), {}},
			{::getBindingKey("g_bufB"), {}},
			{::getBindingKey("s_tex2"), {}},
		}
	};

	ShaderResourceBindingMap bindingMapE{
		{
			{::getBindingKey("s_tex3"), {}},
			{::getBindingKey("g_bufB"), {}},
			{::getBindingKey("s_tex2"), {}},
		}
	};

	uint32_t count = rand() * rand() % 50;
	printf("count: %d\n", count);
	for (uint32_t i = 0; i < count; ++i)
	{
		bindResources(bindingMapA, // New combination
			Bind<"g_texA">(tex1, 5),
			Bind<"g_texB">(tex2, 4),
			Bind<"g_texC">(tex3, 3));
	}

	bindResources(bindingMapA, // Same as above in both Literals: <"g_texA", "g_texB", "g_texC"> and ResourceT: <T, T, T>
		Bind<"g_texA">(tex3, 0),
		Bind<"g_texB">(tex2, 1),
		Bind<"g_texC">(tex1, 2));


	bindResources(bindingMapA, // New combination in ResourceT: <T, B, B>
		Bind<"g_texA">(tex1),
		Bind<"g_texB">(buf2),
		Bind<"g_texC">(buf3));

	uint32_t testName1 = ::getBindingKey("s_tex1");
	bindResources(bindingMapA, // New combination in both Literals: <"g_texA", "g_texB", "g_texC", null_fixed_string> and ResourceT: <T, B, B, T>
		Bind<"g_texA">(tex1),
		Bind<"g_texB">(buf2),
		Bind<"g_texC">(buf3),
		Bind(testName1, tex1));

	uint32_t testName2 = ::getBindingKey("s_tex2");
	bindResources(bindingMapA, // Same as above but variable string binding name is different.
		Bind<"g_texA">(tex1),
		Bind<"g_texB">(buf2),
		Bind<"g_texC">(buf3),
		Bind(testName2, tex1));

	bindResources(bindingMapA, // New combination Literals: <"g_texA", "g_bufA"> and ResourceT: <T, B>
		Bind<"g_texA">(tex1),
		Bind<"g_bufA">(buf1),
		BindIf(true,
			Bind<"g_bufB">(buf2),
			Bind<"g_bufC">(buf3)
			));

	bindResources(bindingMapA, // Same as above but only difference is condition=false.
		Bind<"g_texA">(tex1),
		Bind<"g_bufA">(buf1),
		BindIf(false,
			Bind<"g_bufB">(buf2),
			Bind<"g_bufC">(buf3)
		));

	// Multiple-time variable string binding and case
	struct VariableBindingPair
	{
		uint32_t _bindingName;
		ITexture* _resource;
	} variableBindingPairs[] = {
		{::getBindingKey("s_tex1"), &tex1},
		{::getBindingKey("s_tex2"), &tex2},
		{::getBindingKey("s_tex3"), &tex1}
	};

	ShaderResourceBindingMap* bindingMaps[3] = {
		&bindingMapD,
		&bindingMapA,
		&bindingMapE,
	};

	for (uint32_t i = 0; i < 3; ++i) // for changing binding map
	{
		for(uint32_t j = 0; j < sizeof(variableBindingPairs) / sizeof(variableBindingPairs[0]); ++j) // for changing variable string binding
			bindResources(*bindingMaps[i], // A single binding combination in both Literals: <null_fixed_string> and ResourceT: <T>
				Bind(variableBindingPairs[j]._bindingName, *variableBindingPairs[j]._resource));
	}

	bindResources(bindingMapA, // Complex case
		Bind<"g_texA">(tex1),
		Bind<"g_bufA">(buf1),
		BindIf(true,
			Bind<"g_bufB">(buf2),
			Bind<"g_bufC">(buf3),
			BindIf(true,
				Bind<"g_bufB">(buf2),
				BindIf(true,
					Bind<"g_bufB">(buf2),
					Bind<"g_bufC">(buf3)
				),
				Bind<"g_bufC">(buf3)
			)
		),
		BindIf(false,
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texC">(tex3)
		));

	bindResources(bindingMapA, // Same as above but condition flipped.
		Bind<"g_texA">(tex1),
		Bind<"g_bufA">(buf1),
		BindIf(false,
			Bind<"g_bufB">(buf2),
			Bind<"g_bufC">(buf3),
			BindIf(true,
				Bind<"g_bufB">(buf2),
				BindIf(true,
					Bind<"g_bufB">(buf2),
					Bind<"g_bufC">(buf3)
				),
				Bind<"g_bufC">(buf3)
			)
		),
		BindIf(true,
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texC">(tex3)
		));

	bindResources(bindingMapA, // New complex by "g_buf[B|C]_____________________________WHAT_THE_HELL"
		Bind<"g_texA">(tex1),
		Bind<"g_bufA">(buf1),
		BindIf(true,
			Bind<"g_bufB">(buf2),
			Bind<"g_bufC">(buf3),
			BindIf(true,
				Bind<"g_bufB">(buf2),
				BindIf(false,
					Bind<"g_bufB_____________________________WHAT_THE_HELL">(buf2),
					Bind<"g_bufC_____________________________WHAT_THE_HELL">(buf3)
				),
				Bind<"g_bufC">(buf3)
			)
		),
		BindIf(true,
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texB">(tex2),
			Bind<"g_texC">(tex3)
		));

	bindResources(bindingMapA, // New combination Literals: <"g_texA", "g_texArray", "g_bufA"> and ResourceT: <T, v<T>, B>
		Bind<"g_texA">(tex1),
		Bind<"g_texArray">(textures),
		Bind<"g_bufA">(buf1));
}

#endif