#include "nrpch.h"
#include "ScriptGlue.h"

#include "ScriptEngine.h"

#include "NotRed/Core/UUID.h"
#include "NotRed/Core/KeyCodes.h"
#include "NotRed/Core/Input.h"

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"

#include "mono/metadata/object.h"
#include "mono/metadata/reflection.h"

#include "box2d/b2_body.h"

namespace NR
{
    static std::unordered_map<MonoType*, std::function<bool(Entity)>> sEntityHasComponentFuncs;

#define NR_ADD_INTERNAL_CALL(Name) mono_add_internal_call("NotRed.InternalCalls::" #Name, Name)

    static void NativeLog(MonoString* string, int parameter)
    {
        char* cStr = mono_string_to_utf8(string);
        std::string str(cStr);
        mono_free(cStr);
        std::cout << str << ", " << parameter << std::endl;
    }

	static bool Entity_HasComponent(UUID entityID, MonoReflectionType* componentType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		NR_CORE_ASSERT(scene, "Scene is null!");
		Entity entity = scene->GetEntity(entityID);
		NR_CORE_ASSERT(entity, "Entity does not exist!");

		MonoType* managedType = mono_reflection_type_get_type(componentType);
		NR_CORE_ASSERT(sEntityHasComponentFuncs.find(managedType) != sEntityHasComponentFuncs.end(), "Function of entity not loaded!");
		return sEntityHasComponentFuncs.at(managedType)(entity);
	}

	static void TransformComponent_GetTranslation(UUID entityID, glm::vec3* outTranslation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		NR_CORE_ASSERT(scene, "Scene is null!");
		Entity entity = scene->GetEntity(entityID);
		NR_CORE_ASSERT(entity, "Entity does not exist!");

		*outTranslation = entity.GetComponent<TransformComponent>().Translation;
	}

	static void TransformComponent_SetTranslation(UUID entityID, glm::vec3* translation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		NR_CORE_ASSERT(scene, "Scene is null!");
		Entity entity = scene->GetEntity(entityID);
		NR_CORE_ASSERT(entity, "Entity does not exist!");

		entity.GetComponent<TransformComponent>().Translation = *translation;
	}

	static void Rigidbody2DComponent_ApplyImpulse(UUID entityID, glm::vec2* impulse, glm::vec2* point, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		NR_CORE_ASSERT(scene, "Scene is null!");
		Entity entity = scene->GetEntity(entityID);
		NR_CORE_ASSERT(entity, "Entity does not exist!");

		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		body->ApplyLinearImpulse({ impulse->x, impulse->y }, { point->x, point->y }, wake);
	}

	static void Rigidbody2DComponent_ApplyImpulseToCenter(UUID entityID, glm::vec2* impulse, bool wake)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		NR_CORE_ASSERT(scene, "Scene is null!");
		Entity entity = scene->GetEntity(entityID);
		NR_CORE_ASSERT(entity, "Entity does not exist!");

		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		body->ApplyLinearImpulseToCenter({ impulse->x, impulse->y }, wake);
	}

	static void Rigidbody2DComponent_GetVelocity(UUID entityID, glm::vec2* outVelocity)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		NR_CORE_ASSERT(scene, "Theres no scene loaded!");
		Entity entity = scene->GetEntity(entityID);
		NR_CORE_ASSERT(entity, "Entity does not exist!");

		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		const b2Vec2& velocity = body->GetLinearVelocity();
		*outVelocity = glm::vec2(velocity.x, velocity.y);
	}

	static Rigidbody2DComponent::BodyType Rigidbody2DComponent_GetType(UUID entityID)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		NR_CORE_ASSERT(scene, "Theres no scene loaded!");
		Entity entity = scene->GetEntity(entityID);
		NR_CORE_ASSERT(entity, "Entity does not exist!");

		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		return (Rigidbody2DComponent::BodyType)body->GetType();
	}

	static void Rigidbody2DComponent_SetType(UUID entityID, Rigidbody2DComponent::BodyType bodyType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		NR_CORE_ASSERT(scene, "Theres no scene loaded!");
		Entity entity = scene->GetEntity(entityID);
		NR_CORE_ASSERT(entity, "Entity does not exist!");

		auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
		b2Body* body = (b2Body*)rb2d.RuntimeBody;
		body->SetType((b2BodyType)bodyType);
	}

	static bool Input_IsKeyDown(KeyCode keycode)
	{
		return Input::IsKeyPressed(keycode);
	}

    template<typename... Component>
    static void RegisterComponent()
    {
        ([]()
            {
                std::string_view typeName = typeid(Component).name();
                size_t pos = typeName.find_last_of(':');
                std::string_view structName = typeName.substr(pos + 1);
                std::string managedTypename = fmt::format("NotRed.{}", structName);

                MonoType* managedType = mono_reflection_type_from_name(managedTypename.data(), ScriptEngine::GetCoreAssemblyImage());
                if (!managedType)
                {
                    NR_CORE_WARN("Could not find component type {}!", managedTypename);
                    return;
                }
                sEntityHasComponentFuncs[managedType] = [](Entity entity) { return entity.HasComponent<Component>(); };
            }(), ...);
    }

    template<typename... Component>
    static void RegisterComponent(ComponentGroup<Component...>)
    {
        RegisterComponent<Component...>();
    }

    void ScriptGlue::RegisterComponents()
    {
		sEntityHasComponentFuncs.clear();
        RegisterComponent(AllComponents{});
    }

    void ScriptGlue::RegisterFunctions()
    {
        NR_ADD_INTERNAL_CALL(NativeLog);

        NR_ADD_INTERNAL_CALL(Entity_HasComponent);
        NR_ADD_INTERNAL_CALL(TransformComponent_GetTranslation);
        NR_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);

        NR_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyImpulse);
        NR_ADD_INTERNAL_CALL(Rigidbody2DComponent_ApplyImpulseToCenter);

		NR_ADD_INTERNAL_CALL(Rigidbody2DComponent_GetVelocity);
		NR_ADD_INTERNAL_CALL(Rigidbody2DComponent_GetType);
		NR_ADD_INTERNAL_CALL(Rigidbody2DComponent_SetType);

        NR_ADD_INTERNAL_CALL(Input_IsKeyDown);
    }
}