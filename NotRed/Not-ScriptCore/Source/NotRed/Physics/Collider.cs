using System;

namespace NR
{
    public class Collider
    {
        public ulong EntityID { get; protected set; }
        public bool IsTrigger { get; protected set; }

        private Entity entity;
        private RigidBodyComponent _rigidBodyComponent;

        public Entity Entity
        {
            get
            {
                if (entity == null)
                {
                    entity = new Entity(EntityID);
                }

                return entity;
            }
        }

        public RigidBodyComponent RigidBody
        {
            get
            {
                if (_rigidBodyComponent == null)
                {
                    _rigidBodyComponent = Entity.GetComponent<RigidBodyComponent>();
                }

                return _rigidBodyComponent;
            }
        }
    }

    public class BoxCollider : Collider
    {
        public Vector3 Size { get; private set; }
        public Vector3 Offset { get; private set; }
        
		internal BoxCollider(ulong entityID, Vector3 size, Vector3 offset, bool isTrigger)
        {
            EntityID = entityID;
            Size = size;
            Offset = offset;
            IsTrigger = isTrigger;
        }
    }

    public class SphereCollider : Collider
    {
        public float Radius { get; private set; }

        internal SphereCollider(ulong entityID, float radius, bool isTrigger)
        {
            EntityID = entityID;
            Radius = radius;
            IsTrigger = isTrigger;
        }
    }

    public class CapsuleCollider : Collider
    {
        public float Radius { get; private set; }
        public float Height { get; private set; }

        internal CapsuleCollider(ulong entityID, float radius, float height, bool isTrigger)
        {
            EntityID = entityID;
            Radius = radius;
            Height = height;
            IsTrigger = isTrigger;
        }
    }

    public class MeshCollider : Collider
    {
        public Mesh Mesh { get; private set; }

        internal MeshCollider(ulong entityID, bool isTrigger, IntPtr mesh, bool isStatic)
        {
            EntityID = entityID;
            IsTrigger = isTrigger;
            Mesh = new Mesh(mesh, isStatic);
        }
    }
}