using NR;

public enum Direction
{
    foreward,    // +z direction
    right,       // +x direction
    backwards,   // -z direction
    left,        // -x direction
    up,          // +y direction
    down         // -y direction
}

public static class DirectionExtensions
{
    public static Vector3 GetVector(this Direction direction)
    {
        switch (direction)
        {
            case Direction.up:            return Vector3.Up;
            case Direction.down:          return Vector3.Down;
            case Direction.right:         return Vector3.Right;
            case Direction.left:          return Vector3.Left;
            case Direction.foreward:      return Vector3.Forward;
            case Direction.backwards:     return Vector3.Back;
        }

        return Vector3.Zero;
    }
}