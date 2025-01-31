using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace NR
{
    public class Time
    {
        public static float DeltaTime { get => GetDeltaTime_Native();}

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetDeltaTime_Native();
    }
}
