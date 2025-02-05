#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_utils/TypeTraits.h"
#include "sge_utils/containers/vector_map.h"
#include "sge_utils/hash/hash_combine.h"
#include <cstring>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

namespace sge {

struct MemberChain;

struct TypeId {
#if 0 // [SGE_AUTOMATIC_TYPE_ID_NON_COMPILER_CONSISTENT]
	int id;

	explicit TypeId(int const id = 0)
	    : id(id) {
	}

	bool isNull() const {
		return id == 0;
	}

	bool isValid() const {
		return id != 0;
	}

	bool operator<(const TypeId& r) const {
		return id < r.id;
	}
	bool operator>(const TypeId& r) const {
		return id > r.id;
	}
	bool operator==(const TypeId& r) const {
		return id == r.id;
	}
	bool operator!=(const TypeId& r) const {
		return id != r.id;
	}
#else
	int hash = 0;

	/// Not allocated, it is assumed that this is a variable from .TEXT section.
	/// Originally this was a std::string.
	const char* name = nullptr;

	TypeId() = default;
	explicit TypeId(const char* const cstr)
	{
		if (cstr) {
			hash = hashCString_djb2(cstr);
			name = cstr;
		}
	}

	bool isNull() const { return hash == 0 && name == nullptr; }

	bool isValid() const { return !isNull(); }

	bool operator<(const TypeId& r) const { return hash < r.hash; }

	bool operator>(const TypeId& r) const { return hash > r.hash; }

	bool operator==(const TypeId& r) const { return hash == r.hash && strcmp(name, r.name) == 0; }

	bool operator!=(const TypeId& r) const { return hash != r.hash || strcmp(name, r.name) != 0; }
#endif
};

} // namespace sge

/// Definition of std::hash<TypeId>
namespace std {
#if 0 // [SGE_AUTOMATIC_TYPE_ID_NON_COMPILER_CONSISTENT]
template <>
struct hash<sge::TypeId> {
	int operator()(const sge::TypeId& k) const {
		return k.id;
	}
};
#else
template <>
struct hash<sge::TypeId> {
	int operator()(const sge::TypeId& k) const { return k.hash; }
};
#endif
} // namespace std


namespace sge {

/// A funtion providing the type-id of each type.
/// A function specialization for everytype that participates in the reflection
/// must be provided. If you get linker errors it is because this function isn't
/// defined for a type that is used in the reflection system.
/// 0 is reserved for invalid id.
///
/// A good nomenclature for assigning ids is:
/// yy'mm'dd'nnnn where nnnn is the number of type registered on this day.
#if 0 // [SGE_AUTOMATIC_TYPE_ID_NON_COMPILER_CONSISTENT]
template <typename T>
TypeId sgeTypeIdFn();
#else
template <typename T>
TypeId sgeTypeIdFn()
{
	#ifdef WIN32
	return TypeId(__FUNCSIG__); // TODO: make this cross-compiler safe.
	#else
	return TypeId(__FUNCSIG__);
	#endif
}
#endif

/// Don't use in header definition it will bloat the cpps unless you need
/// to use id same id in different libraries (dlls/so and so on) where
/// the function specialization isn't acessible.
#if 0 // [SGE_AUTOMATIC_TYPE_ID_NON_COMPILER_CONSISTENT]
	#define ReflAddTypeIdInline(T, _id)     \
		template <>                         \
		inline TypeId sge::sgeTypeIdFn<T>() \
		{                                   \
			return TypeId(_id);             \
		}

	#ifdef SGE_CORE_BUILDING_DLL
		#define ReflAddTypeId(T, _id)            \
			template <>                          \
			SGE_CORE_API TypeId sgeTypeIdFn<T>() \
			{                                    \
				return TypeId(_id);              \
			}
	#else
		#define ReflAddTypeId(T, _id)            \
			template <>                          \
			SGE_CORE_API TypeId sgeTypeIdFn<T>() \
			{                                    \
				return TypeId(_id);              \
			}
	#endif
#else
	#define ReflAddTypeIdInline(T, _id)
	#define ReflAddTypeId(T, _id)
#endif

// Mark that the type id already exists, this is used for situations were we
#if 0 // [SGE_AUTOMATIC_TYPE_ID_NON_COMPILER_CONSISTENT]
	#ifdef SGE_CORE_BUILDING_DLL
		#define ReflAddTypeIdExists(T) \
			struct T;                  \
			template <>                \
			SGE_CORE_API TypeId sgeTypeIdFn<T>();
	#else
		#define ReflAddTypeIdExists(T) \
			struct T;                  \
			template <>                \
			TypeId sgeTypeIdFn<T>();
	#endif
#else
	#define ReflAddTypeIdExists(T)
#endif

/// A macros used to quckly obtain id for a particular type.
#define sgeTypeId(T) sgeTypeIdFn<T>()

} // namespace sge

template <typename Test, template <typename...> class Ref>
struct is_specialization : std::false_type {
};

template <template <typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref> : std::true_type {
};

template <typename T, typename M>
constexpr int sge_offsetof(M T::*member)
{
	return (int)((char*)&((T*)nullptr->*member) - (char*)nullptr);
}

/// Find the byte offset of the @Base when derived in @Derived.
template <typename Base, typename Derived>
constexpr int sge_offsetof_inheritance()
{
	const ptrdiff_t fundamentalAlign = sizeof(void*);
	const ptrdiff_t r = ((ptrdiff_t)(Base*)(Derived*)fundamentalAlign) - fundamentalAlign;
	return int(r);
}

namespace sge {

struct TypeDesc;

/// A set of additional flags that could be applied on struct members.
enum MemberFieldFlags : unsigned {
	MFF_NonEditable = 1 << 0,
	MFF_NonSaveable = 1 << 1,
	MFF_FloatAsDegrees = 1 << 2,
	MFF_FloatAsPercentage = 1 << 3,
	MFF_Vec3fAsColor = 1 << 4,
	MFF_Vec4fAsColor = 1 << 5,
	MFF_PrefabDontCopy = 1 << 6,
};

/// A structure describing a single member (property) of a class:
/// its byte offset, name, type etc.
struct SGE_CORE_API MemberDesc {
	MemberDesc()
	{
		min_int = 0;
		max_int = 0;
	}

	const TypeDesc* owner = nullptr;
	const char* name = nullptr;
	std::string prettyName;
	TypeId typeId;
	int byteOffset = 0;
	int sizeBytes = 0; // CAUTION: This is a bit redundant but it simplfies the code a bit. This value must be the same
	                   // that is stored in the type register for that type!.
	TypeId inheritedForm;
	unsigned int flags = 0;

	bool isEditable() const { return (flags & MFF_NonEditable) == 0; }

	bool isSaveable() const { return (flags & MFF_NonSaveable) == 0; }

	void (*getDataFn)(void* object, void* dest) = nullptr;
	void (*setDataFn)(void* object, const void* src) = nullptr;

	float sliderSpeed_float = 1.f;

	union {
		int min_int;
		float min_float;
	};

	union {
		int max_int;
		float max_float;
	};

	/// Checks if the current member is the one passed as the function argument.
	/// The function will work only if the very 1st type is used for @T (basically the base class that introduced it)
	/// Or if the same type of the owner of this member is used. No types in between.
	template <typename T, typename M>
	bool is(M T::*memberPtr) const;
};

} // namespace sge

extern template class std::vector<sge::MemberDesc>;

namespace sge {

/// TypeDesc hold the description of an entier type,
/// is this type an enum or struct, is it a std::vector<T>
/// what members does it have, who are the base classes, what is the name of type (as a string).
struct SGE_CORE_API TypeDesc {
	static std::string computePrettyName(const char* const name);

	template <typename T>
	static TypeDesc create(const char* const name)
	{
		TypeDesc td;

		td.name = name;
		td.typeId = sgeTypeId(T);
		td.sizeBytes = sizeof(T);

		return td;
	}

	template <typename T>
	TypeDesc& constructable()
	{
		sgeAssert(sgeTypeId(T) == typeId);

		constructorFn = [](void* dest) { new ((T*)dest) T(); };
		destructorFn = [](void* dest) { ((T*)dest)->~T(); };

		newFn = []() -> void* { return new T(); };
		deleteFn = [](void* ptr) -> void {
			// Delete should do the nullptr check itself.
			delete (reinterpret_cast<T*>(ptr));
		};

		return *this;
	}

	template <typename T>
	TypeDesc& copyable()
	{
		sgeAssert(sgeTypeId(T) == typeId);
		copyFn = [](void* dest, const void* src) { *(T*)(dest) = *(T*)(src); };

		return *this;
	}

	template <typename T>
	TypeDesc& compareable()
	{
		sgeAssert(sgeTypeId(T) == typeId);
		equalsFn = [](const void* a, const void* b) -> bool { return *(T*)(a) == *(T*)(b); };
		return *this;
	}

	/// Indicates that the represented type is an enum.
	template <typename T>
	TypeDesc& thisIsEnum()
	{
		enumUnderlayingType = sgeTypeId(typename std::underlying_type<T>::type);
		return *this;
	}

	/// Indicates that the represented type is std::vector<T>.
	template <typename T>
	TypeDesc& thisIsStdVector()
	{
		stdVectorUnderlayingType = sgeTypeId(typename T::value_type);

		stdVectorSize = [](const void* vector) -> size_t { return (*(T*)(vector)).size(); };

		stdVectorEraseAtIndex = [](void* vector, size_t index) -> void {
			auto& v = *(T*)(vector);
			v.erase(v.begin() + index);
		};

		stdVectorEmplaceBack = [](void* vector, void* elemData) -> void {
			auto& v = *(T*)(vector);
			typename T::value_type& elemDataTyped = *(typename T::value_type*)elemData;
			v.emplace_back(elemDataTyped);
		};

		stdVectorResize = [](void* vector, size_t size) -> void { (*(T*)(vector)).resize(size); };

		stdVectorGetElement = [](void* vector, size_t index) -> void* {
			auto& v = *(T*)(vector);
			return &v.data()[index];
		};

		stdVectorGetElementConst = [](const void* vector, size_t index) -> const void* {
			auto& v = *(T*)(vector);
			return &v.data()[index];
		};

		return *this;
	}

	/// Indicates that the represented type is std::map<T>.
	template <typename T>
	TypeDesc& thisIsStdMap()
	{
		stdMapKeyType = sgeTypeId(typename T::key_type);
		stdMapValueType = sgeTypeId(typename T::mapped_type);

		stdMapSize = [](void* umap) -> size_t { return (*(T*)(umap)).size(); };

		stdMapGetNthPair = [](void* umap, size_t idx, void* outKey, void* outValue) -> void {
			typename T::iterator itr = (*(T*)(umap)).begin();
			while (idx > 0) {
				++itr;
				--idx;
			}

			if (outKey != nullptr) {
				typename T::key_type& keyRef = *(typename T::key_type*)(outKey);
				keyRef = itr->first;
			}

			if (outValue != nullptr) {
				typename T::mapped_type& valueRef = *(typename T::mapped_type*)(outValue);
				valueRef = itr->second;
			}
		};

		stdMapGetPointerToValueByKey = [](void* umapPtr, const void* key, void* outValue) -> void {
			T& umap = (*(T*)(umapPtr));
			const typename T::key_type& keyRef = *(const typename T::key_type*)(key);
			outValue = &umap[keyRef];
		};

		stdMapInsert = [](void* umapPtr, const void* key, const void* value) -> void {
			T& umap = (*(T*)(umapPtr));
			const typename T::key_type& keyRef = *(const typename T::key_type*)(key);
			typename T::mapped_type& valueRef = *(typename T::mapped_type*)(value);
			umap[keyRef] = valueRef;
		};

		return *this;
	}

	// This function depends on @typelib() which is defined just below this class.
	// That's we we are defining this function below as it doesn't compile with GCC>
	template <typename T, typename TParent>
	TypeDesc& inherits();

	/// Registers a new member (property) to the struct represented by this type.
	template <typename T, typename M>
	TypeDesc& member(const char* const name, M T::*memberPtr, unsigned const flags = 0)
	{
		sgeAssert(sgeTypeId(T) == typeId);

		MemberDesc md;

		md.owner = this;
		md.name = name;
		md.prettyName = computePrettyName(name);
		md.typeId = sgeTypeId(M);
		md.byteOffset = sge_offsetof(memberPtr);
		md.sizeBytes = sizeof(M);
		md.flags = flags;

		members.push_back(md);
		return *this;
	}

	// Registers a new member to the struct.
	template <typename T, typename M>
	TypeDesc& member2base(
	    const char* const name,
	    void (*getDataFn)(void* object, void* dest),
	    void (*setDataFn)(void* object, const void* src),
	    unsigned const flags)
	{
		sgeAssert(sgeTypeId(T) == typeId);

		MemberDesc mfd;
		mfd.owner = this;
		mfd.name = name;
		mfd.typeId = sgeTypeId(M);
		// mfd.selfType = this->find(typeId);
		mfd.byteOffset = -1;
		mfd.getDataFn = getDataFn;
		mfd.setDataFn = setDataFn;
		mfd.sizeBytes = sizeof(M);
		mfd.flags = flags;

		members.push_back(mfd);
		return *this;
	}

	template <class T, class M, M (T::*getter)() const, void (T::*setter)(const M)>
	TypeDesc& member2(const char* const name, unsigned const flags = 0)
	{
		auto getDataFn = [](void* object, void* dest) -> void { *((M*)(dest)) = (((T*)(object))->*getter)(); };
		auto setDataFn = [](void* object, const void* dest) -> void { (((T*)(object))->*setter)(*((M*)(dest))); };

		return member2base<T, M>(name, getDataFn, setDataFn, flags);
	}

	template <typename T, typename M, const M& (T::*getter)() const, void (T::*setter)(const M&)>
	TypeDesc& member2(const char* const name, unsigned const flags = 0)
	{
		auto getDataFn = [](void* object, void* dest) -> void { *((M*)(dest)) = (((T*)(object))->*getter)(); };
		auto setDataFn = [](void* object, const void* dest) -> void { (((T*)(object))->*setter)(*((M*)(dest))); };

		return member2base<T, M>(name, getDataFn, setDataFn, flags);
	}

	/// Adds a flag to the last added member to the struct.
	/// This is used as "syntax" sugar, when typing reflection code.
	TypeDesc& addMemberFlag(int flag)
	{
		members.back().flags |= flag;
		return *this;
	}

	/// If applicable controls the UI range of the values in case the type is a float.
	TypeDesc& uiRange(float vmin, float vmax, float sliderSpeed);

	/// If applicable controls the UI range of the value in case the type is an int.
	TypeDesc& uiRange(int vmin, int vmax, float sliderSpeed);

	/// Specifies a string to be used in the UI when referencing this member.
	/// If not called manually, an auto generated version will be used.
	TypeDesc& setPrettyName(const char* prettyName)
	{
		if (prettyName) {
			members.back().prettyName = prettyName;
		}
		return *this;
	}

	/// Registers an enum value associated to with this type.
	TypeDesc& addEnumMember(int member, const char* name);

	/// Searches for the specified member property in the described type.
	template <typename T, typename M>
	const MemberDesc* findMember(M T::*memberPtr) const
	{
		int const byteOffset = sge_offsetof(memberPtr);
		for (int t = 0; t < members.size(); ++t) {
			if (members[t].byteOffset == byteOffset) {
				if (members[t].typeId != sgeTypeId(M)) {
					sgeAssert(false);
					return nullptr;
				}
				return &members[t];
			}
		}

		// Since this is the reflection system, usually we know that the member exists so
		// assert here to notify the user that he probably did something wrong.
		sgeAssert(false && "The member was not found");
		return nullptr;
	}

	/// Searches for a member by its name.
	const MemberDesc* findMemberByName(const char* const memberName) const;

	bool doesInherits(const TypeId parentClass) const;

  public:
	const char* name = nullptr;
	TypeId typeId;
	int sizeBytes = 0;

	TypeId
	    enumUnderlayingType; ///< The type that is used to represent an enum. If the type is not enum this is nullptr.
	TypeId stdVectorUnderlayingType; ///< Set if the type is std::vector. Unset otherwise.

	TypeId stdMapKeyType;
	TypeId stdMapValueType;

	std::vector<MemberDesc> members;

	struct SuperClassData {
		TypeId id;
		int byteOffset = 0;
	};

	/// A list of all inherited types.
	std::vector<SuperClassData> superclasses;

	void (*copyFn)(void* dest, const void* src) = nullptr;
	bool (*equalsFn)(const void* a, const void* b) = nullptr;
	void (*constructorFn)(void* dest) = nullptr;
	void (*destructorFn)(void* dest) = nullptr;
	void* (*newFn)() = nullptr;
	void (*deleteFn)(void* ptr) = nullptr;

	/// std::vector specific functions:
	size_t (*stdVectorSize)(const void* vector) = nullptr;
	void (*stdVectorEraseAtIndex)(void* vector, size_t index) = nullptr;
	void (*stdVectorEmplaceBack)(void* vector, void* data) = nullptr;
	void (*stdVectorResize)(void* vector, size_t size) = nullptr;
	void* (*stdVectorGetElement)(void* vector, size_t index) = nullptr;
	const void* (*stdVectorGetElementConst)(const void* vector, size_t index) = nullptr;

	/// std::map specifc functions:
	size_t (*stdMapSize)(void* umap) = nullptr;
	void (*stdMapGetNthPair)(void* umap, size_t idx, void* outKey, void* outValue) = nullptr;
	void (*stdMapGetPointerToValueByKey)(void* umap, const void* key, void* outValue) = nullptr;
	void (*stdMapInsert)(void* umap, const void* key, const void* value) = nullptr;

	/// Converts an enum value to string.
	vector_map<int, const char*> enumValueToNameLUT;
};

} // namespace sge

extern template class std::map<sge::TypeId, sge::TypeDesc>;
extern template class std::map<sge::TypeId, sge::TypeDesc*>;

namespace sge {

/// TypeLib is the class that holds the entier program reflection data in it.
/// The instance of it @typeLib() should be used.
struct SGE_CORE_API TypeLib {
	using MapTypes = std::map<TypeId, TypeDesc>;

	template <typename T>
	TypeDesc& addType(const char* const name)
	{
#if SGE_USE_DEBUG
		MapTypes::iterator itr = m_registeredTypes.find(sgeTypeId(T));
		if (itr != std::end(m_registeredTypes)) {
			// Type is already registered this may not be intended.
			// sgeAssert(false);

			// NOTE: Defining type again isn't that harmful as it is usually going to be defined
			// in the same way, some testing needs to be done in order to figure out how to apploach this.
		}
#endif

		TypeDesc& retval = m_registeredTypes[sgeTypeId(T)];
		retval = TypeDesc::create<T>(name);
		isCompleted[sgeTypeId(T)] = false;

		// FInd the traits of that type.
		if constexpr (std::is_enum<T>::value) {
			retval.thisIsEnum<T>();
		}

		if constexpr (is_specialization<T, std::vector>::value) {
			retval.thisIsStdVector<T>();
		}

		if constexpr (is_specialization<T, std::map>::value) {
			retval.thisIsStdMap<T>();
		}

		if constexpr (std::is_default_constructible<T>::value) {
			retval.constructable<T>();
		}

		if constexpr (std::is_copy_assignable<T>::value) {
			retval.copyable<T>();
		}

		if constexpr (is_compareable<T>::value) {
			retval.compareable<T>();
		}

		return retval;
	}

	TypeDesc* find(TypeId const typeId)
	{
		MapTypes::iterator itr = m_registeredTypes.find(typeId);
		if (itr == std::end(m_registeredTypes)) {
			return nullptr;
		}

		return &itr->second;
	}

	const TypeDesc* find(TypeId const typeId) const
	{
		MapTypes::const_iterator itr = m_registeredTypes.find(typeId);
		if (itr == std::end(m_registeredTypes)) {
			return nullptr;
		}

		return &itr->second;
	}

	/// Searches for the reflection data of type @T
	template <typename T>
	const TypeDesc* find() const
	{
		return find(sgeTypeId(T));
	}

	TypeDesc* findByName(const char* const name)
	{
		for (MapTypes::iterator itr = m_registeredTypes.begin(); itr != m_registeredTypes.end(); ++itr) {
			if (strcmp(itr->second.name, name) == 0) {
				return &itr->second;
			}
		}

		return nullptr;
	}

	/// Searches for a member of the specified type.
	/// A bit shorter way of calling this is by using the @sgeFindMember macro.
	template <typename T, typename M>
	const MemberDesc* findMember(M T::*memberPtr) const
	{
		const TypeDesc* const typeDesc = find<T>();
		if (typeDesc) {
			return typeDesc->findMember(memberPtr);
		}

		sgeAssert(false);
		return nullptr;
	}

	void performRegistration();

	MapTypes m_registeredTypes;

	std::map<TypeId, bool> isCompleted;
	std::vector<void (*)()> functionsToBeCalledThatWillRegisterTypes;
};

SGE_CORE_API int addFunctionThatDefinesTypesToTypeLibrary(void (*fnPtr)());

/// typeLib() is a function used to access the global type register for the current binary.
SGE_CORE_API TypeLib& typeLib();

template <typename T, typename TParent>
inline TypeDesc& TypeDesc::inherits()
{
	sgeAssert(sgeTypeId(T) == typeId);
	static_assert(std::is_base_of<TParent, T>::value, "T must inherit TParent");

	int const byteOffset = sge_offsetof_inheritance<TParent, T>();
	TypeId const superclassId = sgeTypeId(TParent);
	superclasses.push_back({superclassId, byteOffset});

	return *this;
}

///-------------------------------------------------------------------------------------------
/// A set of helper macros, used to create type definitions (TypeDesc).
///-------------------------------------------------------------------------------------------

/// Starts a definition of a type.
#define ReflAddType(T) typeLib().addType<T>(#T)
#define ReflAddTypeWithName(T, name) typeLib().addType<T>(name)

#define ReflInherits(T, TParent) .inherits<T, TParent>()

/// Adds a reflection describing a member-variable
#define ReflMemberNamed(TStruct, TMember, TName) .member(TName, &TStruct::TMember)
#define ReflMember(TStruct, TMember) .member(#TMember, &TStruct::TMember)

#define ReflEnumVal(v, name) .addEnumMember(int(v), name)

///-------------------------------------------------------------------------------------------
/// A set of macros that enable the user to declare reflection inside a header
/// or a cpp file. The registration will happen when
/// TypeLibrary::performRegistration() is called.
///-------------------------------------------------------------------------------------------
#define ReflBlock() ReflRegisterBlock_Impl(SGE_ANONYMOUS(_SGE_REFL_ANON__FUNC))

#define ReflRegisterBlock_Impl(fnName)                                                                    \
	static void fnName();                                                                                 \
	static int SGE_ANONYMOUS(fnNameRegisterVariable) = addFunctionThatDefinesTypesToTypeLibrary(&fnName); \
	static void fnName()

///-------------------------------------------------------------------------------------------
/// Other helper macros
///-------------------------------------------------------------------------------------------

/// A macro that searches for a member in the specified type.
/// This is just to reduce tying.
#define sgeFindMember(Type, Member) typeLib().find<Type>()->findMember(&Type::Member)

// The function will work only if the very 1st type is used for @T (basically the root of the hierarchy)
// Or if the same type of the owner of this member is used. No types in between.
template <typename T, typename M>
bool MemberDesc::is(M T::*memberPtr) const
{
	const MemberDesc* const mfdRef = typeLib().findMember(memberPtr);

	const bool doesByteOffsetMatch = byteOffset == mfdRef->byteOffset;
	const bool doesTypesMatch = typeId == mfdRef->typeId;
	const bool doesOwnerTypeMatch = owner->typeId == sgeTypeId(T);
	const bool doesInheridTypeMatch = inheritedForm == sgeTypeId(T);

	return doesByteOffsetMatch && doesTypesMatch && (doesOwnerTypeMatch || doesInheridTypeMatch);
}

} // namespace sge
