﻿using System;
using System.Runtime.CompilerServices;

namespace NR
{
    public class Entity
    {
        public ulong ID { get; private set; }
        public string Tag => GetComponent<TagComponent>().Tag;

        private Action<Entity> _collision2DBeginCallbacks;
        private Action<Entity> _collision2DEndCallbacks;

        private Action<Entity> _collisionBeginCallbacks;
        private Action<Entity> _collisionEndCallbacks;

        private Action<Entity> _triggerBeginCallbacks;
        private Action<Entity> _triggerEndCallbacks;

        protected Entity() { ID = 0; }

        internal Entity(ulong id)
        {
            ID = id;
        }

        ~Entity()
        {

        }

        public ulong GetID()
        {
            return ID;
        }

        public Entity Create()
        {
            return new Entity(CreateEntity_Native(ID));
        }

        public Entity Instantiate(Prefab prefab)
        {
            ulong entityID = Instantiate_Native(ID, prefab.ID);
            if (entityID == 0)
            {
                return null;
            }

            return new Entity(entityID);
        }

        public Entity Instantiate(Prefab prefab, Vector3 translation)
        {
            ulong entityID = InstantiateWithTranslation_Native(ID, prefab.ID, ref translation);
            if (entityID == 0)
            {
                return null;
            }

            return new Entity(entityID);
        }

        public void Destroy()
        {
            DestroyEntity_Native(ID);
        }

        public void Destroy(Entity entity)
        {
            DestroyEntity_Native(entity.ID);
        }

        public Vector3 Translation
        {
            get
            {
                return GetComponent<TransformComponent>().Translation;
            }
            set
            {
                GetComponent<TransformComponent>().Translation = value;
            }
        }

        public Vector3 Rotation
        {
            get
            {
                return GetComponent<TransformComponent>().Rotation;
            }
            set
            {
                GetComponent<TransformComponent>().Rotation = value;
            }
        }

        public Vector3 Scale
        {
            get
            {
                return GetComponent<TransformComponent>().Scale;
            }
            set
            {
                GetComponent<TransformComponent>().Scale = value;
            }
        }

        public Entity Parent
        {
            get => new Entity(GetParent_Native(ID));
            set => SetParent_Native(ID, value.ID);
        }

        public Entity[] Children
        {
            get => GetChildren_Native(ID);
        }

        public T CreateComponent<T>() where T : Component, new()
        {
            CreateComponent_Native(ID, typeof(T));
            T component = new T();
            component.Entity = this;
            return component;
        }

        public bool HasComponent<T>() where T : Component, new()
        {
            return HasComponent_Native(ID, typeof(T));
        }

        public bool HasComponent(Type type)
        {
            return HasComponent_Native(ID, type);
        }

        public T GetComponent<T>() where T : Component, new()
        {
            if (HasComponent<T>())
            {
                T component = new T();
                component.Entity = this;
                return component;
            }
            return null;
        }

        public Entity FindEntityByTag(string tag)
        {
            ulong entityID = FindEntityByTag_Native(tag);
            
            if (entityID == 0)
            {
                return null;
            }

            return new Entity(entityID);
        }

        public Entity Instantiate()
        {
            ulong entityID = InstantiateEntity_Native();
            return new Entity(entityID);
        }

        public Entity FindEntityByID(ulong entityID)
        {
            return new Entity(entityID);
        }

        public void AddCollision2DBeginCallback(Action<Entity> callback)
        {
            _collision2DBeginCallbacks += callback;
        }

        public void AddCollision2DEndCallback(Action<Entity> callback)
        {
            _collision2DEndCallbacks += callback;
        }

        public void AddCollisionBeginCallback(Action<Entity> callback)
        {
            _collisionBeginCallbacks += callback;
        }

        public void AddCollisionEndCallback(Action<Entity> callback)
        {
            _collisionEndCallbacks += callback;
        }

        public void AddTriggerBeginCallback(Action<Entity> callback)
        {
            _triggerBeginCallbacks += callback;
        }

        public void AddTriggerEndCallback(Action<Entity> callback)
        {
            _triggerEndCallbacks += callback;
        }

        private void CollisionBegin(ulong id)
        {
            if (_collisionBeginCallbacks != null)
            {
                _collisionBeginCallbacks.Invoke(new Entity(id));
            }
        }

        private void CollisionEnd(ulong id)
        {
            if (_collisionEndCallbacks != null)
            {
                _collisionEndCallbacks.Invoke(new Entity(id));
            }
        }

        private void Collision2DBegin(ulong id)
        {
            _collision2DBeginCallbacks?.Invoke(new Entity(id));
        }

        private void Collision2DEnd(ulong id)
        {
            _collision2DEndCallbacks?.Invoke(new Entity(id));
        }

        private void TriggerBegin(ulong id)
        {
            if (_triggerBeginCallbacks != null)
            { 
                _triggerBeginCallbacks.Invoke(new Entity(id)); 
            }
        }

        private void TriggerEnd(ulong id)
        {
            if (_triggerEndCallbacks != null)
            {
                _triggerEndCallbacks.Invoke(new Entity(id));
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern ulong GetParent_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern ulong SetParent_Native(ulong entityID, ulong parentID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern Entity[] GetChildren_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void CreateComponent_Native(ulong entityID, Type type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool HasComponent_Native(ulong entityID, Type type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong FindEntityByTag_Native(string tag);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong InstantiateEntity_Native();
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong CreateEntity_Native(ulong entityID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong Instantiate_Native(ulong entityID, ulong prefabID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong InstantiateWithTranslation_Native(ulong entityID, ulong prefabID, ref Vector3 translation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong DestroyEntity_Native(ulong entityID);

    }
}