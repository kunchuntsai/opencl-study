# OpenCL Image Processing Framework - Slide Plan Part II

> **Presentation Duration**: 30 minutes
> **Objective**: We take care of the host, so users can focus on kernel development

---

## Slide Overview (Total: 12 slides)

| Section | Slides | Time |
|---------|--------|------|
| 1. Introduction / Recap | 1-2 | 4 min |
| 2. Why Two-Phase Architecture | 3-4 | 6 min |
| 3. Compilation Phase | 5-6 | 6 min |
| 4. Execution Phase | 7-8 | 6 min |
| 5. Cross-Platform Deployment | 9-10 | 5 min |
| 6. Summary & Q&A | 11-12 | 3 min |

---

## Section 1: Introduction / Recap

### Slide 1: Title Slide
**Title**: OpenCL Two-Phase Architecture
**Subtitle**: Compile Once, Run Anywhere

**Content**:
- Framework logo/visual
- Presenter name and date
- Tagline: "We handle the host, you focus on kernels"

**Speaker Notes**:
Welcome to Part II. In Part I we learned how to use the framework. Now we'll look at how we split compilation from execution for better deployment.

---

### Slide 2: Part I Recap
**Title**: What We Covered in Part I

**Visual**: Display `svg/202_part1_recap.svg`

**Content**:

| Topic | Key Points |
|-------|------------|
| Framework Benefits | Zero host code, JSON-driven |
| User Journey | Build → Run → Learn |
| Platform Options | Standard CL vs CL Extension |
| JSON Configuration | Kernel config, I/O setup |

**Today's Focus**: How we split the host into two phases

**Speaker Notes**:
Quick recap of Part I. Today we dive deeper into the architecture - specifically how we separate compilation from execution.

---

## Section 2: Why Two-Phase Architecture

### Slide 3: The Problem with Monolithic Flow
**Title**: Current Architecture Limitations

**Visual**: Display `svg/203_monolithic_flow.svg`

**Pain Points**:
- Every run recompiles kernel (~100ms overhead)
- Source code must be present on target
- Can't pre-compile for different devices
- No binary distribution possible

**Speaker Notes**:
The current monolithic flow compiles the kernel every time you run. That's 100ms overhead per run. We want to eliminate this for production.

---

### Slide 4: The Solution - Two Phases
**Title**: Split Into Compilation + Execution

**Visual**: Display `svg/204_two_phase_split.svg`

**Two-Phase Architecture**:
```
[Compilation] (Linux)
  Input:  kernel.cl, config.json
  Output: kernel.bin, opencl_host (android)

[Execution] (Linux/Android)
  Input:  kernel.bin, config.json
  Output: result.bin
```

**Key Benefit**: Compile once, run many times

**Speaker Notes**:
The solution is to split into two phases. Compile once on Linux, then execute on any platform using the binaries.

---

## Section 3: Compilation Phase

### Slide 5: Compilation Phase Overview
**Title**: Phase 1 - Binary Generation

**Visual**: Display `svg/205_compile_execute_split.svg` (left panel)

**Purpose**: Convert source + config into deployment-ready binaries

**Inputs**:
- `kernel.cl` - OpenCL source code
- `config.json` - Kernel configuration

**Outputs**:
- `kernel.bin` - Compiled kernel binary
- `opencl_host` - Android executable (when cross-compiling)

**Speaker Notes**:
The compilation phase runs on Linux. It takes your kernel source and config, produces binaries ready for deployment.

---

### Slide 6: Compilation Phase I/O Details
**Title**: What Goes Into the Binaries

**Visual**: Display `svg/206_phase_io_details.svg` (compilation section)

**kernel.bin contains**:
- Compiled kernel binary (device-specific)
- Metadata (device name, OpenCL version)
- Compile options used

**Note**: Binary is device-specific (GPU vendor + model)

**Speaker Notes**:
The kernel binary contains pre-compiled GPU code. It's device-specific - you need to compile for each target GPU type.

---

## Section 4: Execution Phase

### Slide 7: Execution Phase Overview
**Title**: Phase 2 - Fast Execution

**Visual**: Display `svg/205_compile_execute_split.svg` (right panel)

**Purpose**: Run kernel without compilation overhead

**Inputs**:
- `kernel.bin` - From compilation phase
- `config.json` - Runtime configuration
- Input data

**Output**:
- `result.bin` - Processed output

**Key**: Uses `clCreateProgramWithBinary()` - instant kernel load

**Speaker Notes**:
The execution phase loads pre-compiled binaries. No JIT compilation means near-instant kernel startup.

---

### Slide 8: Execution Phase I/O Details
**Title**: Execution Flow

**Visual**: Display `svg/206_phase_io_details.svg` (execution section)

**Execution Steps**:
1. Initialize OpenCL (platform, device, context)
2. Load kernel.bin with `clCreateProgramWithBinary()`
3. Parse config.json for work sizes and arguments
4. Execute kernel and retrieve results

**Performance**: ~5ms vs ~105ms (with compilation)

**Speaker Notes**:
Loading a pre-compiled binary is nearly instant. This eliminates the 100ms compilation overhead on every run.

---

## Section 5: Cross-Platform Deployment

### Slide 9: Linux vs Android Deployment
**Title**: Platform-Specific Deployment

**Visual**: Display `svg/209_cross_platform.svg`

**Content**:

| Platform | opencl_host | kernel.bin | config.json |
|----------|-------------|------------|-------------|
| Linux | Same as compilation | ✓ | ✓ |
| Android | Cross-compiled for ARM | ✓ | ✓ |

**Linux Workflow**:
```
./opencl_host <algo> <variant>  # Uses local kernel.bin
```

**Android Workflow**:
```
adb push opencl_host kernel.bin config.json /data/local/tmp/
adb shell /data/local/tmp/opencl_host <algo> <variant>
```

**Speaker Notes**:
For Linux, use the same executable from compilation. For Android, cross-compile the host executable and push binaries to the device.

---

### Slide 10: Deployment Package
**Title**: What to Ship

**Visual**: Display `svg/210_deployment_package.svg`

**Content**:

**Production Package**:
```
dist/
├── opencl_host    # Executor binary
├── kernel.bin     # Compiled kernel
└── config.json    # Runtime config
```

**NOT required in production**:
- ~~kernel.cl~~ (source code)
- ~~include/~~ (headers)
- ~~build tools~~

**Benefits**: Smaller footprint, no source exposure, faster startup

**Speaker Notes**:
The production package is minimal. Just the executor, binary, and config. No source code needed on target devices.

---

## Section 6: Summary & Q&A

### Slide 11: Summary
**Title**: Key Takeaways

**Content**:

| Phase | When | Where | Output |
|-------|------|-------|--------|
| Compilation | Once | Linux | kernel.bin + opencl_host |
| Execution | Many times | Linux/Android | result.bin |

**Benefits**:
- Zero compile time after first run
- Binary-only deployment
- Cross-platform support (Linux + Android)
- Smaller production footprint

**Speaker Notes**:
Two-phase architecture: compile once on Linux, run many times on any platform. Eliminates compilation overhead in production.

---

### Slide 12: Q&A
**Title**: Questions & Discussion

**Content**:
- Questions about the two-phase architecture?
- Questions about cross-platform deployment?

**Resources**:
- SVG diagrams: `slides/svg/`
- Full documentation: `presentation/USER_GUIDE.md`

**Speaker Notes**:
Open the floor for questions about the architecture and deployment workflow.

---

## Appendix: SVG Diagrams

| Diagram | Slide | Description |
|---------|-------|-------------|
| `svg/202_part1_recap.svg` | 2 | Part I recap summary |
| `svg/203_monolithic_flow.svg` | 3 | Monolithic 6-step flow |
| `svg/204_two_phase_split.svg` | 4 | Two-phase split overview |
| `svg/205_compile_execute_split.svg` | 5, 7 | Compilation vs Execution phases |
| `svg/206_phase_io_details.svg` | 6, 8 | Detailed I/O for both phases |
| `svg/209_cross_platform.svg` | 9 | Linux vs Android deployment |
| `svg/210_deployment_package.svg` | 10 | Production package contents |

---

*Document Version: 2.0*
*Updated for 30-minute presentation format*
