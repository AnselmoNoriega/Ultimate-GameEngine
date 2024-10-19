using System;

using NR;

public class Script : Entity
{
    public float Speed = 5.0f;

    public void Init()
    {
        Console.WriteLine("Script.OnCreate");
        for (int i = 0; i < 10000; i++)
        {
            Instantiate();
        }
    }

    public void Update(float ts)
    {
        
    }

}