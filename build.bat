@echo off
setlocal

if not exist build mkdir build || exit /b 1
if not exist release mkdir release || exit /b 1

gcc -m64 -std=c11 -Wall -Wextra -Werror -O2 -ffunction-sections -fdata-sections -I. -I..\xllm -I..\xrt -c xwork.c -o release\xwork.o || exit /b 1
gcc -m64 -std=c11 -Wall -Wextra -Werror -O2 -ffunction-sections -fdata-sections -Wl,--gc-sections tests\test_xwork.c ..\xrt\xrt.c -I. -I..\xllm -I..\xrt -lWs2_32 -lIPHLPAPI -o build\test_xwork.exe || exit /b 1
build\test_xwork.exe || exit /b 1

echo.
echo xwork build: PASS
exit /b 0
