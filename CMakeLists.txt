cmake_minimum_required(VERSION 2.8)

set ( PROJECT_NAME rtspclient_with_opengl )
set ( LIVE555 live555 )
set ( SDL2 sdl2 )

# https://www.microsoft.com/en-us/download/details.aspx?id=6812
# set DXSDK_DIR=C:\Tools/DXSDK
# cmake . -Bstatic -G "Visual Studio 15 2017" -DCMAKE_INSTALL_PREFIX=install -DEXTRA_LDFLAGS="vcruntimed.lib;ucrtd.lib"
# cmake --build static --config Release
message ( $ENV{DXSDK_DIR} )

project( ${PROJECT_NAME} )

add_subdirectory( ${LIVE555} )
add_subdirectory( ${SDL2} )
add_subdirectory( ${PROJECT_NAME} )
