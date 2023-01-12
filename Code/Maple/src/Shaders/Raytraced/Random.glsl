#ifndef RANDOM_GLSL
#define RANDOM_GLSL

struct Random
{
    uvec2 s;
};

// xoroshiro64* random number generator.
// http://prandom.di.unimi.it/xoroshiro64star.c
uint randomRotl(uint x, uint k)
{
    return (x << k) | (x >> (32 - k));
}

// Xoroshiro64* random
uint randomNext(inout Random random)
{
    uint result = random.s.x * 0x9e3779bb;

    random.s.y ^= random.s.x;
    random.s.x = randomRotl(random.s.x, 26) ^ random.s.y ^ (random.s.y << 9);
    random.s.y = randomRotl(random.s.y, 13);

    return result;
}

// Thomas Wang 32-bit hash.
// http://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/
uint randomHash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

Random randomInit(uvec2 id, uint frameIndex)
{
    uint s0 = (id.x << 16) | id.y;
    uint s1 = frameIndex;

    Random random;
    random.s.x = randomHash(s0);
    random.s.y = randomHash(s1);
    randomNext(random);
    return random;
}


float nextFloat(inout Random rand)
{
    uint u = 0x3f800000 | (randomNext(rand) >> 9);
    return uintBitsToFloat(u) - 1.0;
}

uint nextUInt(inout Random rand, uint nmax)
{
    float f = nextFloat(rand);
    return uint(floor(f * nmax));
}

vec2 nextVec2(inout Random rand)
{
    return vec2(nextFloat(rand), nextFloat(rand));
}

vec3 nextVec3(inout Random rand)
{
    return vec3(nextFloat(rand), nextFloat(rand), nextFloat(rand));
}

mat3 makeRotationMatrix(vec3 z)
{
    const vec3 ref = abs(dot(z, vec3(0, 1, 0))) > 0.99f ? vec3(0, 0, 1) : vec3(0, 1, 0);

    const vec3 x = normalize(cross(ref, z));
    const vec3 y = cross(z, x);

    return mat3(x, y, z);
}

#endif