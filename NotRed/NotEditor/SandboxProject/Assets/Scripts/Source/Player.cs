using NotRed;
using System;

namespace Sandbox
{
    public class Player : Entity
    {
        private TransformComponent _transform;
        private Rigidbody2DComponent _rigidbody;

        public float Speed = 0.1f;

        void Create()
        {
            Console.WriteLine($"Player.OnCreate - {ID}");

            _transform = GetComponent<TransformComponent>();
            _rigidbody = GetComponent<Rigidbody2DComponent>();
        }

        void Update(float dt)
        {
            Vector3 velocity = Vector3.Zero;

            if (Input.IsKeyDown(KeyCode.W))
            {
                velocity.Y = 5.0f;
            }
            else if (Input.IsKeyDown(KeyCode.S))
            {
                velocity.Y = -5.0f;
            }

            if (Input.IsKeyDown(KeyCode.A))
            {
                velocity.X = -1.0f;
            }
            else if (Input.IsKeyDown(KeyCode.D))
            {
                velocity.X = 1.0f;
            }

            velocity *= Speed * dt;

            _rigidbody.ApplyImpulse(velocity.XY, true);
        }

    }
}