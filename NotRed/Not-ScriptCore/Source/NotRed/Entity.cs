using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;

namespace NR
{
    public class Entity
    {
        private Action<Entity> _collision2DBeginCallbacks;
        private Action<Entity> _collision2DEndCallbacks;

        private Action<Entity> _collisionBeginCallbacks;
        private Action<Entity> _collisionEndCallbacks;

        private Action<Entity> _triggerBeginCallbacks;
        private Action<Entity> _triggerEndCallbacks;

        private Action<Vector3, Vector3> _jointBreakCallbacks;

        private List<IEnumerator> coroutines = new List<IEnumerator>(); 
        private readonly object coroutineLock = new object();
        private bool isRunning = false;

        protected Entity() { ID = 0; }

        internal Entity(ulong id)
        {
            ID = id;
        }

        ~Entity()
        {

        }

        public Entity Create()
        {
            return new Entity(CreateEntity_Native());
        }

        public Entity Instantiate(Prefab prefab)
        {
            ulong entityID = Instantiate_Native(prefab.ID);
            if (entityID == 0)
            {
                return null;
            }

            return new Entity(entityID);
        }

        public Entity Instantiate(Prefab prefab, Vector3 translation)
        {
            ulong entityID = InstantiateWithTranslation_Native(prefab.ID, ref translation);
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

        public Vector3 Forward
        {
            get
            {
                float radX = Rotation.x * (Mathf.PI / 180f);
                float radY = Rotation.y * (Mathf.PI / 180f);

                float cosPitch = Mathf.Cos(radX);
                float sinPitch = Mathf.Sin(radX);
                float cosYaw = Mathf.Cos(radY);
                float sinYaw = Mathf.Sin(radY);

                return new Vector3(cosYaw * cosPitch, sinPitch, sinYaw * cosPitch);
            }
        }

        public ulong ID { get; private set; }
        public string Tag => GetComponent<TagComponent>().Tag;

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

        /// <summary>
        /// Returns true if it possbile to treat this entity as if it were of type T,
        /// returns false otherwise.
        /// <returns></returns>
        public bool Is<T>() where T : Entity, new()
        {
            ScriptComponent sc = GetComponent<ScriptComponent>();
            object instance = sc != null ? sc.Instance : null;
            return instance is T ? true : false;
        }

        /// <summary>
        /// Returns this entity cast to type T if that is possible, otherwise returns null.
        /// <returns></returns>
        public T As<T>() where T : Entity, new()
        {
            ScriptComponent sc = GetComponent<ScriptComponent>();
            object instance = sc != null ? sc.Instance : null;
            return instance is T ? instance as T : null;
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

        public void AddJointBreakCallback(Action<Vector3, Vector3> callback) => _jointBreakCallbacks += callback;

        private void CollisionBegin(ulong id)
        {
            _collisionBeginCallbacks?.Invoke(new Entity(id));
        }

        private void CollisionEnd(ulong id)
        {
            _collisionEndCallbacks?.Invoke(new Entity(id));
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
            _triggerBeginCallbacks?.Invoke(new Entity(id));
        }

        private void TriggerEnd(ulong id)
        {
            _triggerEndCallbacks?.Invoke(new Entity(id));
        }

        private void OnJointBreak(Vector3 linearForce, Vector3 angularForce)
        {
            _jointBreakCallbacks?.Invoke(linearForce, angularForce);
        }

        public void StartCoroutine(IEnumerator coroutine)
        {
            lock (coroutineLock)
            {
                coroutines.Add(coroutine);
            }

            if (!isRunning)
            {
                isRunning = true;
                Task.Run(UpdateCoroutines);
            }
        }

        public void StopAllCoroutines()
        {
            lock (coroutineLock)
            {
                coroutines.Clear();
            }

            isRunning = false;
        }

        private async Task UpdateCoroutines()
        {
            while (true)
            {
                List<IEnumerator> coroutinesSnapshot;

                lock (coroutineLock)
                {
                    if (coroutines.Count == 0)
                    {
                        break;
                    }
                    
                    coroutinesSnapshot = new List<IEnumerator>(coroutines);
                }

                for (int i = coroutinesSnapshot.Count - 1; i >= 0; i--)
                {
                    var coroutine = coroutinesSnapshot[i];
                    if (!coroutine.MoveNext())
                    {
                        lock (coroutineLock)
                        {
                            coroutines.Remove(coroutine);
                        }
                    }
                }

                float millisecondsToWait = Time.DeltaTime * 1000f;
                await Task.Delay((int)millisecondsToWait);
            }

            isRunning = false;
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
        private static extern ulong CreateEntity_Native();
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong Instantiate_Native(ulong prefabID);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong InstantiateWithTranslation_Native(ulong prefabID, ref Vector3 translation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong DestroyEntity_Native(ulong entityID);

    }

    public class WaitForSeconds : IEnumerator
    {
        private float waitTime;
        private float elapsedTime = 0.0f;

        public WaitForSeconds(float seconds)
        {
            waitTime = seconds;
        }

        public bool MoveNext()
        {
            elapsedTime += Time.DeltaTime; // Uses static deltaTime
            return elapsedTime < waitTime;
        }

        public void Reset() { elapsedTime = 0f; }
        public object Current => null;
    }
}