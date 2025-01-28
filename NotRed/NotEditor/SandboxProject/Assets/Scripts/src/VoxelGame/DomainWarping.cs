using NR;
using System.Collections;
using System.Collections.Generic;
public class DomainWarping
{
    public NoiseSettings noiseDomainX, noiseDomainY;
    public int amplitudeX = 80, amplitudeY = 80;

    public DomainWarping()
    {
        noiseDomainX = new NoiseSettings();
        noiseDomainX.Zoom = 0.16f;
        noiseDomainX.Octaves = 2;
        noiseDomainX.Offset = new Vector2(10.0f, 50.0f);
        noiseDomainX.WorldOffset = new Vector2();
        noiseDomainX.Persistance = 2.3f;
        noiseDomainX.RedistributionModifier = 0.9f;
        noiseDomainX.Exponent = 1.0f;

        noiseDomainY = new NoiseSettings();
        noiseDomainY.Zoom = 0.16f;
        noiseDomainY.Octaves = 2;
        noiseDomainY.Offset = new Vector2(10.0f, 50.0f);
        noiseDomainY.WorldOffset = new Vector2();
        noiseDomainY.Persistance = 2.3f;
        noiseDomainY.RedistributionModifier = 0.9f;
        noiseDomainY.Exponent = 1.0f;
    }

    public float GenerateDomainNoise(float x, float z, NoiseSettings defaultNoiseSettings)
    {
        Vector2 domainOffset = GenerateDomainOffset(x, z);
        return Noise.OctavePerlin(x + domainOffset.x, z + domainOffset.y, defaultNoiseSettings);
    }

    public Vector2 GenerateDomainOffset(float x, float z)
    {
        var noiseX = Noise.OctavePerlin(x, z, noiseDomainX) * amplitudeX;
        var noiseY = Noise.OctavePerlin(x, z, noiseDomainY) * amplitudeY;
        return new Vector2(noiseX, noiseY);
    }

    public Vector2 GenerateDomainOffsetInt(float x, float z)
    {
        return GenerateDomainOffset(x, z);
    }
}