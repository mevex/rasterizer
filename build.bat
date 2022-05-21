@echo off

set compilerFlags=-std:c++17 -EHsc -Ox -Zi -nologo -FC -WX -W4 -wd4100 -wd4189 -wd4239 -wd4201 -wd4505 -wd4702 -wd4700

pushd ..\build

cl %compilerFlags% ..\code\main.cpp  -link -opt:ref -incremental:no

popd

REM -Fe[name] is the compiler flag to rename the executable
REM -Ox instead of -Od for the optimized build