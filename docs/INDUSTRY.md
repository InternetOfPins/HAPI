## What is HAPI?

**HAPI (Hardware Abstraction Pipeline Interface)** is an open-source, MIT-licensed framework written in modern C++17. It allows developers to design hardware architectures and pipelines using functional programming and type-level composition instead of being forced to rely solely on traditional Hardware Description Languages (like VHDL or Verilog).

By using advanced template metaprogramming, HAPI constructs data pipelines entirely at compile time. The host compiler flattens the abstract type hierarchy, generating ultra-optimized code that writes directly to absolute memory-mapped I/O (MMIO) hardware addresses with **zero runtime overhead** and **zero RAM waste**.

---

## Why does it matter to the industry?

HAPI directly addresses three critical pain points in modern embedded systems and hardware engineering:

### 1. Breaking Vendor Lock-In

The semiconductor industry heavily relies on bloated, expensive, and closed-source proprietary IDEs from specific silicon vendors to program hardware targets. HAPI brings control back to the open-source ecosystem. Because it is standard C++17, any mainstream compiler frontend (like GCC or Clang) can parse HAPI’s type tree to generate an open, deterministic Abstract Syntax Tree (AST). External FOSS synthesis tools can then ingest this AST to target open CPLD/FPGA architectures.

### 2. Real "Zero-Cost Abstraction" for Hardware

In industrial electronics, every single byte of RAM and clock cycle matters. Traditional object-oriented abstractions introduce runtime pointers, dynamic allocations, and virtual tables (vtables) that waste critical resources. HAPI eliminates this completely. The hardware's physical address is injected directly into the compiler's type system. The resulting assembly translates to absolute memory writes, bypassing runtime indirection entirely.

### 3. Unified PC-to-Silicon Workflows

HAPI allows the exact same logical pipeline designed for a physical chip to be simulated, unit-tested, and validated natively on a host PC using modern CI/CD software pipelines. Bridging the gap between software development practices and hardware synthesis radically reduces validation costs and shortens time-to-market for embedded infrastructure.

## Targeted Industry Domains & Impact

### 1. Automotive & ADAS (Advanced Driver Assistance Systems)
* **The Problem:** Processing sensor data streams (LiDAR, Radar) requires deterministic, real-time pipelines. Current C++ frameworks introduce safety-critical runtime risks.
* **HAPI Impact:** Eliminates runtime allocations and dynamic dispatch. Pipelines compile to deterministic, static hardware writes, matching strict safety compliance boundaries.

### 2. Aerospace & Defense (Rad-Hardened CPLDs/FPGAs)
* **The Problem:** Radiation-hardened chips have highly constrained logic cells and memory. Bloated vendor-specific IP blocks waste silicon space.
* **HAPI Impact:** The zero-RAM footprint ensures pipeline logic maps efficiently to the smallest possible silicon area, maximizing available space on expensive hardware.

### 3. Industrial IoT & Edge Gateways
* **The Problem:** Edge devices must process high-frequency signals at the hardware level while maintaining ultra-low power consumption.
* **HAPI Impact:** Direct MMIO synthesis bypasses heavy runtimes and OS overhead, deploying straight-to-silicon pipelines that slash power draw per compute cycle.

### 4. Automated Test Equipment (ATE) & Lab Instrumentation
* **The Problem:** Custom signal generation boards require frequent re-configuration; changing traditional HDL code breaks test benches and slows verification.
* **HAPI Impact:** Allows engineers to write structural hardware blocks in native C++, validating configurations via standard PC-based software test frameworks before synthesis.