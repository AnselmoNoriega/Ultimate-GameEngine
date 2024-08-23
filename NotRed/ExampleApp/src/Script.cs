using System;

using NR;

namespace Example
{
    public class Script : Entity
    {
        public float Speed = 5.0f;

        public void OnCreate()
        {
            Console.WriteLine("Script.OnCreate");
        }

        public void OnUpdate(float ts)
        {
            Matrix4 transform = GetTransform();
            Vector3 translation = transform.Translation;

            float speed = Speed * ts;

            if (Input.IsKeyPressed(KeyCode.Up))
            {
                translation.x += speed;
            }
            else if (Input.IsKeyPressed(KeyCode.Down))
            {
                translation.y -= speed;
            }
            if (Input.IsKeyPressed(KeyCode.Right))
            {
                translation.x += speed;
            }
            else if (Input.IsKeyPressed(KeyCode.Left))
            {
                translation.x -= speed;
            }

            transform.Translation = translation;
            SetTransform(transform);
        }

    }
}