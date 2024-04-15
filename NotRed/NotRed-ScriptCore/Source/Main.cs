using System;
using System.Runtime.CompilerServices;

namespace NR
{
    public static class InternalCalls
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void NativeLog(string text, int parameter);
    }

    public class Entity
    {
        public Entity()
        {
            Console.WriteLine("Main constructor!");
            Log("First message", 3000);
        }

        public void PrintMessage()
        {
            Console.WriteLine("Printed in C#");
        }

        public void PrintInt(int value)
        {
            Console.WriteLine(value.ToString());
        }

        public void PrintInts(int value, int value2)
        {
            Console.WriteLine(value.ToString() + " " + value2.ToString());
        }

        public void PrintCustomMessage(string message)
        {
            Console.WriteLine(message);
        }

        private void Log(string text, int parameter)
        {
            InternalCalls.NativeLog(text, parameter);
        }
    }
}