Scene: Name
Entities:
  - Entity: 4274657439661873420
    TagComponent:
      Tag: FloorT
    TransformComponent:
      Translation: [-2.47597647, 0.307433456, 0]
      Rotation: [0, 0, -0.720501542]
      Scale: [4.4779582, 0.447594553, 1]
    SpriteRendererComponent:
      Color: [0.6875, 0.284119874, 0.216889873, 1]
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
  - Entity: 57677566890593907
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
  - Entity: 7458784592694837773
    TagComponent:
      Tag: Floor
    TransformComponent:
      Translation: [0, -1.60128093, 0]
      Rotation: [0, 0, 0]
      Scale: [10.6568756, 0.447594285, 1]
    SpriteRendererComponent:
      Color: [0.678571463, 0.272351831, 0.161564618, 1]
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
  - Entity: 2528896276513038877
    TagComponent:
      Tag: Ball
    TransformComponent:
      Translation: [-2.75193977, 2.08502007, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    CircleRendererComponent:
      Color: [1, 1, 1, 1]
      Thickness: 1
    Rigidbody2DComponent:
      BodyType: Dynamic
      FixedRotation: false
      Density: 1
      Friction: 0.5
      Restitution: 0.100000001
      RestitutionThreshold: 0.5
    CircleCollider2DComponent:
      Offset: [0, 0]
      Radius: 0.5
  - Entity: 9147471346879783186
    TagComponent:
      Tag: Object
    TransformComponent:
      Translation: [0, 0, 0]
      Rotation: [0, 0, 0.596902609]
      Scale: [0.5, 0.5, 1]
    ScriptComponent:
      ClassName: Player
    SpriteRendererComponent:
      Color: [1, 1, 1, 1]
      TexturePath: Assets\Textures\Image_Two.png
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