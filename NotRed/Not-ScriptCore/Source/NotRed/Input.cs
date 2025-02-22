using System.Runtime.CompilerServices;

namespace NR
{
    public enum CursorMode
    {
        Normal,
        Hidden,
        Locked
    }

    public enum MouseButton
    {
        Button0,
        Button1,
        Button2,
        Button3,
        Button4,
        Button5,
        Left = Button0,
        Right = Button1,
        Middle = Button2
    }

    public class Input
    {
        public static bool IsKeyPressed(KeyCode keycode)
        {
            return IsKeyPressed_Native(keycode);
        }

        public static bool IsMouseButtonPressed(MouseButton button)
        {
            return IsMouseButtonPressed_Native(button);
        }

        public static Vector2 GetMousePosition()
        {
            GetMousePosition_Native(out Vector2 position);
            return position;
        }

        public static bool IsControllerPresent(int id)
        {
            return IsControllerPresent_Native(id);
        }

        public static int[] GetConnectedControllerIDs()
        {
            return GetConnectedControllerIDs_Native();
        }

        public static string GetControllerName(int id)
        {
            return GetControllerName_Native(id);
        }

        public static bool IsControllerButtonPressed(int id, int button)
        {
            return IsControllerButtonPressed_Native(id, button);
        }

        public static float GetControllerAxis(int id, int axis)
        {
            return GetControllerAxis_Native(id, axis);
        }

        public static byte GetControllerHat(int id, int hat)
        {
            return GetControllerHat_Native(id, hat);
        }

        public static void SetCursorMode(CursorMode mode) => SetCursorMode_Native(mode);
        public static CursorMode GetCursorMode() => GetCursorMode_Native();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool IsKeyPressed_Native(KeyCode keycode);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsMouseButtonPressed_Native(MouseButton button);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetMousePosition_Native(out Vector2 position);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetCursorMode_Native(CursorMode mode);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern CursorMode GetCursorMode_Native();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsControllerPresent_Native(int id);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern int[] GetConnectedControllerIDs_Native();
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern string GetControllerName_Native(int id);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsControllerButtonPressed_Native(int id, int button);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern float GetControllerAxis_Native(int id, int axis);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern byte GetControllerHat_Native(int id, int hat);
    }
}