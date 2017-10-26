pushd src
call msvcbuild.bat
popd
devenv vcproj\LuaJIT.vcxproj /Build "Release|CTR" /Project "LuaJIT"


