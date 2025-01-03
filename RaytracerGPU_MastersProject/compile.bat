C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/logistic.comp -o shaders/compiled/logistic.comp.spv

C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/ModelSpaceToWorldSpace.comp -o shaders/compiled/ModelSpaceToWorldSpace.comp.spv

C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/GetEnclosingAABB.comp -o shaders/compiled/GetEnclosingAABB.comp.spv --target-env=vulkan1.1
C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/GenerateMortonCodesOfPrimitives.comp -o shaders/compiled/GenerateMortonCodesOfPrimitives.comp.spv
C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/RadixSortSimple.comp -o shaders/compiled/RadixSortSimple.comp.spv --target-env=vulkan1.1
C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/ConstructHLBVH.comp -o shaders/compiled/ConstructHLBVH.comp.spv
C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/ConstructAABBsOfInternalNodes.comp -o shaders/compiled/ConstructAABBsOfInternalNodes.comp.spv

C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/raytrace.comp -o shaders/compiled/raytrace.comp.spv
C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/compute/raytraceBVH.comp -o shaders/compiled/raytraceBVH.comp.spv

C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/fragment/SingleTriangleFullScreen.frag -o shaders/compiled/SingleTriangleFullScreen.frag.spv
C:\VulkanSDK\1.3.250.1\Bin\glslc.exe shaders/vertex/SingleTriangleFullScreen.vert -o shaders/compiled/SingleTriangleFullScreen.vert.spv
pause