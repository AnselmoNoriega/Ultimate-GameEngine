using NotRed;
using System;

namespace Sandbox
{
    public class Player : Entity
    {
        private TransformComponent _transform;
        private Rigidbody2DComponent _rigidbody;

        float _speed = 0.01f;

        void Create()
        {
            Console.WriteLine($"Player.OnCreate - {ID}");

            _transform = GetComponent<TransformComponent>();
            _rigidbody = GetComponent<Rigidbody2DComponent>();
        }

        void Update(float ts)
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

            velocity *= _speed;

            _rigidbody.ApplyImpulse(velocity.XY, true);
        }

    }
}