using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;

namespace NR
{
    public class Entity
    {
        public ulong ID { get; private set; }

        //TODO change to same format (not a list)
        private List<Action<float>> _collision2DBeginCallbacks = new List<Action<float>>();
        private List<Action<float>> _collision2DEndCallbacks = new List<Action<float>>();
        private Action<float> _collisionBeginCallbacks;
        private Action<float> _collisionEndCallbacks;

        protected Entity() { ID = 0; }

        internal Entity(ulong id)
        {
            ID = id;
        }

        ~Entity()
        {

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

        public Matrix4 GetTransform()
        {
            Matrix4 mat4Instance;
            GetTransform_Native(ID, out mat4Instance);
            return mat4Instance;
        }

        public Vector3 GetForwardDirection()
        {
            GetForwardDirection_Native(ID, out Vector3 forward);
            return forward;
        }

        public Vector3 GetRightDirection()
        {
            GetRightDirection_Native(ID, out Vector3 right);
            return right;
        }

        public Vector3 GetUpDirection()
        {
            GetUpDirection_Native(ID, out Vector3 up);
            return up;
        }

        public void SetTransform(Matrix4 transform)
        {
            SetTransform_Native(ID, ref transform);
        }
        public void AddCollision2DBeginCallback(Action<float> callback)
        {
            _collision2DBeginCallbacks.Add(callback);
        }

        public void AddCollision2DEndCallback(Action<float> callback)
        {
            _collision2DEndCallbacks.Add(callback);
        }

        public void AddCollisionBeginCallback(Action<float> callback)
        {
            _collisionBeginCallbacks += callback;
        }

        public void AddCollisionEndCallback(Action<float> callback)
        {
            _collisionEndCallbacks += callback;
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
            foreach (var callback in _collision2DBeginCallbacks)
            {
                callback.Invoke(data);
            }
        }

        private void Collision2DEnd(float data)
        {
            foreach (var callback in _collision2DEndCallbacks)
            {
                callback.Invoke(data);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void CreateComponent_Native(ulong entityID, Type type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool HasComponent_Native(ulong entityID, Type type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void GetTransform_Native(ulong entityID, out Matrix4 matrix);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SetTransform_Native(ulong entityID, ref Matrix4 matrix);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void GetForwardDirection_Native(ulong entityID, out Vector3 forward);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void GetRightDirection_Native(ulong entityID, out Vector3 right);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void GetUpDirection_Native(ulong entityID, out Vector3 up);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong FindEntityByTag_Native(string tag);

    }
}