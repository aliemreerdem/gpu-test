// stress.hlsl
// GPU Stress Tester V2: Multiple Workloads

RWStructuredBuffer<float> BufferOut : register(u0);
RWStructuredBuffer<float> BufferB   : register(u1); // Used for memory bandwidth testing
RWStructuredBuffer<float> BufferC   : register(u2); // V6 G-Buffer Albedo
RWStructuredBuffer<float> BufferD   : register(u3); // V6 G-Buffer Normal/Depth

// ---------------------------------------------------------
// 1. Math Stress Kernel (ALU Bound)
// Heavy computational workload with minimal memory access
// ---------------------------------------------------------
[numthreads(256, 1, 1)]
void CSMath(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    float value = (float)dispatchThreadID.x * 0.001f;

    // Extremely heavy transcendental math loop to max out the ALUs
    // Target is around 1000 iterations to avoid Windows TDR timeout
    for (int i = 0; i < 1500; ++i)
    {
        value = sin(value) * cos(value) + tan(value) * sqrt(abs(value) + 1.0f);
        value = fmod(value, 1000.0f);
    }

    BufferOut[dispatchThreadID.x] = value;
}

// ---------------------------------------------------------
// 2. Memory Bandwidth Kernel (VRAM Bound)
// Minimal math, constant pseudo-random read/writes to stress VRAM
// ---------------------------------------------------------
[numthreads(256, 1, 1)]
void CSMemory(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    uint maxElements = 1024 * 1024 * 16; // Matches NUM_ELEMENTS
    
    // Simulate memory thrashing by jumping around the buffer
    for (int i = 0; i < 500; ++i)
    {
        uint jumpIndex = (id + (i * 104729)) % maxElements; // Prime number jump
        float data = BufferOut[jumpIndex] + BufferB[jumpIndex];
        
        BufferOut[id] = data * 0.5f;
        BufferB[jumpIndex] = data * 0.1f;
    }
}

// ---------------------------------------------------------
// 3. Simulated Game Workload (Mixed Bound)
// Moderate math combined with structured memory reads/writes
// simulates rendering pipelines (fetching textures, calculating lighting)
// ---------------------------------------------------------
[numthreads(256, 1, 1)]
void CSGame(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    uint maxElements = 1024 * 1024 * 16;
    float finalColor = 0.0f;

    // Simulating rendering objects on screen
    for (int i = 0; i < 500; ++i) 
    {
        // 1. "Texture Fetch" Phase (Memory Read)
        uint texIndex = (id + (i * 1234)) % maxElements;
        float texel = BufferB[texIndex];

        // 2. "Lighting Calculation" Phase (ALU Math)
        float lightIntensity = saturate(dot(float3(sin(texel), cos(texel), tan(texel)), float3(0.5, 0.5, 0.5)));
        float colorValue = pow(abs(texel * lightIntensity), 2.2f); // Gamma correction simulation
        
        // 3. "Post-Processing" Phase (More Math)
        finalColor += smoothstep(0.1f, 0.9f, colorValue);
    }

    // 4. "Framebuffer Write" Phase (Memory Write)
    BufferOut[id] = finalColor;
}

// ---------------------------------------------------------
// 4. Crypto Hashing Simulation (Integer / Bitwise ALU Bound)
// Simulates Bitcoin/SHA-256 style bitwise operations
// ---------------------------------------------------------
[numthreads(256, 1, 1)]
void CSCrypto(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    uint hash = id ^ 0x5bd1e995;
    uint k = 0xcc9e2d51;

    // Heavy integer and bitwise scrambling
    for (int i = 0; i < 2000; ++i)
    {
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;

        hash ^= k;
        hash = (hash << 13) | (hash >> 19);
        hash = hash * 5 + 0xe6546b64;
        
        // Extra mixer to push ALU execution units
        hash ^= hash >> 16;
        hash *= 0x85ebca6b;
        hash ^= hash >> 13;
        hash *= 0xc2b2ae35;
        hash ^= hash >> 16;
    }

    BufferOut[id] = (float)hash;
}

// ---------------------------------------------------------
// 5. Ray Tracing Math Simulation (Tensor / Matrix ALU Bound)
// Simulates complex 3D Vector intersections and Matrix Multiplications
// ---------------------------------------------------------
[numthreads(256, 1, 1)]
void CSRayTrace(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // A dummy 3x3 matrix multiplication to stress vector logic
    float3x3 m1 = float3x3(1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9);
    float3x3 m2 = float3x3(9.9, 8.8, 7.7, 6.6, 5.5, 4.4, 3.3, 2.2, 1.1);
    float3 vec = float3((float)dispatchThreadID.x * 0.001f, 1.0f, 0.5f);

    for (int i = 0; i < 500; ++i)
    {
        // Matrix multiplications
        float3x3 m3 = mul(m1, m2);
        
        // Ray-Sphere intersection math simulation
        float3 rayDir = normalize(mul(m3, vec));
        float b = dot(rayDir, rayDir);
        float c = dot(vec, vec) - 1.0f;
        float discriminant = b * b - c;
        
        vec = (discriminant > 0.0f) ? rayDir * sqrt(discriminant) : -rayDir;
        
        // Iteration
        m1[0][0] = vec.x;
    }

    BufferOut[dispatchThreadID.x] = vec.x + vec.y + vec.z;
}

// ---------------------------------------------------------
// 6. Massive VRAM Filler Kernel
// Triggers across hundreds of millions of elements. 
// Uses a much lower loop count per-thread to avoid Windows TDR device reset,
// because the total number of running threads is 14x higher.
// ---------------------------------------------------------
[numthreads(256, 1, 1)]
void CSMemoryMassive(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    uint maxElements = 1024 * 1024 * 16 * 14; 
    
    // Simulate memory thrashing by jumping around the buffer
    // Lower iteration count (35) to balance the 14x increase in threads
    for (int i = 0; i < 35; ++i)
    {
        uint jumpIndex = (id + (i * 104729)) % maxElements; 
        float data = BufferOut[jumpIndex] + BufferB[jumpIndex];
        
        BufferOut[id] = data * 0.5f;
        BufferB[jumpIndex] = data * 0.1f;
    }
}

// ---------------------------------------------------------
// 7. GPU Particle Physics (N-Body Simulation)
// Simulates gravitational pull among thousands of particles. O(N^2) complexity.
// ---------------------------------------------------------
[numthreads(256, 1, 1)]
void CSParticles(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    uint maxElements = 1024 * 1024 * 16;
    
    float3 pos = float3(BufferOut[id], BufferB[id], BufferOut[(id + 1) % maxElements]);
    float3 vel = float3(0.0f, 0.0f, 0.0f);
    
    // N-Body gravity loop - ALU heavy O(N^2) pseudo-simulation
    for (int i = 0; i < 200; ++i)
    {
        uint otherId = (id + i * 16) % maxElements;
        float3 otherPos = float3(BufferOut[otherId], BufferB[otherId], BufferOut[(otherId+1) % maxElements]);
        
        float3 diff = otherPos - pos;
        float distSqr = dot(diff, diff) + 0.1f;
        float invDist = rsqrt(distSqr);
        float force = invDist * invDist * invDist;
        
        vel += diff * force * 0.001f;
    }
    
    pos += vel * 0.016f;
    
    BufferOut[id] = pos.x;
    BufferB[id] = pos.y;
}

// ---------------------------------------------------------
// 8. Cache Coherency Benchmark (SoA vs AoS Pattern)
// Sequential (SoA) access hits L1 cache perfectly.
// Stride/Random (AoS) access causes L1 misses and memory stalls.
// ---------------------------------------------------------
[numthreads(256, 1, 1)]
void CSCache(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    uint maxElements = 1024 * 1024 * 16;
    float result = 0.0f;
    
    // SoA / Sequential Loop (Good Cache Hit Rate)
    for (int i = 0; i < 150; ++i)
    {
        result += BufferOut[(id + i) % maxElements];
    }
    
    // AoS / Stride Loop (Cache Thrashing / L1 Misses)
    for (int j = 0; j < 150; ++j)
    {
        uint badIndex = (id + j * 104729) % maxElements;
        result *= BufferB[badIndex] * 0.1f;
    }
    
    BufferOut[id] = result;
}

// ---------------------------------------------------------
// 9. Deferred Rendering G-Buffer Simulation
// Emulates a modern deferred shading pipeline by writing to 4 UAVs simultaneously.
// Heavily taxes PCIe and VRAM write bandwidth.
// ---------------------------------------------------------
[numthreads(256, 1, 1)]
void CSDeferred(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    uint maxElements = 1024 * 1024 * 16;
    
    // Write out Albedo, Normal, Material, and Depth to 4 separate buffers
    for (int i = 0; i < 60; ++i)
    {
        uint idx = (id + i * 256) % maxElements;
        float tex = BufferOut[idx];
        
        // 1. Albedo / Color
        BufferOut[idx]  = tex * 0.9f + 0.1f;
        // 2. Normal (Simulated xyz encoding)
        BufferB[idx]    = sin(tex) * 0.5f + 0.5f;
        // 3. Material (Roughness, Metal, AO)
        BufferC[idx]    = cos(tex) * 0.5f + 0.5f;
        // 4. Depth map
        BufferD[idx]    = frac(tex * 123.456f);
    }
}

// ---------------------------------------------------------
// 10. Open-World Volumetric Ray-Marching (e.g. Red Dead Redemption 2 Sim)
// Simulates rendering of massive open worlds with volumetric clouds
// and heavy atmospheric scattering using ray-marching math.
// ---------------------------------------------------------
[numthreads(256, 1, 1)]
void CSRedDead2(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    float3 rayPos = float3(id * 0.001f, 0.0f, id * 0.002f);
    float3 rayDir = normalize(float3(sin(id*0.1f), 1.0f, cos(id*0.1f)));
    
    float totalDensity = 0.0f;
    float transmittance = 1.0f;
    
    // Ray-marching loop (150 steps) to simulate cloud volume sampling
    for(int i = 0; i < 150; ++i)
    {
        rayPos += rayDir * 0.5f; // Step forward
        
        // Simulating 3D noise sample using trig functions
        float noiseVal = sin(rayPos.x * 12.3f) * cos(rayPos.y * 45.6f) * sin(rayPos.z * 78.9f);
        noiseVal = max(0.0f, noiseVal * 0.5f + 0.5f - 0.3f); // Thresholding
        
        // Accumulate density
        totalDensity += noiseVal * transmittance;
        transmittance *= exp(-noiseVal * 0.1f); // Beer's Law attenuation
        
        // Atmospheric scattering fake math
        rayDir = normalize(rayDir + float3(sin(totalDensity), cos(totalDensity), 0.0f) * 0.01f);
    }
    
    BufferOut[id] = totalDensity;
}

// ---------------------------------------------------------
// 11. Dense Isometric Grid ARPG (e.g. Diablo 4 Sim)
// Simulates overhead camera rendering with thousands of small
// visible entities (minions, particles, loot) triggering 
// cache-unfriendly scattered memory reads and shadow calculations.
// ---------------------------------------------------------
[numthreads(256, 1, 1)]
void CSDiablo4(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    uint maxElements = 1024 * 1024 * 16;
    
    float pixelLighting = 0.0f;
    
    // Simulate looking up properties of dozens of overlapping entities per pixel
    for(int i = 0; i < 80; ++i)
    {
        // Entity ID lookup (pseudo-random scatter)
        uint entityId = (id + (i * 1337)) % maxElements;
        
        // Fetch base color and normal from BufferOut and BufferB
        float baseColor = BufferOut[entityId];
        float normalVal = BufferB[entityId];
        
        // Simulate shadow map cascade lookup (another scatter read)
        uint shadowIndex = (id + (uint)(baseColor * 10000.0f)) % maxElements;
        float shadowDepth = BufferB[shadowIndex];
        
        // Calculate point light contribution
        float lightDist = abs(baseColor - normalVal);
        float attenuation = 1.0f / (1.0f + lightDist * lightDist);
        float inShadow = (shadowDepth > 0.5f) ? 0.2f : 1.0f;
        
        pixelLighting += attenuation * inShadow;
    }
    
    BufferOut[id] = pixelLighting;
}
