@echo off

for %%a in (*.vert) do (
	C:\VulkanSDK\1.2.176.0\Bin32\glslc.exe %%a -o %%a.spv
	echo generated %%a.spv
)

for %%a in (*.frag) do (
	C:\VulkanSDK\1.2.176.0\Bin32\glslc.exe %%a -o %%a.spv
	echo generated %%a.spv
)

echo completed

pause