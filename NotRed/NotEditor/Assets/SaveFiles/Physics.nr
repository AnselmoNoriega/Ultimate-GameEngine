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
    ? Entity: 123456
      TagComponent:
        Tag: Object
      TransformComponent:
        Translation: [0, 0, 0]
        Rotation: [0, 0, 0]
        Scale: [1, 1, 1]
      ? Entity: 123456
        TagComponent:
          Tag: ObjectOne
        TransformComponent:
          Translation: [-0.0104218759, 1.15808249, 0]
          Rotation: [0, 0, 0.619940937]
          Scale: [0.530000031, 0.530000031, 0.529999971]
        SpriteRendererComponent:
          Color: [1, 1, 1, 1]
        Rigidbody2DComponent:
          BodyType: Dynamic
          FixedRotation: false
          Density: 1
          Friction: 0.5
          Restitution: 0
          RestitutionThreshold: 0.5
        BoxCollider2DComponent:
          Offset: [0, 0]
          Size: [0.5, 0.5]
      : Entity: 123456
        TagComponent:
          Tag: Floor
        TransformComponent:
          Translation: [0, -1.01199806, 0]
          Rotation: [0, 0, 0]
          Scale: [6.9367609, 0.669119418, 1.55999994]
        SpriteRendererComponent:
          Color: [0.934523821, 0.324421257, 0.0111252973, 1]
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