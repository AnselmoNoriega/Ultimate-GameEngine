using System;
using System.Runtime.CompilerServices;

namespace NR
{
    public class Entity
    {
        public ulong ID { get; private set; }

        private Action<float> _collision2DBeginCallbacks;
        private Action<float> _collision2DEndCallbacks;

        private Action<float> _collisionBeginCallbacks;
        private Action<float> _collisionEndCallbacks;

        private Action<float> _triggerBeginCallbacks;
        private Action<float> _triggerEndCallbacks;

        protected Entity() { ID = 0; }

        internal Entity(ulong id)
        {
            ID = id;
        }

        ~Entity()
        {

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
            return new Entity(entityID);
        }

        public Entity FindEntityByID(ulong entityID)
        {
            return new Entity(entityID);
        }

        public void AddCollision2DBeginCallback(Action<float> callback)
        {
            _collision2DBeginCallbacks += callback;
        }

        public void AddCollision2DEndCallback(Action<float> callback)
        {
            _collision2DEndCallbacks += callback;
        }

        public void AddCollisionBeginCallback(Action<float> callback)
        {
            _collisionBeginCallbacks += callback;
        }

        public void AddCollisionEndCallback(Action<float> callback)
        {
            _collisionEndCallbacks += callback;
        }

        public void AddTriggerBeginCallback(Action<float> callback)
        {
            _triggerBeginCallbacks += callback;
        }

        public void AddTriggerEndCallback(Action<float> callback)
        {
            _triggerEndCallbacks += callback;
        }

        private void CollisionBegin(float data)
        {
            if (_collisionBeginCallbacks != null)
            {
                _collisionBeginCallbacks.Invoke(data);
            }
        }

        private void CollisionEnd(float data)
        {
            if (_collisionEndCallbacks != null)
            {
                _collisionEndCallbacks.Invoke(data);
            }
        }

        private void Collision2DBegin(float data)
        {
            _collision2DBeginCallbacks.Invoke(data);
        }

        private void Collision2DEnd(float data)
        {
            _collision2DEndCallbacks.Invoke(data);
        }

        private void TriggerBegin(float data)
        {
            if (_triggerBeginCallbacks != null)
            { 
                _triggerBeginCallbacks.Invoke(data); 
            }
        }

        private void TriggerEnd(float data)
        {
            if (_triggerEndCallbacks != null)
            {
                _triggerEndCallbacks.Invoke(data);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void CreateComponent_Native(ulong entityID, Type type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool HasComponent_Native(ulong entityID, Type type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong FindEntityByTag_Native(string tag);

    }
}