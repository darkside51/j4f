mkdir %~dp0tmp
cd %2

for %%f in (*.frag, *.vert, *.geom) do (
	%VULKAN_SDK%/Bin/glslc.exe %%f -o %1/%%f.spv
	)

for %%f in (*.vsh) do (
	copy %%f %~dp0tmp\tmp.vert
	%VULKAN_SDK%/Bin/glslc.exe %~dp0tmp\tmp.vert -o %1/%%f.spv
	)

for %%f in (*.psh) do (
	copy %%f %~dp0tmp\tmp.frag
	%VULKAN_SDK%/Bin/glslc.exe %~dp0tmp\tmp.frag -o %1/%%f.spv
	)

for %%f in (*.gsh) do (
	copy %%f %~dp0tmp\tmp.geom
	%VULKAN_SDK%/Bin/glslc.exe %~dp0tmp\tmp.geom -o %1/%%f.spv
	)

cd ..
del %~dp0tmp /q
pause