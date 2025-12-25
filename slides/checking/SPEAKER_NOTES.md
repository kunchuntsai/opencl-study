# OpenCL Image Processing Framework
## Speaker Notes & Storytelling Guide (42-Slide Version)

---

## Overview: The Story Arc

**Main Narrative**: Transform your audience from "OpenCL is painful" to "This framework makes it easy"

**Story Structure**:
1. **Part I (30 min)**: Establish the problem → Show the solution → Demonstrate ease of use
2. **Part II (30 min)**: Deep dive into architecture → Show production deployment → Empower adoption

**Key Emotional Journey**:
- Frustration (slides 1-4): "Yes, I've felt that pain"
- Relief (slides 5-7): "This looks much simpler"
- Understanding (slides 8-23): "Now I see how it works"
- Confidence (slides 24-41): "I can use this in production"
- Excitement (slide 42): "I want to try this!"

**New Navigation Feature**: This presentation uses **highlighted agenda slides** to show progress. Each section begins with the agenda showing the current topic highlighted in orange. This helps the audience track where they are in the overall presentation.

---

## PART I: OCLExample Introduction (30 min)

---

### Slide 1: Title Slide
**Time**: 1 min

**Speaker Notes**:
> Welcome everyone. Today I'm going to show you a framework that will fundamentally change how you develop OpenCL image processing algorithms.
>
> The tagline says it all: "Easy Kernel Development, Zero Host Code."
>
> By the end of this hour, you'll understand how to go from writing your first kernel to deploying on Android — without writing a single line of host code.

**Storytelling Tip**: Start with energy. Make eye contact. Let the title sink in before moving on.

---

### Slide 2: Agenda - Introduction / Features
**Time**: 30 sec

**Speaker Notes**:
> Here's our roadmap for today. We're in Part I — OCLExample Introduction.
>
> [Point to highlighted item] We're starting with Introduction and Features — what this framework is and why you need it.
>
> Then we'll cover the OpenCL host flow, platform options, caching mechanism, and JSON configuration.
>
> Part II dives into the two-phase architecture for production deployment.

**Storytelling Tip**: Point to the highlighted section. Establish the navigation pattern you'll use throughout.

---

### Slide 3: Section Overview - Introduction / Features
**Time**: 1 min

**Speaker Notes**:
> In this first section, we'll cover five key areas:
>
> First, the **problem** — why existing OpenCL development is painful.
>
> Second, our **solution** — the benefits this framework provides.
>
> Third, how users **benefit** from zero host code.
>
> Fourth, the **user journey** from writing a kernel to seeing results.
>
> And finally, the **high-level architecture** showing how components fit together.
>
> Let's start with the problem.

**Storytelling Tip**: Use this overview to set expectations. Touch each bullet as you mention it.

---

### Slide 4: The Problem We Solve
**Time**: 3 min

**Speaker Notes**:
> Let me start with a question: How many of you have written OpenCL host code before?
>
> [Pause for hands]
>
> Then you know the pain. Every time you want to test a new kernel, you need to write 500 lines of boilerplate. You need to manage contexts, command queues, buffers — all manually.
>
> You compile the kernel, handle errors, set up every argument with the right type and size. Then you verify against a CPU reference to make sure it's correct.
>
> And here's the worst part: you repeat ALL of this for every kernel variant. Want to try a different optimization? Another 500 lines.
>
> This framework eliminates ALL of that boilerplate. You focus on your kernel — we handle the rest.

**Storytelling Tip**: Read each pain point slowly. Let the audience nod in recognition. Build the frustration before revealing the solution.

**Transition**: "So what does the solution look like?"

---

### Slide 5: Framework Benefits
**Time**: 3 min

**Speaker Notes**:
> Here are the six key benefits you get from this framework.
>
> **Zero Host Code** — Your configuration lives in JSON. No C/C++ host code to write or maintain.
>
> **Instant Comparison** — Want to compare v0 against v1 of your kernel? Just run both. The framework handles everything.
>
> **Platform Switching** — Switch between standard OpenCL and vendor-specific CL extensions with one line in your config.
>
> **Auto-Verification** — Every GPU output is automatically compared against a CPU reference. You'll know immediately if something is wrong.
>
> **Binary Caching** — After the first run, your compiled kernel is cached. Subsequent runs skip compilation entirely.
>
> **Easy Extension** — Adding a new algorithm? We have a scaffold that generates all the boilerplate for you.

**Storytelling Tip**: Touch each card as you explain it. Let benefits build momentum.

**Transition**: "Let me show you how this works in practice."

---

### Slide 6: User Journey
**Time**: 3 min

**Speaker Notes**:
> The typical user journey has three phases: Build, Run, and Learn.
>
> **BUILD**: You write your kernel in standard OpenCL C. You create a simple JSON config that defines inputs, outputs, and kernel arguments. That's it — no host code.
>
> **RUN**: You execute using our command-line tool. It compiles your kernel, sets up all the OpenCL infrastructure, runs your algorithm, and produces output. All automatically.
>
> **LEARN**: The framework gives you detailed profiling data. You see exactly how long each operation took. You compare against the CPU reference. Then you iterate — tweak your kernel, run again, see improvements.
>
> This cycle — Build, Run, Learn — is how you go from initial concept to optimized production kernel.

**Storytelling Tip**: Trace the flow with your hand. Emphasize the simplicity of each step.

---

### Slide 7: High-Level Architecture
**Time**: 3 min

**Speaker Notes**:
> Here's the architecture at a high level.
>
> On the left is YOUR code — just the kernel and JSON config. That's all you need to provide.
>
> On the right is the FRAMEWORK — it handles everything else.
>
> The framework reads your config, sets up OpenCL, compiles your kernel, manages memory, executes, and verifies results.
>
> Notice the clear separation: you stay in "user space" with your algorithm logic. The framework handles all the "plumbing" in its space.
>
> This separation is why you can focus purely on optimization without worrying about infrastructure.

**Storytelling Tip**: Use two-hand gestures — left hand for "your code", right hand for "framework". Emphasize the clean boundary.

**Transition**: "Now let's look at what happens inside the framework."

---

### Slide 8: Agenda - General OpenCL Host Flow
**Time**: 15 sec

**Speaker Notes**:
> [Point to highlighted item] Now we're moving to General OpenCL Host Flow.
>
> Let's peek under the hood and see what the framework does for you.

**Storytelling Tip**: Quick transition. Use the agenda to re-orient the audience.

---

### Slide 9: Section Overview - General OpenCL Host Flow
**Time**: 30 sec

**Speaker Notes**:
> In this section, we'll see:
>
> The **6 standard steps** every OpenCL program must perform.
>
> How the **framework handles** all these steps automatically.
>
> The **timing breakdown** — where time goes during execution.
>
> And how this enables **rapid iteration** during development.

**Storytelling Tip**: Brief overview — don't linger here.

---

### Slide 10: Standard OpenCL Host Flow
**Time**: 4 min

**Speaker Notes**:
> This is the standard OpenCL host flow — the 6 steps that every OpenCL program must perform.
>
> **Step 1: Platform Discovery** — Find available GPUs using clGetPlatformIDs.
>
> **Step 2: Context Creation** — Create the execution context with clCreateContext.
>
> **Step 3: Kernel Build** — This is where your kernel gets compiled with clBuildProgram. Notice the timing: ~100ms. This is a significant cost.
>
> **Step 4: Buffer Allocation** — Allocate GPU memory with clCreateBuffer.
>
> **Step 5: Kernel Execute** — Actually run the kernel with clEnqueueNDRangeKernel.
>
> **Step 6: Result Retrieve** — Read results back with clRead.
>
> The framework does ALL of this for you. You just provide the kernel and config — we handle these 6 steps automatically.
>
> Pay special attention to that ~100ms compilation time. We'll see later how caching eliminates this.

**Storytelling Tip**: Walk through each step. Emphasize the ~100ms compilation — it's the hook for the caching story later.

---

### Slide 11: Agenda - Standard CL vs CL Extension
**Time**: 15 sec

**Speaker Notes**:
> [Point to highlighted item] Next: Standard CL vs CL Extension.
>
> Not all OpenCL is created equal. Let me show you the two paths.

---

### Slide 12: Section Overview - Standard CL vs CL Extension
**Time**: 30 sec

**Speaker Notes**:
> This section covers:
>
> **Two platform types** — Standard OpenCL and vendor CL Extensions.
>
> **When to use each** — Development vs production scenarios.
>
> **One-line switching** — Change platforms with a single config line.
>
> And how the **same kernel code** works on both platforms.

---

### Slide 13: Platform Branching
**Time**: 4 min

**Speaker Notes**:
> The framework supports two platform types, and you choose with a single line in your JSON config.
>
> **Standard OpenCL** on top — this uses only standard OpenCL APIs. It works on ANY OpenCL-capable GPU. Perfect for development and portability.
>
> **CL Extension** on bottom — this uses vendor-specific extensions. It's optimized for specific hardware and gives you maximum performance. Perfect for production.
>
> The key insight: your kernel code stays the same. Only the config changes.
>
> You develop on Standard CL for fast iteration, then switch to CL Extension for production deployment. One line change.

**Storytelling Tip**: Point to both paths. Emphasize the "same kernel, different platform" message.

---

### Slide 14: Platform Configuration in JSON
**Time**: 2 min

**Speaker Notes**:
> Here's what that looks like in practice.
>
> On the left, you see the JSON configuration. The key line is `host_type` — just change this one value.
>
> On the right, the key benefits:
> - Runtime platform selection — no recompile needed
> - No code changes required — same kernel.cl
> - Switch with one line change
> - Run both variants to compare performance
>
> Development uses Standard CL. Production uses CL Extension. Same kernel, maximum flexibility.

**Storytelling Tip**: Point to the host_type field, then to the benefits list.

---

### Slide 15: Agenda - Caching Mechanism
**Time**: 15 sec

**Speaker Notes**:
> [Point to highlighted item] Now let's look at Caching Mechanism.
>
> Remember that 100ms compilation time? Here's how we eliminate it.

---

### Slide 16: Section Overview - Caching Mechanism
**Time**: 30 sec

**Speaker Notes**:
> This section covers:
>
> **Binary caching** — How compiled kernels are stored and reused.
>
> **Cache hit vs miss** — What happens on first run vs subsequent runs.
>
> **20x speedup** — From ~100ms to ~5ms startup time.
>
> And **automatic invalidation** — How the cache stays fresh when code changes.

---

### Slide 17: Binary Caching
**Time**: 4 min

**Speaker Notes**:
> This diagram shows the caching mechanism.
>
> When you run a kernel, the framework first checks if a cached binary exists.
>
> **MISS (first run)**: No cache found, so we call clBuildProgram(). This takes ~100ms. The compiled binary is saved to cache.
>
> **HIT (cached)**: Cache exists! We load the binary directly. This takes only ~5ms. That's a 20x improvement.
>
> The cache is organized by algorithm and variant. Each gets its own binary.
>
> **Auto-invalidation**: The framework computes a SHA256 hash of your kernel source. If the hash changes, the cache is automatically invalidated and recompiled.
>
> You never need to manually manage the cache — it just works.

**Storytelling Tip**: Trace both paths on the diagram. Emphasize the 20x speedup.

---

### Slide 18: Agenda - JSON Configuration
**Time**: 15 sec

**Speaker Notes**:
> [Point to highlighted item] Our last Part I topic: JSON Configuration.
>
> This is where you define your algorithm without writing host code.

---

### Slide 19: Section Overview - JSON Configuration
**Time**: 30 sec

**Speaker Notes**:
> This section covers:
>
> **File structure** — How the JSON config is organized.
>
> **Global vs algorithm configs** — Shared settings vs per-algorithm settings.
>
> **Kernel configuration** — Defining work sizes and entry points.
>
> And **argument types** — All the data types the framework supports.

---

### Slide 20: JSON Configuration Structure
**Time**: 2 min

**Speaker Notes**:
> The JSON config has two levels.
>
> **Global config** defines shared settings — input dimensions, host type, I/O directories. These apply to all algorithms.
>
> **Algorithm config** is specific to each algorithm. Here we have ReLU with three variants: v0, v1, v2.
>
> Each variant specifies its kernel file, entry point, and arguments.
>
> This separation means you define common settings once and override only what's different per algorithm.

**Storytelling Tip**: Point to global section, then algorithm section. Show the hierarchy.

---

### Slide 21: Kernel Configuration
**Time**: 3 min

**Speaker Notes**:
> Let's look at a specific kernel configuration.
>
> **kernel_file**: Points to your OpenCL source file.
>
> **entry_point**: The kernel function name to execute.
>
> **global_work_size** and **local_work_size**: Define the execution grid. Notice these can reference global dimensions like `img_width` and `img_height`.
>
> **args**: An array defining each kernel argument — type, name, and value.
>
> This is all the information the framework needs to compile and execute your kernel.

**Storytelling Tip**: Walk through each field. Show how dimensions are referenced from global config.

---

### Slide 22: Supported Argument Types
**Time**: 2 min

**Speaker Notes**:
> The framework supports a comprehensive set of argument types.
>
> **Buffer types**: input_buffer and output_buffer for GPU memory.
>
> **Scalar types**: int, uint, float, size_t for kernel parameters.
>
> **Special types**: local_mem for local memory allocation.
>
> Each type maps to the appropriate OpenCL argument. The framework handles all the clSetKernelArg() calls automatically.

**Storytelling Tip**: Quick overview — this is reference material. Don't spend too long here.

---

### Slide 23: Part I Summary
**Time**: 2 min

**Speaker Notes**:
> Let's summarize Part I.
>
> We've seen how the framework provides:
> - Zero host code — just kernel and JSON
> - Automatic compilation and execution
> - Platform flexibility — Standard CL or CL Extension
> - Binary caching for fast iteration
>
> You now have everything you need for development. But what about production?
>
> Part II covers the two-phase architecture that enables production deployment.

**Storytelling Tip**: Brief recap. Build anticipation for Part II.

**Transition**: "Now let's look at production deployment."

---

## PART II: Two-Phase Architecture (30 min)

---

### Slide 24: Agenda - Recap
**Time**: 30 sec

**Speaker Notes**:
> Welcome to Part II — Two-Phase Architecture.
>
> [Point to highlighted Recap] We'll start with a quick recap of Part I, then dive into why we need two phases, the compilation phase, execution phase, and cross-platform deployment.
>
> By the end, you'll know how to take your kernel from development to production.

**Storytelling Tip**: Re-establish the agenda for Part II. Note that we're now in the second column.

---

### Slide 25: Part I Recap
**Time**: 1 min

**Speaker Notes**:
> Quick recap of Part I:
>
> **Framework handles** the 6-step OpenCL host flow automatically.
>
> **Two platforms** — Standard CL for development, CL Extension for production.
>
> **Binary caching** — 20x faster startup after first run.
>
> **JSON configuration** — Define algorithms without host code.
>
> Now the question: How do we deploy this to production? That's what Part II answers.

**Storytelling Tip**: Fast recap — don't repeat Part I in detail.

---

### Slide 26: Agenda - Why Two-Phase Architecture
**Time**: 15 sec

**Speaker Notes**:
> [Point to highlighted item] Let's understand why we need two phases.
>
> The monolithic approach has serious limitations for production.

---

### Slide 27: Section Overview - Why Two-Phase Architecture
**Time**: 30 sec

**Speaker Notes**:
> This section covers:
>
> **Problems with monolithic** — Why compile-every-time doesn't work in production.
>
> **The solution** — Separating compilation from execution.
>
> **Benefits** — What you gain from two-phase architecture.
>
> And the **architecture diagram** showing how it works.

---

### Slide 28: The Problem with Monolithic Flow
**Time**: 3 min

**Speaker Notes**:
> Why do we need two phases? Because the monolithic flow has serious problems for production.
>
> **Problem 1**: Every run recompiles the kernel. Even with caching, the first run on a new device takes ~100ms. For real-time applications, this matters.
>
> **Problem 2**: Source code must be present on the target device. Do you want to ship your kernel source to customers? Probably not.
>
> **Problem 3**: Compilation requires the full OpenCL toolchain. That's a lot of dependencies on embedded devices.
>
> **Problem 4**: No binary-only distribution. You're forced to ship source.
>
> The solution: split into Compilation phase and Execution phase. Compile on your dev machine, execute on the target.

**Storytelling Tip**: Build the problem before the solution. Each point should feel like a real concern.

---

### Slide 29: Agenda - Compilation Phase
**Time**: 15 sec

**Speaker Notes**:
> [Point to highlighted item] Now let's look at the Compilation Phase.
>
> This is where we pre-compile kernels for target devices.

---

### Slide 30: Section Overview - Compilation Phase
**Time**: 30 sec

**Speaker Notes**:
> This section covers:
>
> **When and where** compilation happens — on your dev machine, not the target.
>
> **Inputs and outputs** — What goes in, what comes out.
>
> **kernel.bin contents** — What's actually in the compiled binary.
>
> And **target-specific compilation** — Building for different hardware.

---

### Slide 31: Two-Phase Architecture
**Time**: 4 min

**Speaker Notes**:
> Here's the two-phase architecture.
>
> **Left side — Compilation Phase**: This runs on your Linux development machine. It takes kernel source and JSON config as input. It outputs a compiled binary and an executor program.
>
> **Right side — Execution Phase**: This runs on the target device — Linux or Android. It takes the pre-compiled binary and runtime config. It produces processed output.
>
> The key insight: compilation happens ONCE on your dev machine. Execution happens MANY times on target devices — without ever compiling.
>
> This is how you get fast startup, binary-only distribution, and cross-platform deployment.

**Storytelling Tip**: Draw an invisible line between phases. Emphasize "once" vs "many times".

---

### Slide 32: Compilation Phase Details
**Time**: 3 min

**Speaker Notes**:
> Let's look at the Compilation Phase in detail.
>
> **Inputs**: Your kernel source file and JSON configuration.
>
> **Process**: ClExtensionBuildKernel() compiles with hardware-specific optimizations.
>
> **Outputs**: Two things —
> 1. `kernel.bin` — the compiled binary, specific to the target hardware
> 2. `opencl_host` — an executable that can run on Android
>
> The key benefits:
> - No source code on target
> - Pre-compiled for specific hardware
> - One-time compilation cost

**Storytelling Tip**: Show input → output flow. Emphasize the pre-compilation benefit.

---

### Slide 33: Agenda - Execution Phase
**Time**: 15 sec

**Speaker Notes**:
> [Point to highlighted item] Now the Execution Phase.
>
> This is what runs on your target devices.

---

### Slide 34: Section Overview - Execution Phase
**Time**: 30 sec

**Speaker Notes**:
> This section covers:
>
> **Target devices** — Linux and Android execution.
>
> **clCreateProgramWithBinary** — The API that loads pre-compiled code.
>
> **20x performance** — Why binary loading is so much faster.
>
> And **runtime requirements** — What the target needs to execute.

---

### Slide 35: Execution Phase
**Time**: 3 min

**Speaker Notes**:
> Now the Execution Phase.
>
> **Inputs**: The pre-compiled `kernel.bin`, runtime config, and input data.
>
> **Output**: Processed result.
>
> The magic is in one API call: `clCreateProgramWithBinary()`. Instead of compiling from source, we load the pre-compiled binary directly.
>
> Look at the performance difference: ~5ms with pre-compiled binary versus ~105ms with source compilation. That's a 20x improvement.
>
> For real-time image processing, this difference is critical.

**Storytelling Tip**: Emphasize clCreateProgramWithBinary(). The performance comparison should draw attention.

---

### Slide 36: Agenda - Cross-Platform Deployment
**Time**: 15 sec

**Speaker Notes**:
> [Point to highlighted item] Now Cross-Platform Deployment.
>
> How do you get from Linux development to Android production?

---

### Slide 37: Section Overview - Cross-Platform Deployment
**Time**: 30 sec

**Speaker Notes**:
> This section covers:
>
> **Develop on Linux** — Your full development environment.
>
> **Deploy to targets** — Linux servers and Android devices.
>
> **What to ship** — The minimal deployment package.
>
> And **what NOT to ship** — Keep source code on your dev machine.

---

### Slide 38: Cross-Platform Deployment
**Time**: 3 min

**Speaker Notes**:
> Here's how cross-platform deployment works.
>
> You develop on Linux — that's your compilation environment. You have access to the full toolchain: compiler, debugger, profiler.
>
> From Linux, you produce a deployment package: the opencl_host executable, kernel.bin, and config.json.
>
> That package can deploy to Linux production servers OR to Android devices. Same artifacts, different targets.
>
> For Android, just use `adb push` to copy files, then execute. No compilation infrastructure needed on the device.

**Storytelling Tip**: Show the flow from left to right. Emphasize Android deployment simplicity.

---

### Slide 39: What to Ship
**Time**: 3 min

**Speaker Notes**:
> Let's be specific about what you ship to production.
>
> **What you SHIP**:
> - `opencl_host` — the executor binary (Linux or ARM version)
> - `kernel.bin` — your compiled algorithm
> - `config.json` — runtime configuration
>
> That's it. Three files.
>
> **What you DON'T ship**:
> - Source code — stays on your dev machine
> - Headers — not needed at runtime
> - Build tools — no compilation on target
> - Dev dependencies — production is lean
>
> Benefits: smaller footprint, no source exposure, faster startup.

**Storytelling Tip**: Contrast the two lists. "Ship this, not that."

---

### Slide 40: Agenda - Summary & Q&A
**Time**: 15 sec

**Speaker Notes**:
> [Point to highlighted item] We're at our final section: Summary and Q&A.
>
> Let's wrap up what we've learned.

---

### Slide 41: Summary
**Time**: 3 min

**Speaker Notes**:
> Let's summarize everything we've covered.
>
> **Part I — OCLExample Introduction**:
> - Zero host code — JSON configuration
> - Framework handles 6-step OpenCL flow
> - Standard CL and CL Extension platforms
> - Binary caching for fast iteration
>
> **Part II — Two-Phase Architecture**:
> - Compile once on Linux
> - Execute anywhere — Linux or Android
> - Binary-only deployment
> - 20x faster startup
>
> The framework transforms OpenCL development from painful to productive.

**Storytelling Tip**: Touch each bullet. Build to a confident conclusion.

---

### Slide 42: Q&A
**Time**: 5+ min

**Speaker Notes**:
> Thank you for your attention!
>
> Let's open the floor for questions.
>
> [If no immediate questions, seed with:]
> - "A common question is about performance comparison with hand-written host code..."
> - "Some teams ask about integrating with existing build systems..."
> - "You might be wondering about debugging kernel issues..."
>
> [For closing:]
> The framework is available at [repo location]. Documentation is at [docs location].
>
> Feel free to reach out if you have questions after this session.
>
> Thank you!

**Storytelling Tip**: Genuine openness for questions. Have 2-3 seeded questions ready if needed. End with clear next steps.

---

## Timing Summary

| Section | Slides | Time |
|---------|--------|------|
| **Part I: OCLExample Introduction** | | **~30 min** |
| - Introduction / Features | 1-7 | 14 min |
| - General OpenCL Host Flow | 8-10 | 5 min |
| - Standard CL vs CL Extension | 11-14 | 7 min |
| - Caching Mechanism | 15-17 | 5 min |
| - JSON Configuration | 18-23 | 9 min |
| **Part II: Two-Phase Architecture** | | **~25 min** |
| - Recap | 24-25 | 2 min |
| - Why Two-Phase Architecture | 26-28 | 4 min |
| - Compilation Phase | 29-32 | 8 min |
| - Execution Phase | 33-35 | 4 min |
| - Cross-Platform Deployment | 36-39 | 7 min |
| - Summary & Q&A | 40-42 | 8+ min |
| **Total** | **42 slides** | **~60 min** |

---

## Navigation Pattern

This presentation uses a consistent navigation pattern:

1. **Agenda Slide** — Shows full agenda with current section highlighted in orange
2. **Section Overview** — Bullet points previewing section content
3. **Content Slides** — The actual material
4. **Repeat** — Next agenda slide for the next section

This pattern helps the audience:
- Know where they are in the overall presentation
- Anticipate what's coming in each section
- Feel the progress as sections complete

---

## Key Messages to Reinforce

1. **"Zero Host Code"** — Say this phrase at least 3 times
2. **"100ms → 5ms"** — The caching benefit is memorable
3. **"Compile Once, Run Anywhere"** — Part II tagline
4. **"Same kernel, different platform"** — Flexibility message
5. **"Three files to ship"** — Production simplicity
6. **"20x faster startup"** — Performance headline

---

## Audience Engagement Tips

- **Ask questions early** (slide 4) — "How many have written OpenCL host code?"
- **Use pauses** — After key benefits, let them sink in
- **Point to visuals** — Diagrams do the heavy lifting
- **Watch for confusion** — If you see puzzled faces, offer to clarify
- **Use agenda transitions** — "As you can see, we're now moving to..."
- **Invite questions at midpoint** (slide 23) — Don't wait until the end

---

## Potential Questions & Answers

**Q: How does this compare to just using vendor SDKs?**
> A: Vendor SDKs give you vendor-specific optimizations but lock you in. Our framework abstracts both standard CL and vendor extensions behind the same config. You get portability AND performance.

**Q: What's the learning curve?**
> A: If you know OpenCL kernel programming, you're 90% there. The JSON config takes maybe an hour to learn. Most people are productive within a day.

**Q: Can I use this with existing kernels?**
> A: Absolutely. Just create a JSON config for your existing kernel. No code changes needed.

**Q: What about debugging?**
> A: The framework provides detailed error messages and profiling. For kernel debugging, you still use standard OpenCL debugging tools — our framework doesn't interfere.

**Q: How do I handle different image sizes?**
> A: The JSON config supports parameterization. You can define image dimensions as variables that get resolved at runtime.

**Q: Does the binary work across different GPU models?**
> A: Binaries are device-specific. You need to compile separately for each target GPU type. The framework makes this easy to manage.

**Q: What's the overhead of the framework vs hand-written code?**
> A: Negligible. The framework adds maybe 1-2% overhead for the orchestration. The kernel execution itself is identical.

---

## Diagram Quick Reference

| Slide | Diagram | Key Points |
|-------|---------|------------|
| 6 | User Journey | Build → Run → Learn cycle |
| 7 | Architecture Overview | User Space vs Framework |
| 10 | OpenCL Host Flow | 6 steps, ~100ms compile time |
| 13 | Platform Branching | Standard CL vs CL Extension |
| 17 | Binary Caching | Cache hit/miss, 20x speedup |
| 31 | Two-Phase Architecture | Compile vs Execute phases |
| 38 | Cross-Platform Deployment | Linux dev → Linux/Android targets |

---

## Final Checklist

Before presenting:
- [ ] Test all slides render correctly
- [ ] Verify diagrams are visible on projection system
- [ ] Have backup PDF version ready
- [ ] Know your timing for each section
- [ ] Prepare 2-3 seed questions for Q&A
- [ ] Have repo/docs URLs ready to share
