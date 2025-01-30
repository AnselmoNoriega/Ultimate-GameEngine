using NR;

public class VoxelGameCamera : Entity
{
    private float sensitivity = 0.1f; // Mouse sensitivity
    private float rotationX = 0.0f;
    private float rotationY = 0.0f;

    public Entity player; // The entity that this camera is attached to

    public void Update(float deltaTime)
    {
        // Get mouse movement
        Vector2 mouseDelta = Input.GetMousePosition() * sensitivity;

        // Rotate the camera (look up/down)
        rotationY -= mouseDelta.y;
        rotationY = Mathf.Clamp(rotationY, -90f, 90f);

        rotationX += mouseDelta.x;

        // Apply rotations
        player.Rotation = new Vector3(0, rotationX, 0.0f);
        Rotation = new Vector3(rotationY, rotationX, 0.0f);
    }
}