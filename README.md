# rtspclient_with_opengl
RTSP Client with live555 and opengl




http://www.gregwessels.com/dev/2017/10/27/ffmpeg-x265.html

https://github.com/FFmpeg/FFmpeg

https://www.arkeon.be/trunk/dependencies/CMake/Packages/FindFFmpeg.cmake

## Overview
Vcpkg helps you get C and C++ libraries on Windows. This tool and ecosystem are currently in a preview state; your involvement is vital to its success.

For short description of available commands, run `vcpkg help`.


## Prerequisites:

- Windows 10, 8.1, or 7
- Visual Studio 2017 or Visual Studio 2015 Update 3
- Git
- *Optional: CMake 3.10.2

# vcpkg list 
# https://blogs.msdn.microsoft.com/vcblog/2016/09/19/vcpkg-a-tool-to-acquire-and-build-c-open-source-libraries-on-windows/
# https://docs.microsoft.com/en-us/cpp/vcpkg

```
set DXSDK_PATH="D:\Tools\DXSDK"
set PKG_CONFIG_EXECUTABLE="D:\Tools\pkg-config\bin\pkg-config.exe"
set PKG_CONFIG_PATH="D:\workspace\ffmpeg-example\vcpkg\installed\x86-windows\lib"
```


cd D:\Tools\

Clone vcpkg repository, then run
```
git clone https://github.com/Microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
```

Then, to hook up user-wide integration, run (note: requires admin on first use)
```
.\vcpkg integrate install
PS D:\Tools\vcpkg> .\vcpkg integrate install
Applied user-wide integration for this vcpkg root.

All MSBuild C++ projects can now #include any installed libraries.
Linking will be handled automatically.
Installing new libraries will make them instantly available.

CMake projects should use: "-DCMAKE_TOOLCHAIN_FILE=D:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

Install any packages for x86, x64, arm, arm64 windows with
```
.\vcpkg install ffmpeg:x64-windows ffmpeg:x86-windows ffmpeg:arm-windows ffmpeg:arm64-winodws sdl2:x64-winodws sdl2:x86-windows sdl2:arm-windows sdl2:arm64-winodws
```


Finally, create a New Project (or open an existing one) in Visual Studio 2017 or 2015. All installed libraries are immediately ready to be `#include`'d and used in your project.

For CMake projects, simply include our toolchain file. See our [using a package](docs/examples/using-sqlite.md) example for the specifics.
## Tab-Completion / Auto-Completion
`Vcpkg` supports auto-completion of commands, package names, options etc. To enable tab-completion in Powershell, use
```
.\vcpkg integrate powershell
```
and restart Powershell.

Check packages list with
```
.\vcpkg list
```

My CMake script:

```
cmake_minimum_required( VERSION 3.6.0 )

project ( v2t )

add_executable ( v2t WIN32 main.cpp H264_Decoder.cpp YUV420P_Player.cpp )

target_compile_options( v2t PRIVATE -Wall )

find_package( FFmpeg REQUIRED )
target_link_libraries( v2t FFmpeg )
target_include_directories( v2t PRIVATE ${FFMPEG_INCLUDE_DIR} )
```

find_package(SDL REQUIRED)
link_libraries(${SDL_LIBRARY} SDLmain)
include_directories(${SDL_INCLUDE_DIR})

find_package(sdl2 REQUIRED)
target_link_libraries(main PRIVATE SDL2::SDL2 SDL2::SDL2main)
include_directories(${SDL2_INCLUDE_DIR})


add_executable(hello main.cpp)

-DCMAKE_TOOLCHAIN_FILE="D:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake"

cmake .. -DCMAKE_TOOLCHAIN_FILE="D:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake" -G "Visual Studio 15 2017" -DVCPKG_TARGET_TRIPLET=x86-windows-static -DCMAKE_BUILD_TYPE=Debug

cmake .. -G "Visual Studio 15 2017" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="D:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake" 

https://github.com/Microsoft/vcpkg/issues/703

cmake . -Bstatic -G "Visual Studio 15 2017" -DCMAKE_INSTALL_PREFIX=install

cmake .. -Bstatic -G "Visual Studio 15 2017" -DCMAKE_BUILD_TYPE=Release -DVCPKG_TARGET_TRIPLET=x86-windows -DCMAKE_TOOLCHAIN_FILE="D:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_INSTALL_PREFIX=install 
-DCMAKE_CXX_STANDARD_LIBRARIES="user32.lib gdi32.lib winmm.lib imm32.lib ole32.lib oleaut32.lib version.lib uuid.lib dinput8.lib dxguid.lib dxerr.lib kernel32.lib winspool.lib shell32.lib comdlg32.lib advapi32.lib vcruntimed.lib ucrtd.lib" \

cmake --build static --config Release --target install


http://www.voidcn.com/article/p-tlcacfpn-bs.html
https://github.com/sipsorcery/mediafoundationsamples/tree/master/MFWebCamRtp
http://en.pudn.com/Download/item/id/1303846.html
http://blog.chinaunix.net/uid-15063109-id-4482932.html
https://stackoverflow.com/questions/34619418/ffmpeg-does-not-decode-some-h264-streams
https://stackoverflow.com/questions/17579286/sdl2-0-alternative-for-sdl-overlay
https://github.com/sipsorcery/mediafoundationsamples/blob/master/MFWebCamRtp/MFWebCamRtp.cpp
https://stackoverflow.com/questions/19427576/live555-x264-stream-live-source-based-on-testondemandrtspserver

https://github.com/xiongziliang/ZLMediaKit
https://github.com/royshil/KinectAtHomeExtension
https://github.com/flowerinthenight/ffmpeg-encode-h264mp4/tree/master/H264Encoder