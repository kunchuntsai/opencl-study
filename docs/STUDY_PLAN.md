# OpenCL Study Plan

A structured learning path to master OpenCL concepts using this repository.

## Prerequisites

Before starting, ensure you understand:
- Basic C/C++ programming
- Pointers and memory management
- Basic understanding of parallel computing concepts (optional but helpful)

## Study Path

### Phase 1: Understand the Concepts (Day 1)

**Goal**: Build theoretical foundation before diving into code

1. **Read the Architecture Overview** (30 min)
   - File: `docs/opencl_concepts.md` - sections 1-2
   - Focus: Understand host-device relationship, platforms, and devices
   - Key question: *What's the role of each component (context, queue, kernel)?*

2. **Study the Execution Model** (30 min)
   - File: `docs/opencl_concepts.md` - section 3
   - Focus: Work items, work groups, NDRange
   - Draw it: Sketch a 2D grid showing how 512x512 image maps to work items
   - Key question: *How does one pixel map to one work item?*

3. **Learn the Memory Model** (45 min)
   - File: `docs/opencl_concepts.md` - section 4
   - Focus: Memory hierarchy (global, local, private)
   - Create a diagram: Draw the memory hierarchy for your own reference
   - Key question: *Why is memory organization important for performance?*

**Checkpoint**: Can you explain to yourself (or someone else) how OpenCL divides work across parallel processors?

### Phase 2: Read the Kernel Code (Day 1-2)

**Goal**: Understand how kernels are written

4. **Analyze the Threshold Kernel** (45 min)
   - File: `kernels/threshold.cl`
   - Read line by line with comments
   - For each line, ask: *What is this doing and why?*

5. **Practice Exercise**: Modify the kernel (30 min)
   - Try mentally modifying the algorithm:
     - What if we want brightness adjustment instead? (add constant to all pixels)
     - What if we want to invert the image? (255 - pixel_value)
   - Don't code yet, just think through the logic
   - Key question: *What would change in the kernel code?*

6. **Reference the Documentation** (30 min)
   - File: `docs/opencl_concepts.md` - section 5 (Kernel Configuration)
   - Cross-reference with the actual kernel code
   - Look up built-in functions: `get_global_id()`, etc.

**Checkpoint**: Can you explain what happens when work item (256, 128) executes the threshold kernel?

### Phase 3: Study the Host Code Workflow (Day 2-3)

**Goal**: Understand the complete OpenCL application flow

7. **Read Host Code - Setup Phase** (60 min)
   - File: `src/main.cpp` - Steps 1-3 (lines ~50-150)
   - Understand: Platform selection, context creation, kernel compilation
   - Reference: `docs/opencl_concepts.md` - sections 2, 6
   - Key question: *Why do we need both context AND command queue?*

8. **Read Host Code - Memory Management** (60 min)
   - File: `src/main.cpp` - Steps 4-6 (lines ~150-220)
   - Understand: Buffer creation, data transfer to device
   - Reference: `docs/opencl_concepts.md` - sections 7, 8
   - Key question: *What's the difference between CL_MEM_READ_ONLY and CL_MEM_WRITE_ONLY?*

9. **Read Host Code - Execution & Cleanup** (60 min)
   - File: `src/main.cpp` - Steps 7-9 (lines ~220-end)
   - Understand: Setting kernel arguments, execution, result retrieval
   - Reference: `docs/opencl_concepts.md` - section 9
   - Key question: *How does the host know when the kernel is done?*

**Checkpoint**: Can you trace the flow of data from host → device → kernel → device → host?

### Phase 4: Hands-On Experimentation (Day 3-4)

**Goal**: Actually see the code work (if you have access to Linux/Windows machine)

10. **Setup Development Environment** (60-120 min)
    - If on Linux/Windows: Follow `README.md` build instructions
    - If on M4 Mac: Use a VM or remote machine, or skip to step 12
    - Verify OpenCL is available with `clinfo`

11. **Build and Run the Example** (30 min)
    - Build the project following `README.md`
    - Run `./threshold_demo`
    - Examine console output - match it with code sections
    - View `input.pgm` and `output.pgm` (use GIMP or online viewer)
    - Key observation: *See the visual result of thresholding*

12. **Experiment with Parameters** (60 min)
    - Modify the threshold value (line ~188 in main.cpp): try 64, 128, 192
    - Rebuild and run, observe different outputs
    - Modify image size: try 256x256, 1024x1024
    - Observe performance differences
    - Key insight: *How do parameters affect the result and performance?*

**Checkpoint**: Can you modify the threshold value and predict what the output image will look like?

### Phase 5: Deep Dive into Specific Topics (Day 4-5)

**Goal**: Master specific OpenCL concepts

13. **Study Memory Patterns** (90 min)
    - File: `docs/opencl_concepts.md` - section 7 (Memory Allocation)
    - Read all three memory patterns
    - Compare with current code in `main.cpp`
    - Exercise: Sketch how you'd modify the code to use mapped memory instead

14. **Study Synchronization** (60 min)
    - File: `docs/opencl_concepts.md` - section 9 (Result Retrieval)
    - Understand: Blocking vs non-blocking operations
    - Exercise: How would you modify the code to use events instead of blocking calls?

15. **Study Kernel Compilation** (60 min)
    - File: `docs/opencl_concepts.md` - section 6
    - Understand: Runtime vs binary compilation
    - Consider: When would you use each approach?

**Checkpoint**: Can you explain the trade-offs between different memory and synchronization strategies?

### Phase 6: Extend the Example (Day 5-7)

**Goal**: Apply your knowledge by extending the code

16. **Add a New Kernel - Brightness Adjustment** (2-3 hours)
    - Create `kernels/brightness.cl`
    - Kernel should add a constant value to each pixel (with clamping to 0-255)
    - Modify `main.cpp` to load and run this kernel
    - Test with different brightness values (-50, +50, +100)
    - Reference similar structure from threshold example

17. **Add a New Kernel - Image Inversion** (1-2 hours)
    - Create `kernels/invert.cl`
    - Kernel should compute: output = 255 - input
    - Add it to the project
    - Compare output with input

18. **Optimization Challenge** (2-3 hours)
    - Modify threshold kernel to use `int4` or `uchar4` vector types
    - Process 4 pixels at once per work item
    - Measure performance difference (add profiling code from docs)
    - Reference: `docs/opencl_concepts.md` - section on profiling

**Checkpoint**: Can you independently create a new kernel and integrate it into the host code?

### Phase 7: Advanced Topics (Day 7-10)

**Goal**: Explore more complex OpenCL features

19. **Study Local Memory** (2-3 hours)
    - Research: What is local memory good for?
    - Design: How would you use local memory for a convolution filter (e.g., blur)?
    - Sketch: Draw how work groups would share data via local memory
    - Don't implement yet, just understand the concept

20. **Study Work Group Optimization** (2-3 hours)
    - Current code uses NULL for local work size (auto-selected)
    - Research: What's a good local work size for your device?
    - Experiment: Try different local sizes (16x16, 32x32, etc.)
    - Measure: Use profiling to see performance impact

21. **Implement Gaussian Blur** (4-6 hours)
    - More complex algorithm requiring local memory
    - Each work item needs data from neighboring pixels
    - Use local memory to cache a tile of the image
    - This will solidify your understanding of the memory hierarchy

**Checkpoint**: Can you implement a kernel that requires cooperation between work items?

### Phase 8: Compare with OpenCV (Day 10-12)

**Goal**: Understand how real-world libraries use OpenCL

22. **Study OpenCV's Approach** (2-3 hours)
    - Read: https://github.com/opencv/opencv/wiki/OpenCL-optimizations
    - Compare: How does OpenCV's `cv::UMat` differ from our explicit approach?
    - Understand: Transparent API vs custom kernels
    - When would you use each approach?

23. **Read OpenCV OpenCL Source** (3-4 hours)
    - Browse OpenCV source code for a simple operation (e.g., threshold)
    - Location: `opencv/modules/imgproc/src/opencl/threshold.cl`
    - Compare their implementation with yours
    - What optimizations do they use?

**Checkpoint**: Can you explain the trade-offs between writing custom kernels vs using library implementations?

## Study Tips

### Active Learning Techniques

1. **Draw Diagrams**: Visualize memory layout, execution flow, data movement
2. **Explain Out Loud**: Pretend you're teaching someone else
3. **Predict Then Verify**: Before running code, predict what will happen
4. **Break Things**: Intentionally introduce bugs to understand error messages
5. **Keep Notes**: Document your insights and "aha" moments

### When You Get Stuck

1. Check the error message against `docs/opencl_concepts.md`
2. Review the corresponding section in the documentation
3. Add `printf` debugging in host code
4. Check kernel compilation logs for syntax errors
5. Verify buffer sizes match data sizes

### Code Reading Strategy

1. **First pass**: Read comments and high-level structure
2. **Second pass**: Read code line by line
3. **Third pass**: Trace execution with specific values
4. **Fourth pass**: Ask "why" for each design decision

## Time Estimates

- **Minimum path** (theory + code reading): 12-15 hours over 5 days
- **Recommended path** (including experimentation): 25-30 hours over 7 days
- **Complete path** (including advanced topics): 40-50 hours over 10-12 days

## Success Criteria

You've mastered the basics when you can:

✅ Explain the 9-step OpenCL workflow from memory
✅ Write a simple kernel without referencing documentation
✅ Modify host code to add a new kernel
✅ Debug common OpenCL errors (platform, compilation, memory)
✅ Choose appropriate memory flags for different use cases
✅ Understand when to use blocking vs non-blocking operations
✅ Implement a kernel that requires local memory

## Next Steps After This Study

1. **Image Processing**: Implement convolution, edge detection, morphology
2. **Matrix Operations**: Matrix multiplication, transpose (different memory access patterns)
3. **Reduction Algorithms**: Sum, min/max, histogram (requires synchronization)
4. **Real Applications**: Integrate OpenCL into a real project
5. **OpenCV Integration**: Learn to use `cv::UMat` and create custom operations

## Resources for Further Learning

- **Official Spec**: Khronos OpenCL Specification (reference when needed)
- **Tutorials**: Intel OpenCL tutorials, AMD GPU programming guides
- **Books**: "OpenCL Programming Guide" by Munshi et al.
- **Practice**: Project Euler problems, implement them with OpenCL

## Study Journal Template

Keep a daily log:

```
Date: ___________
Phase: ___________
Time Spent: ___________

What I Learned:
-

What Confused Me:
-

Questions to Research:
-

Tomorrow's Goal:
-
```

Good luck with your OpenCL learning journey!
