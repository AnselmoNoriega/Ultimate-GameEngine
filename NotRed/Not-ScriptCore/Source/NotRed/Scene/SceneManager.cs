using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;

namespace Hazel
{
    public static class SceneManager
    {
        public static void LoadScene(string scene) => LoadScene_Native(scene);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void LoadScene_Native(string scene);
    }
}