using System;

using NR;

public class Script : Entity
{
    public float Speed = 5.0f;

    public void Init()
    {
        Console.WriteLine("Script.OnCreate");
    }

    public void Update(float ts)
    {
        Matrix4 transform = GetTransform();
        Vector3 translation = transform.Translation;

        float speed = Speed * ts;

        if (Input.IsKeyPressed(KeyCode.Up))
        {
            translation.y += speed;
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