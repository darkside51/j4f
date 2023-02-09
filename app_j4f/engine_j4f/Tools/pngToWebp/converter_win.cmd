
mkdir tmp
cd png

set quality=75

for %%f in (*.png) do (
	%~dp0../../3rd_party/webp/win64/bin/cwebp.exe -q %quality% %%f -o ../webp/%%~nf.webp"
	)

cd ..
del tmp /q
pause