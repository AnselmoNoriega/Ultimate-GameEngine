using System.Runtime.CompilerServices;

namespace NR
{
    public class Time
    {
        public static float DeltaTime { get => GetDeltaTime_Native();}

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetDeltaTime_Native();
    }
}
