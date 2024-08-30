using System;

namespace NR
{
    public class Collider
    {
        public ulong EntityID { get; protected set; }
        public bool IsTrigger { get; protected set; }
    }

    public class BoxCollider : Collider
    {
        public Vector3 Size { get; private set; }
        public Vector3 Offset { get; private set; }

        private BoxCollider(ulong entityID, Vector3 size, Vector3 offset, bool isTrigger)
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

        private SphereCollider(ulong entityID, float radius, bool isTrigger)
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

        private CapsuleCollider(ulong entityID, float radius, float height, bool isTrigger)
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

        private MeshCollider(ulong entityID, IntPtr mesh, bool isTrigger)
        {
            EntityID = entityID;
            IsTrigger = isTrigger;
            Mesh = new Mesh(mesh);
        }
    }
}