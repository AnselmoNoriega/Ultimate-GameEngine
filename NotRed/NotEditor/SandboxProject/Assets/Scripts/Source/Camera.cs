namespace NotRed
{
    public class Camera : Entity
    {
        float _speed = 1.0f;

        void Update(float dt)
        {
            Vector3 velocity = Vector3.Zero;

            if (Input.IsKeyDown(KeyCode.Up))
            {
                velocity.Y = 1.0f;
            }
            else if (Input.IsKeyDown(KeyCode.Down))
            { 
                velocity.Y = -1.0f;
            }

            if (Input.IsKeyDown(KeyCode.Left))
            {
                velocity.X = -1.0f;
            }
            else if (Input.IsKeyDown(KeyCode.Right))
            {
                velocity.X = 1.0f;
            }

            velocity *= _speed;

            Vector3 translation = Translation;
            translation += velocity * dt;
            Translation = translation;
        }

    }
}