using System;
using NR;

public class CameraControllerOld : Entity
{
    public float MoveSpeed = 8.0f;
    public float MouseSensitivity = 1.0f;

    private TransformComponent m_Transform;
    private Vector2 m_LastMousePosition;

    private bool m_CameraMovementEnabled = true;

    public void Init()
    {
        m_Transform = GetComponent<TransformComponent>();
    }

    //public void Update(float ts)
    //{

    //    if (Input.IsKeyPressed(KeyCode.W))
    //    {
    //        m_Transform.Transform.Forward = false;
    //    }

    //    if (Input.IsKeyPressed(KeyCode.F5) && !m_CameraMovementEnabled)
    //    {
    //        m_CameraMovementEnabled = true;
    //    }

    //    Vector2 currentMousePosition = Input.GetMousePosition();
    //    if (m_CameraMovementEnabled)
    //    {
    //        Vector2 delta = m_LastMousePosition - currentMousePosition;
    //        Vector3 rotation = m_Transform.Rotation;
    //        Vector3 translation = m_Transform.Translation;
    //        rotation.y = (rotation.y - delta.x * MouseSensitivity * ts) % Mathf.PI * 2;

    //        translation.x = DistanceFromPlayer * Mathf.Sin(rotation.y);
    //        translation.z = DistanceFromPlayer * -Mathf.Cos(rotation.y);
    //        m_Transform.Translation = translation;
    //        m_Transform.Rotation = rotation;
    //    }
    //    m_LastMousePosition = currentMousePosition;
    //}

}
