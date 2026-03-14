# GPU Stress Tester v2 (DirectX 11)

An advanced, native C++ GPU profiling and stress-testing application engineered to push modern graphics hardware to its absolute limits. Utilizing Direct3D 11 Compute Shaders (`CS_5_0`), this tool features 11 highly specialized workloads that simulate everything from extreme ALU compute scenarios and massive VRAM throughput to complex AAA game engine environments (like Red Dead Redemption 2 and Diablo 4).

## 🚀 Key Features

- **DirectX 11 Compute Shaders:** Bypasses traditional graphics pipelines to hammer the GPU's Compute Units directly, maximizing thermal and power draw limits.
- **11 Modular Workloads:** Includes dedicated kernels for ALU Math, VRAM Bandwidth Thrashing, Crypto Hashing, Particle Physics (N-Body), and Ray Tracing Tensor Math.
- **AAA Game Simulations:** Simulates the hardware bottlenecks of specific rendering architectures (e.g., Deferred G-Buffers, RPG Isometric grids, Open-World Volumetric Ray-Marching).
- **Automated Benchmarking:** Sequential execution suite that runs all 11 kernels and logs performance data (Telemetry) to a CSV file (`benchmark_results.csv`) with automatic Average FPS calculations.
- **Universal Deployment:** Shaders are pre-compiled offline (`.cso`) using Microsoft `fxc.exe`, completely removing the need for runtime D3DCompiler dependencies on the host machine.
- **Hardware Telemetry:** Auto-detects installed DXGI Display Adapters and native VRAM capacities, allowing targeted stress testing on Dual-GPU laptops (IGP vs Dedicated).

## 🛡️ AMD Adrenalin & Hardware Heuristics Bypass

This project incorporates heavy reverse-engineering of Windows DWM (Desktop Window Manager) and GPU Driver Heuristics (specifically AMD Adrenalin and Xbox Game Bar) to force them into recognizing a headless Compute application as a "True 3D Game". 

The architecture includes the following active bypasses:
1. **True Borderless Fullscreen Promotion:** Dynamically polls `GetSystemMetrics` to match the exact physical display resolution, forcing Windows DWM into Independent Flip (`DXGI_SWAP_EFFECT_FLIP_DISCARD`) mode instead of Windowed Composition.
2. **Input Assembler (IA) Geometric Profiling:** Submits 300,000 degenerate vertex primitives per frame (`Draw(300000, 0)`) to the Rasterizer. This guarantees the driver logs massive geometric activity, bypassing "2D UI Application" filters that ignore apps like Chrome or Discord.
3. **Dynamic Uniform Streaming:** Instantiates a `D3D11_USAGE_DYNAMIC` Constant Buffer that is mapped/unmapped every single frame, fulfilling the heuristic requirement that a real 3D camera provides a moving View/Projection matrix.
4. **WDDM Thread Pacing (Spinlock Prevention):** Implements a sub-millisecond `Sleep(1)` and rigourous `MsgWaitForMultipleObjects` Native Event Pump inside the core render loop. This mimics the precise thread pacing of an AAA Game Engine, assuring Windows the CPU thread hasn't locked the OS (preventing "Not Responding" tags that disable overlay telemetry).
5. **Depth/Stencil Signature:** Binds a complete `D3D11_DEPTH_STENCIL_DESC` with an active Z-Buffer (`DepthEnable = TRUE`) to the RenderTarget, acting as the final cryptographic handshake to the GPU that a spatial 3D environment is active.
6. **Subsystem Whitewashing:** Renamed binary execution target to `Game.exe` bypassing literal executable string filters.

## 🛠️ Technology Stack

- **Core Language:** C++ (Native Win32 API)
- **Graphics API:** DirectX 11 (`D3D11`, `DXGI 1.1 / 1.2`)
- **Shading Language:** HLSL (High-Level Shader Language)
- **Compiler/Toolchain:** Microsoft Visual Studio C++ Compiler (`cl.exe`), DirectX Shader Compiler (`fxc.exe`)

## ⚙️ Building and Execution

Ensure you are running from a **"x64 Native Tools Command Prompt for VS 202* "** environment before compilation.

1. **Compile the Shaders and C++ Runtime:**
   Simply execute the provided batch script. This will compile all `.hlsl` files into `.cso` objects, and link `main.cpp`.
   ```bat
   ./build.bat
   ```
2. **Run the Stress Tester:**
   *(Note: Target executable must retain a 'game-like' name to trigger certain graphics overlays).*
   ```bat
   ./Game.exe
   ```

## 📝 Performance Logging

The application streams live FPS telemetry to the console host and automatically persists results inside `benchmark_results.csv`, structured for downstream Python Pandas / Excel visualization. 

*Caution: Running workloads like `#1 (ALU Math)` or `#6 (Massive VRAM)` will immediately push modern GPUs to their maximum TGP and Junction Temperatures. Ensure adequate system cooling before initiating 0-duration (infinite) tests.*
