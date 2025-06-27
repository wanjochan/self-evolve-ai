@echo off
pushd tcc-win\tcc
tcc.exe -o ..\..\program_c99.exe ..\..\program_c99.c ..\..\x64_codegen.c
popd