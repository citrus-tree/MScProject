@echo off

for %%a in (*.vert) do (
	..\..\ext\shaderc\tools\glslc.exe %%a -o %%a.spv
	echo generated %%a.spv
)

for %%a in (*.frag) do (
	..\..\ext\shaderc\tools\glslc.exe %%a -o %%a.spv
	echo generated %%a.spv
)

echo completed

pause