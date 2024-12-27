using System.Runtime.CompilerServices;

namespace NR
{
    public static class Scene
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern Entity[] GetEntities();
    }
}