## Note:
Reference: https://github.com/opencv/opencv/wiki/OpenCL-optimizations

- in main.cpp funciton, be able to load image parameters, kernel codes path from the target class
- new operations will inherit from OpsBase class and implement its details

The OpBase/derived classes will focus on preparing their inputs/outputs, kernel implementation and golden results for the rest of CL host processes will handled by main.cpp or OpBase class. Thus, main.cpp may need something like Op list for adding/removing each derived classes. 

Keep the custom OP implementation clean and only focus on it's
- inputs/outputs
- kernel implementation
- golden sample verification
And don't touch any CL host flow control behaviour

Move the rest of host behaviour flow control to main.cpp, e.g. 
- Step 5: Allocate device memory buffers
- Step 6: Transfer data from host to device
- Step 7: Configure kernel arguments and execute <it should be read parameters from custom class but the actual control should be in main.cpp>
- Step 8: Retrieve results from device to host