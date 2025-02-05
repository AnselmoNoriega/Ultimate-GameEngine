using NR;

public class PlayerController : Entity
{
    public Entity Camera;
    //private RigidBodyComponent rb;

    private float speed = 5.0f;
    //private float jumpForce = 5.0f;
    //private bool isGrounded = true;

    //public World world;

    public void Init()
    {
        //AddCollisionBeginCallback(CollisionStart);
        //rb = GetComponent<RigidBodyComponent>();
        //world = FindEntityByTag("World").As<World>();
    }

    public void Update(float deltaTime)
    {
        Vector3 moveDirection = Vector3.Zero;

        if (Input.IsKeyPressed(KeyCode.W)) moveDirection.x += speed * deltaTime;
        if (Input.IsKeyPressed(KeyCode.S)) moveDirection.x -= speed * deltaTime;
        if (Input.IsKeyPressed(KeyCode.A)) moveDirection.z -= speed * deltaTime;
        if (Input.IsKeyPressed(KeyCode.D)) moveDirection.z += speed * deltaTime;

        Log.Debug($"X: {moveDirection.x}, Y: {moveDirection.y}, Z: {moveDirection.z}");

        Translation += moveDirection;

        //Vector3 moveDirection = Vector3.Zero;

        //if (Input.IsKeyPressed(KeyCode.W)) moveDirection += GetCameraForward();
        //if (Input.IsKeyPressed(KeyCode.S)) moveDirection -= GetCameraForward();
        //if (Input.IsKeyPressed(KeyCode.A)) moveDirection -= GetCameraRight();
        //if (Input.IsKeyPressed(KeyCode.D)) moveDirection += GetCameraRight();

        //if (moveDirection.Length() > 0)
        //{
        //    moveDirection = moveDirection.Normalized();
        //}

        //// Apply movement using Rigidbody
        //Vector3 velocity = rb.Velocity;
        //velocity.x = moveDirection.x * speed;
        //velocity.z = moveDirection.z * speed;

        //// Simple ground check based on Y position
        //if (rb.Velocity.y <= -0.1)
        //{
        //    isGrounded = false;
        //}

        //// Jumping
        //if (Input.IsKeyPressed(KeyCode.Space) && isGrounded)
        //{
        //    velocity.y = jumpForce;
        //    isGrounded = false;
        //}

        //rb.Velocity = velocity;
    }

    //private Vector3 GetCameraForward()
    //{
    //    return new Vector3(
    //        Mathf.Sin(Camera.Rotation.y),
    //        0,
    //        Mathf.Cos(Camera.Rotation.y)
    //    ).Normalized();
    //}

    //private Vector3 GetCameraRight()
    //{
    //    return new Vector3(
    //        Mathf.Cos(Camera.Rotation.y),
    //        0,
    //        -Mathf.Sin(Camera.Rotation.y)
    //    ).Normalized();
    //}

    //private void CollisionStart(Entity other)
    //{
    //    if (other.Tag == "Terrain")
    //    {
    //        isGrounded = true;
    //    }
    //}

    //private void HandleMouseClick()
    //{
    //    RaycastHit hit;
    //    if (Physics.Raycast(Camera.Translation, Camera.Forward, 5.0f, out hit))
    //    {
    //        ModifyTerrain(hit);
    //    }
    //}

    private void ModifyTerrain(RaycastHit hit)
    {
        //world.SetVoxel(hit, VoxelType.Air);
    }
}