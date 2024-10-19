using System;

namespace NR
{
    public class RuntimeException
    {
        public static void Exception(object exception)
        {
            Console.WriteLine("RuntimeException.OnException");
            if (exception != null)
            {
                if (exception is NullReferenceException)
                {
                    var e = exception as NullReferenceException;
                    Console.WriteLine(e.Message);
                    Console.WriteLine(e.Source);
                    Console.WriteLine(e.StackTrace);
                }
            }
            else
            {
                Console.WriteLine("Exception is null");
            }
        }
    }
}