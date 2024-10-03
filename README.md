
# RaytracerGPU
### Purpose
This project aims to create a small game engine that primarily renders with raytracing. This is largely a learning project, so suggestions and ideas are welcome.
This will also serve as my master's project for my degree at the University of Nebraska at Omaha.

### To setup (windows only)
This project is created using C++20, Vulkan SDK 1.3.250.1, glfw 3.3.8, glm 0.9.9.8, tiny_obj_loader.h from the https://github.com/tinyobjloader/tinyobjloader repository.
Download links for all but tiny_obj_loader can be found within the vulkan tutorial for setting up a development environment (https://vulkan-tutorial.com/Development_environment)
You may need to modify the `Additional Include Directories` under C/C++, General and `Additional Library Directories` under Linker, General.

Next, the shaders used need to be compiled. The file `compile.bat` contains commands for compiling the shaders, but uses hard-coded paths to my local vulkan install location (which includes the version number).
Change this path if need be.

Run compile.bat once and after any future changes to shader code.

You should now be able to run the program via the Visual Studio debugger. If you have trouble, feel free to sent me a message over github.
