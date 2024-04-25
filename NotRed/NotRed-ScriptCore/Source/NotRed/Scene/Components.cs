namespace NotRed
{
    public abstract class Component
    {
        public Entity Entity { get; internal set; }
    }

    public class TransformComponent : Component
    {
        public Vector3 Translation
        {
            get
            {
                InternalCalls.TransformComponent_GetTranslation(Entity.ID, out Vector3 translation);
                return translation;
            }
            set
            {
                InternalCalls.TransformComponent_SetTranslation(Entity.ID, ref value);
            }
        }
    }

    public class Rigidbody2DComponent : Component
    {
        public enum BodyType { Static, Kinematic, Dynamic }

        public Vector2 LinearVelocity
        {
            get
            {
                InternalCalls.Rigidbody2DComponent_GetVelocity(Entity.ID, out Vector2 velocity);
                return velocity;
            }
        }

        public BodyType Type
        {
            get => InternalCalls.Rigidbody2DComponent_GetType(Entity.ID);
            set => InternalCalls.Rigidbody2DComponent_SetType(Entity.ID, value);
        }

        public void ApplyImpulse(Vector2 impulse, Vector2 worldPosition, bool wake)
        {
            InternalCalls.Rigidbody2DComponent_ApplyImpulse(Entity.ID, ref impulse, ref worldPosition, wake);
        }

        public void ApplyImpulse(Vector2 impulse, bool wake)
        {
            InternalCalls.Rigidbody2DComponent_ApplyImpulseToCenter(Entity.ID, ref impulse, wake);
        }

    }
}
