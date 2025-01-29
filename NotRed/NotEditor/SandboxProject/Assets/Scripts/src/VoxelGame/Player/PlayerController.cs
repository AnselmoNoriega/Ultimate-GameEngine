using NR;

public class PlayerController
{
    public Entity Camera; 
    public Entity Player;

    private Vector3 velocity = Vector3.Zero;
    private float speed = 5.0f;
    private float jumpForce = 5.0f;
    private float gravity = 9.81f;
    private bool isGrounded = true;

    public void Update(float dt)
    {
        Vector3 direction = Vector3.Zero;

        if (Input.IsKeyPressed(KeyCode.W)) direction += GetCameraForward();
        if (Input.IsKeyPressed(KeyCode.S)) direction -= GetCameraForward();
        if (Input.IsKeyPressed(KeyCode.A)) direction -= GetCameraRight();
        if (Input.IsKeyPressed(KeyCode.D)) direction += GetCameraRight();

        // Normalize direction to maintain consistent speed.
        if (direction.Length() > 0)
        {
            direction = direction.Normalized();
        }

        // Apply movement
        Player.Translation += direction * speed * dt;

        // Jumping
        if (Input.IsKeyPressed(KeyCode.Space) && isGrounded)
        {
            velocity.y = jumpForce;
            isGrounded = false;
        }

        // Apply gravity
        velocity.y -= gravity * dt;
        Player.Translation += velocity * dt;

        // Check if grounded (simplified, depends on your physics system)
        if (Player.Translation.y <= 0.0f)
        {
            Player.Translation.y = 0.0f;
            isGrounded = true;
            velocity.y = 0;
        }
    }

    private Vector3 GetCameraForward()
    {
        Vector3 forward = new Vector3(
            Mathf.Sin(Camera.Rotation.y),
        0,
            Mathf.Cos(Camera.Rotation.y)
        );
        return forward.Normalized();
    }

    private Vector3 GetCameraRight()
    {
        Vector3 right = new Vector3(
            Mathf.Cos(Camera.Rotation.y),
        0,
            -Mathf.Sin(Camera.Rotation.y)
        );
        return right.Normalized();
    }
}