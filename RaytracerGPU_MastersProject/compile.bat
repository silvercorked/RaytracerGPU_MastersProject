C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/logistic.comp -o shaders/compiled/logistic.comp.spv

C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/fragment/SingleTriangleFullScreen.frag -o shaders/compiled/SingleTriangleFullScreen.frag.spv
C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/vertex/SingleTriangleFullScreen.vert -o shaders/compiled/SingleTriangleFullScreen.vert.spv

C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/ConstructAABBs.comp -o shaders/compiled/ConstructAABBs.comp.spv
C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/GenerateMortonCodesOfAABBs.comp -o shaders/compiled/GenerateMortonCodesOfAABBs.comp.spv
C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/RadixSortSimple.comp -o shaders/compiled/RadixSortSimple.comp.spv --target-env=vulkan1.1

C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/ModelSpaceToWorldSpace.comp -o shaders/compiled/ModelSpaceToWorldSpace.comp.spv
C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/raytrace.comp -o shaders/compiled/raytrace.comp.spv
pause