%VULKAN_SDK%/Bin/glslc.exe shader.vert -o vert.spv
%VULKAN_SDK%/Bin/glslc.exe shader.frag -o frag.spv

mkdir tmp
cd glsl

for %%f in (*.frag, *.vert*) do (
	%VULKAN_SDK%/Bin/glslc.exe %%f -o ../spirv/%%f.spv
	)

for %%f in (*.vsh) do (
	copy %%f %~dp0tmp\tmp.vert
	%VULKAN_SDK%/Bin/glslc.exe %~dp0tmp\tmp.vert -o ../spirv/%%f.spv
	)

for %%f in (*.psh) do (
	copy %%f %~dp0tmp\tmp.frag
	%VULKAN_SDK%/Bin/glslc.exe %~dp0tmp\tmp.frag -o ../spirv/%%f.spv
	)

cd ..
del tmp /q
pause