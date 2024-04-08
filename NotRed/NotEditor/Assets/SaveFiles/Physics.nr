Scene: Name
Entities:
  - Entity: 123456
    TagComponent:
      Tag: Camera
    TransformComponent:
      Translation: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    CameraComponent:
      CameraInfo:
        ProjectionType: 1
        FOV: 10
        NearClip: -1
        FarClip: 1
      Primary: true
      FixedAspectRatio: false
  - Entity: 123456
    TagComponent:
      Tag: Floor
    TransformComponent:
      Translation: [0, -1.60128093, 0]
      Rotation: [0, 0, 0]
      Scale: [10.6568756, 0.447594285, 1]
    SpriteRendererComponent:
      Color: [1, 0.517857134, 0, 1]
    Rigidbody2DComponent:
      BodyType: Static
      FixedRotation: false
      Density: 1
      Friction: 0.5
      Restitution: 0
      RestitutionThreshold: 0.5
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [0.5, 0.5]
  - Entity: 123456
    TagComponent:
      Tag: Object
    TransformComponent:
      Translation: [0, 0, 0]
      Rotation: [0, 0, 0.596902609]
      Scale: [0.5, 0.5, 1]
    SpriteRendererComponent:
      Color: [1, 1, 1, 1]
    Rigidbody2DComponent:
      BodyType: Dynamic
      FixedRotation: false
      Density: 1
      Friction: 0.5
      Restitution: 0.400000006
      RestitutionThreshold: 0.5
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [0.5, 0.5]