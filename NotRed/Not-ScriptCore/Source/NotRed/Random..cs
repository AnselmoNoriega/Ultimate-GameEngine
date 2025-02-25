using System;

namespace NR
{
    public class XRandom
    {
        private ulong x;
        private ulong y;
        private ulong buffer;
        private ulong bufferMask;

        public XRandom()
        {
            x = (ulong)Guid.NewGuid().GetHashCode();
            y = (ulong)Guid.NewGuid().GetHashCode();
        }

        public XRandom(ulong seed)
        {
            x = seed << 3;
            y = seed >> 3;
        }

        public XRandom(int seed)
            : this((ulong)seed)
        {
        }

        public int Next()
        {
            int result;
            ulong tempX, tempY;
            
            tempX = y;
            x ^= x << 23;
            tempY = x ^ y ^ (x >> 17) ^ (y >> 26);
            result = (int)(tempY + y);
        
            x = tempX;
            y = tempY;
            
            return result;
        }

        public int Next(int maxValue)
        {
            if (maxValue < 0)
            {
                throw new ArgumentOutOfRangeException("maxValue");
            }

            return (int)(NextDouble() * maxValue);
        }
        public int Next(int minValue, int maxValue)
        {
            if (minValue > maxValue)
            {
                throw new ArgumentOutOfRangeException("minValue");
            }

            long range = (long)maxValue - minValue;
            
            return ((int)(NextDouble() * range) + minValue);
        }

        private const double DOUBLE_UNIT = 1.0 / (int.MaxValue + 1.0);
        
        public double NextDouble()
        {
            double result;
            ulong tempX, tempY, tempZ;
        
            tempX = y;
            x ^= x << 23;
            tempY = x ^ y ^ (x >> 17) ^ (y >> 26);
            tempZ = tempY + y;
            result = DOUBLE_UNIT * (0x7FFFFFFF & tempZ);
            
            x = tempX;
            y = tempY;
            
            return result;
        }

        public byte NextByte()
        {
            if (bufferMask >= 8)
            {
                byte result = (byte)buffer;
                buffer >>= 8;
                bufferMask >>= 8;
                return result;
            }

            ulong tempX, tempY;
            tempX = y;
            x ^= x << 23;
            tempY = x ^ y ^ (x >> 17) ^ (y >> 26);
            buffer = tempY + y;
            x = tempX;
            y = tempY;
            bufferMask = 0x8000000000000;

            return (byte)(buffer >>= 8);
        }

        public void NextBytes(byte[] buffer)
        {
            ulong _x = x, _y = y, tempX, tempY, z;
            ulong index = 0;
            ulong end = (ulong)buffer.Length;

            while (index <= end - 1)
            {
                tempX = _y;
                _x ^= _x << 23; tempY = _x ^ _y ^ (_x >> 17) ^ (_y >> 26);
                buffer[index++] = (byte)(tempY + _y);
                _x = tempX;
                _y = tempY;
            }

            if (index < end)
            {
                _x ^= _x << 23; tempY = _x ^ _y ^ (_x >> 17) ^ (_y >> 26);
                z = tempY + _y;
                while (index < end) buffer[index++] = (byte)(z >>= 8);
            }

            x = _x;
            y = _y;
        }
    }
}