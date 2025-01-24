using System.Runtime.CompilerServices;

namespace NR
{
    public struct NoiseSettings
    {
        public float Zoom;
        public int Octaves;
        public Vector2 Offset;
        public Vector2 WorldOffset;
        public float Persistance;
        public float RedistributionModifier;
        public float Exponent;
    }

    public static class Noise
    {
        public static float PerlinNoise(float x, float y)
        {
            return PerlinNoise_Native(x, y);
        }

        public static float RemapValue(float value, float initialMin, float initialMax, float outputMin, float outputMax)
        {
            return outputMin + (value - initialMin) * (outputMax - outputMin) / (initialMax - initialMin);
        }

        public static float RemapValue(float value, float outputMin, float outputMax)
        {
            return outputMin + (value - 0) * (outputMax - outputMin) / (1 - 0);
        }

        public static int RemapValueToInt(float value, float outputMin, float outputMax)
        {
            return (int)RemapValue(value, outputMin, outputMax);
        }

        public static float Redistribution(float noise, NoiseSettings settings)
        {
            return Mathf.Pow(noise * settings.RedistributionModifier, settings.Exponent);
        }

        public static float OctavePerlin(float x, float z, NoiseSettings settings)
        {
            x *= settings.Zoom;
            z *= settings.Zoom;
            x += settings.Zoom;
            z += settings.Zoom;
            float total = 0;
            float frequency = 1;
            float amplitude = 1;
            float amplitudeSum = 0;
            for (int i = 0; i < settings.Octaves; ++i)
            {
                total += PerlinNoise((settings.Offset.x + settings.WorldOffset.x + x) * frequency, (settings.Offset.y + settings.WorldOffset.y + z) * frequency) * amplitude;
                amplitudeSum += amplitude;
                amplitude *= settings.Persistance;
                frequency *= 2;
            }
            return total / amplitudeSum;
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float PerlinNoise_Native(float x, float y);

    }
}