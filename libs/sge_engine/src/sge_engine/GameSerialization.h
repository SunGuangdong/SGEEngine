#pragma once

#include "sge_core/typelib/typeLib.h"
#include "sge_engine_api.h"
#include <string>

namespace sge {

class IReadStream;
struct JsonValue;
struct JsonValueBuffer;

struct GameWorld;
struct ObjectId;
struct GameObject;

struct TypeDesc;

SGE_ENGINE_API JsonValue* serializeGameWorld(const GameWorld* world, JsonValueBuffer& jvb);
SGE_ENGINE_API std::string serializeGameWorld(const GameWorld* world);

SGE_ENGINE_API bool loadGameWorldFromStream(GameWorld* world, IReadStream* stream, const char* const workingFilename = "");
SGE_ENGINE_API bool loadGameWorldFromString(GameWorld* world, const char* const levelJson, const char* const workingFilename = "");
SGE_ENGINE_API bool loadGameWorldFromFile(GameWorld* world, const char* const filename);

SGE_ENGINE_API JsonValue* serializeObject(const GameObject* object, JsonValueBuffer& jvb);
SGE_ENGINE_API std::string serializeObject(const GameObject* object);

/// Deserializes the specified game object to json.
/// @param [in/out] world is the world that is going to contain the newly created object.
/// @param [in] json the serialized json string that describes the object.
/// @param [in] shouldGenerateNewId, if true the game object will get a new id assigned by the containing world.
///             if false, the world will try to retain the id of the object that is stored in it's json data.
///             Specifing false is used with command histroy for deleting and undoing deletion of game objects.
///             Usually true should be specified.
/// @param [out] outOriginalId the id of the object specified in the json string.
/// @retval a point to the created game object.
SGE_ENGINE_API GameObject*
    deserializeObjectFromJson(GameWorld* const world, const std::string& json, const bool shouldGenerateNewId, ObjectId* outOriginalId);


SGE_ENGINE_API JsonValue* serializeVariable(const TypeDesc* const typeDesc, const char* const data, JsonValueBuffer& jvb);
SGE_ENGINE_API bool deserializeVariable(char* const valueData, const JsonValue* jValue, const TypeDesc* const typeDesc);

template <typename T>
JsonValue* serializeVariableT(const T& value, JsonValueBuffer& jvb) {
	return serializeVariable(typeLib().find(sgeTypeId(T)), (char*)&value, jvb);
}
} // namespace sge
