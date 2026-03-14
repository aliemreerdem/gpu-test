---
name: cpp-3d-game-mastery
description: >
  Consolidated technical knowledge base from the world's greatest C++ 3D game
  programmers: Carmack (id Software), Sweeney (Unreal Engine), Abrash (Quake/VR),
  Silverman (Build Engine), Newell (Source Engine), and Yerli (CryEngine).
  Covers 3D engine architecture, render pipelines, memory/performance optimization,
  physics/collision, spatial data structures, open-world streaming, multiplayer
  networking, and modern GPU (Vulkan/DX12) programming.
  Trigger this skill when the user asks about: writing or understanding a 3D engine,
  BSP/ray casting/shadow volumes/GI/deferred rendering, C++ game optimization,
  ECS architecture, collision detection, LOD/MegaTexture/streaming, client-side
  prediction/lag compensation, Vulkan or DirectX 12 pipeline design, or any
  code review involving real-time 3D graphics in C++.
compatibility: "claude-code, antigravity, cursor, windsurf, gemini-cli"
metadata:
  version: "1.0.0"
  author: "BPN / A.Emre"
  tags: [cpp, 3d-graphics, game-engine, rendering, performance, physics, vulkan]
  install:
    claude-code: "~/.claude/skills/cpp-3d-game-mastery/"
    antigravity:  "~/.gemini/antigravity/skills/cpp-3d-game-mastery/"
    workspace:    ".agent/skills/cpp-3d-game-mastery/"
---

# C++ 3D Game Mastery

> Distilled wisdom of Carmack, Sweeney, Abrash, Silverman, Newell, Yerli and peers.
> Use `@cpp-3d-game-mastery` or `/cpp-3d-game-mastery` to invoke.

## Quick Reference

| Section | Topics |
|---------|--------|
| [1. Engine Architecture](#1-engine-architecture) | Core loop, ECS, fixed timestep |
| [2. Rendering Techniques](#2-rendering-techniques) | BSP, shadow volumes, deferred, GI |
| [3. Memory & Performance](#3-memory--performance) | Cache, SIMD, allocators, profiling |
| [4. Physics & Collision](#4-physics--collision) | Broad/narrow phase, constraint solver |
| [5. Spatial Data Structures](#5-spatial-data-structures) | BVH, octree, portals |
| [6. Open World & Streaming](#6-open-world--streaming) | LOD, MegaTexture, virtual texturing |
| [7. Networking & Multiplayer](#7-networking--multiplayer) | Prediction, reconciliation, lag comp |
| [8. Modern GPU Pipeline](#8-modern-gpu-pipeline) | Vulkan/DX12, PSO, compute shaders |

---

## 1. Engine Architecture

### Carmack Rule: Lean Core Loop

```cpp
// Fixed physics timestep + variable render (Quake 3 pattern)
void Engine::Run() {
    uint64_t lastTime = Platform::GetMicroseconds();
    while (!shouldQuit) {
        uint64_t now      = Platform::GetMicroseconds();
        float    deltaMs  = (now - lastTime) / 1000.0f;
        lastTime = now;

        physicsAccumulator += deltaMs;
        while (physicsAccumulator >= PHYSICS_TIMESTEP_MS) {
            world.StepPhysics(PHYSICS_TIMESTEP_MS / 1000.0f);
            physicsAccumulator -= PHYSICS_TIMESTEP_MS;
        }

        float alpha = physicsAccumulator / PHYSICS_TIMESTEP_MS; // interpolation
        input.Poll();
        world.Update(deltaMs / 1000.0f);
        renderer.RenderFrame(world, alpha);
    }
}
```

**Rule:** Never mix physics tick with render tick. Physics = deterministic fixed step.

### Data-Oriented ECS (Sweeney / UE4+)

```cpp
// SOA layout — cache-friendly sequential access
struct TransformArray {
    float posX[MAX_ENTITIES], posY[MAX_ENTITIES], posZ[MAX_ENTITIES];
    float rotQuat[MAX_ENTITIES][4];
    // NOT AOS (Array of Structs) — kills cache line utilization
};

void RenderSystem::Update(TransformArray& t, RenderableArray& r, uint32_t n) {
    for (uint32_t i = 0; i < n; ++i)
        DrawMesh(r.meshId[i], BuildMatrix(t, i)); // sequential → L1 hit
}
```

**Abrash Rule:** Profile before optimizing. Never assume the bottleneck.

---

## 2. Rendering Techniques

### BSP Tree (Carmack / id Software — Doom, Quake)

```cpp
struct BSPNode {
    Plane splitPlane;
    int   frontChild; // LEAF_FLAG | leafIndex if leaf
    int   backChild;
    AABB  bounds;
};

void BSPTree::TraverseFrontToBack(int node, const Vec3& cam,
                                   std::vector<int>& leafOrder) {
    if (node < 0) { leafOrder.push_back(node & ~LEAF_FLAG); return; }
    float d = nodes[node].splitPlane.DistanceTo(cam);
    int first  = d >= 0.f ? nodes[node].frontChild : nodes[node].backChild;
    int second = d >= 0.f ? nodes[node].backChild  : nodes[node].frontChild;
    TraverseFrontToBack(first,  cam, leafOrder);
    TraverseFrontToBack(second, cam, leafOrder);
}
```

**Use when:** Static indoor geometry + PVS (Potentially Visible Set).
**Skip when:** Dynamic open world — use BVH or octree instead.

### Shadow Volumes — Carmack's Reverse (z-fail)

```cpp
// Find silhouette edges: faces with opposite light-facing
void FindSilhouetteEdges(const Mesh& m, const Vec3& lp,
                          std::vector<Edge>& out) {
    for (auto& e : m.edges) {
        bool f0 = Dot(m.faces[e.face0].normal, lp - m.faces[e.face0].centroid) > 0;
        bool f1 = Dot(m.faces[e.face1].normal, lp - m.faces[e.face1].centroid) > 0;
        if (f0 != f1) out.push_back(e);
    }
}

void RenderShadowVolume(const std::vector<Edge>& edges, const Vec3& lp) {
    // z-fail: works even when camera is inside the shadow volume
    glStencilOpSeparate(GL_BACK,  GL_KEEP, GL_INCR_WRAP, GL_KEEP);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
    for (auto& e : edges) {
        Vec3 v0f = e.v0 + Normalize(e.v0 - lp) * EXTRUDE;
        Vec3 v1f = e.v1 + Normalize(e.v1 - lp) * EXTRUDE;
        DrawQuad(e.v0, e.v1, v1f, v0f);
    }
    // Also draw front + far caps
}
```

### Deferred Rendering G-Buffer (CryEngine / UE4+)

```cpp
struct GBuffer {
    Texture2D albedo;       // RGB = color, A = metallic
    Texture2D normal;       // Oct32-encoded world-space normal
    Texture2D materialData; // R = roughness, G = AO, B = emissive
    Texture2D depth;        // 32-bit float
};

// Lighting pass: reconstruct world position from depth, evaluate BRDF
vec3 EvaluateLighting(GBuffer g, vec2 uv, vec3 camPos) {
    vec3  N   = DecodeOct(g.normal.Sample(uv).rg);
    float rgh = g.materialData.Sample(uv).r;
    vec3  wp  = ReconstructWorldPos(uv, g.depth.Sample(uv).r);
    return CookTorranceBRDF(g.albedo.Sample(uv).rgb,
                             g.albedo.Sample(uv).a,
                             rgh, N, Normalize(camPos - wp), lightDir);
}
```

### Lumen-Style Global Illumination (Sweeney / UE5)

```
Hierarchy (near → far):
  Screen Space GI  →  fast, AO + bent normal
  Surface Cache    →  SDF ray march, radiance probe grid
  Far Field        →  low-res global irradiance estimate

Steps:
  1. Offline/runtime: build SDF per mesh
  2. Per frame: shoot rays against SDF hierarchy
  3. Update surface cache (radiance probes)
  4. Interpolate probes → final GI
```

---

## 3. Memory & Performance

### Abrash's Five Laws

```
1. Measure first — your intuition about the bottleneck is usually wrong
2. Cache miss > instruction count (by 10-100×)
3. Predictable access patterns → branch predictor happy
4. SIMD: process 4–8 lanes simultaneously, not one at a time
5. Hot path: zero heap allocation — use pool or stack/frame allocator
```

### Frame Allocator (zero fragmentation)

```cpp
class FrameAllocator {
    uint8_t* base; size_t offset, capacity, highWater;
public:
    void* Alloc(size_t size, size_t align = 16) {
        size_t p = (offset + align - 1) & ~(align - 1);
        assert(p + size <= capacity);
        offset    = p + size;
        highWater = std::max(highWater, offset); // always track peak
        return base + p;
    }
    void Reset() { offset = 0; } // O(1) — no fragmentation ever
};
```

### SIMD Batch Frustum Cull (AVX2 — Abrash / Larrabee lineage)

```cpp
#include <immintrin.h>
// Returns bitmask: bit i = 1 → object i is visible
uint8_t CullBatch8(const __m256 px, const __m256 py,
                    const __m256 pz, const Frustum& f) {
    uint8_t vis = 0xFF;
    for (int p = 0; p < 6; ++p) {
        __m256 dot = _mm256_fmadd_ps(_mm256_set1_ps(f.planes[p].z), pz,
                     _mm256_fmadd_ps(_mm256_set1_ps(f.planes[p].y), py,
                     _mm256_fmadd_ps(_mm256_set1_ps(f.planes[p].x), px,
                                     _mm256_set1_ps(f.planes[p].w))));
        vis &= ~(uint8_t)_mm256_movemask_ps(
            _mm256_cmp_ps(dot, _mm256_setzero_ps(), _CMP_LT_OQ));
    }
    return vis;
}
```

---

## 4. Physics & Collision

### Broad Phase — Sweep and Prune

```cpp
class SweepAndPrune {
    struct EP { float v; uint32_t id; bool isMin; };
    std::vector<EP> axis;
public:
    void FindOverlaps(std::vector<CollisionPair>& out) {
        std::sort(axis.begin(), axis.end(), [](auto& a, auto& b){ return a.v < b.v; });
        std::unordered_set<uint32_t> active;
        for (auto& ep : axis) {
            if (ep.isMin) {
                for (uint32_t id : active) out.push_back({ep.id, id});
                active.insert(ep.id);
            } else active.erase(ep.id);
        }
    }
};
```

### Position-Based Constraint Solver (Verlet / Havok-style)

```cpp
void SolveDistance(RigidBody& a, RigidBody& b,
                   float rest, float stiffness, int iter, int maxIter) {
    Vec3  d    = b.position - a.position;
    float dist = Length(d); if (dist < 1e-6f) return;
    float corr = (dist - rest) / dist;
    float w    = 1.f / (a.invMass + b.invMass);
    float sor  = stiffness * (1.f - powf(1.f - stiffness, (float)(maxIter - iter)));
    a.position += d * corr * a.invMass * w * sor;
    b.position -= d * corr * b.invMass * w * sor;
}
```

---

## 5. Spatial Data Structures

### BVH (Bounding Volume Hierarchy)

```
Build (SAH — Surface Area Heuristic):
  1. Find split axis/plane minimizing SAH cost: C = Ct + (SA_L/SA_P)·NL·Ci + (SA_R/SA_P)·NR·Ci
  2. Recurse until leaf: N < 4 triangles
  3. Alternatives: LBVH (Morton-coded, GPU-friendly) for dynamic scenes

Traversal:
  - Stack-based DFS; test near child first (early exit)
  - SIMD 4-wide AABB test for inner nodes
  - Leaf: triangle intersection

Update strategies:
  - Static:  build once offline
  - Dynamic: refit AABB (fast) + periodic SAH rebuild
```

### Portal + PVS (Carmack / Quake indoor scenes)

```
Concepts:
  Leaf  = convex cell (room, corridor segment)
  Portal = shared face between two leaves
  PVS   = precomputed bitset: which leaves are visible from leaf L

Runtime:
  1. Find camera's leaf via BSP
  2. Fetch PVS for that leaf
  3. Only submit geometry in visible leaves → massive draw call reduction
```

---

## 6. Open World & Streaming

### Level of Detail (LOD) Selection

```cpp
int SelectLOD(const BoundingSphere& s, const Camera& cam, float threshold) {
    float screenFrac = s.radius / (Distance(s.center, cam.position)
                                   * tanf(cam.fovY * 0.5f));
    if      (screenFrac > threshold)         return 0;  // full detail
    else if (screenFrac > threshold * 0.25f) return 1;
    else if (screenFrac > threshold * 0.06f) return 2;
    else                                      return 3;  // billboard / imposter
}
```

### MegaTexture / Virtual Texturing (Carmack — Quake Wars)

```
Concept:
  Single huge virtual texture (e.g. 128K×128K) covers entire terrain.
  Only resident tiles are in GPU memory — page table maps virtual → physical.

Pipeline:
  1. Feedback pass  : fragment shader writes (mip, tile_x, tile_y) to buffer
  2. CPU readback   : determine which tiles are needed
  3. Async streaming: disk → CPU → GPU upload queue
  4. Page table tex : updated each frame; shader does extra lookup
  5. Shader         : virtual UV → page table → physical UV → sample
```

---

## 7. Networking & Multiplayer

### Client-Side Prediction + Server Reconciliation (Abrash / Valve)

```
Algorithm:
  1. Client applies input immediately (optimistic update)
  2. Store (input, client_tick) in circular buffer
  3. Server receives input, simulates authoritatively, sends state + acked_tick
  4. Client on receive:
       a. Rewind to acked_tick state
       b. Re-simulate all inputs since acked_tick
       c. |delta| < threshold → smooth lerp; else hard snap

Lag Compensation (Counter-Strike / Valve):
  Server stores per-tick snapshot history (N frames).
  On hit validation: rewind server state to client's send tick,
  then perform hit test against historical positions.
```

### Rollback Netcode (fighting game standard, applicable to fast-action games)

```
vs. Delay-based:
  Delay-based: both sides wait for input → feels sluggish on high ping
  Rollback   : predict opponent input → simulate → if wrong, roll back & re-sim

Key requirement: fully deterministic simulation
  - Fixed timestep physics (see §1)
  - No floating-point divergence (same CPU ISA, or lockstep FP)
  - Save/restore complete game state (snapshot) every frame
```

---

## 8. Modern GPU Pipeline

### Vulkan / DX12 Mental Model (Sweeney / UE5)

```
Old (GL/DX11) — hidden driver overhead:
  Lazy shader compile, implicit hazard tracking, single-thread submission

New (Vk/DX12) — explicit, multi-threaded:
  PSO (Pipeline State Object)  : compiled ahead, no stalls at draw time
  Descriptor Set / Root Sig    : explicit shader resource binding
  Command Buffer               : recorded in parallel on N threads
  Render Pass                  : declare attachments, load/store ops upfront
  Barriers                     : YOU declare all layout transitions & hazards
  Fence / Semaphore             : CPU↔GPU and GPU↔GPU synchronization

Frame loop:
  vkAcquireNextImageKHR  →  record cmd bufs (parallel)  →
  vkQueueSubmit          →  vkQueuePresentKHR
```

### Compute Shader — GPU Particle System

```glsl
layout(local_size_x = 256) in;
layout(std430, binding = 0) buffer Particles {
    vec4 pos[];  // xyz=position, w=lifetime
    vec4 vel[];  // xyz=velocity, w=mass
};
uniform float deltaTime;
uniform vec3  gravity;

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= pos.length()) return;
    vel[i].xyz += gravity * deltaTime;
    pos[i].xyz += vel[i].xyz * deltaTime;
    pos[i].w   -= deltaTime; // age out
}
```

### Bindless Resources (modern AAA pattern)

```cpp
// Bind textures once to a large descriptor heap — no per-draw rebinding
// DX12:
CD3DX12_DESCRIPTOR_RANGE1 range;
range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, NUM_TEXTURES, 0, 0,
           D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
// Shader: Texture2D allTextures[] : register(t0, space0);
// Access: allTextures[NonUniformResourceIndex(texId)].Sample(...)

// Vulkan equivalent: VK_EXT_descriptor_indexing
// layout(set=0, binding=0) uniform sampler2D textures[];
```

---

## Decision Guide

| Situation | Carmack Approach | Sweeney Approach |
|-----------|-----------------|------------------|
| New feature | Ship it, refactor later | Architect first |
| Perf issue | Profile → one bottleneck | Systemic LOD + streaming |
| Rendering | Minimal state, explicit | Layered material abstraction |
| Memory | Frame/pool allocator | Unified virtual memory |
| Multiplayer | Server-authoritative, simple | Full prediction + replay |
| Open World | MegaTexture, stream aggressively | Nanite-style micro-mesh |

---

## Reference Library

| Source | Author | Scope |
|--------|--------|-------|
| Graphics Programming Black Book | Michael Abrash | x86 opt, Quake internals |
| Game Engine Architecture (3rd ed) | Jason Gregory | Large-scale engine design |
| Real-Time Rendering (4th ed) | Akenine-Möller et al. | Modern pipeline bible |
| Physically Based Rendering (4th ed) | Pharr, Jakob, Humphreys | PBR / path tracing |
| Physics for Game Developers | Bourg & Bywalec | Physics implementation |
| github.com/id-Software/Quake | id Software | Carmack's original code |
| github.com/EpicGames/UnrealEngine | Epic Games | Sweeney's living legacy |

---

## Installation

**Claude Code (global):**
```bash
mkdir -p ~/.claude/skills/cpp-3d-game-mastery
cp SKILL.md ~/.claude/skills/cpp-3d-game-mastery/
```

**Antigravity (global):**
```bash
mkdir -p ~/.gemini/antigravity/skills/cpp-3d-game-mastery
cp SKILL.md ~/.gemini/antigravity/skills/cpp-3d-game-mastery/
```

**Workspace (project-local, both tools):**
```bash
mkdir -p .agent/skills/cpp-3d-game-mastery
cp SKILL.md .agent/skills/cpp-3d-game-mastery/
```

**Invocation:**
```
Claude Code : @cpp-3d-game-mastery <task>
Antigravity : /cpp-3d-game-mastery <task>
```
