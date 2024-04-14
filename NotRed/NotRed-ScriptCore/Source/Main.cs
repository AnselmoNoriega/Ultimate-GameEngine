using System;

namespace NR
{
    public class Main
    {
        public float FloatVar { get; set; }

        public Main()
        {
            Console.WriteLine("Hello C#");
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
    }
}