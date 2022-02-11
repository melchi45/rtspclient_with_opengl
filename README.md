# rtspclient_with_opengl
RTSP Client with live555 and opengl

![test application image](https://github.com/melchi45/rtspclient_with_opengl/blob/master/capture.png?raw=true)

## Overview
Vcpkg helps you get C and C++ libraries on Windows. This tool and ecosystem are currently in a preview state; your involvement is vital to its success.

For short description of available commands, run `vcpkg help`.


## Prerequisites:

- Windows 10, 8.1, or 7
- Visual Studio 2017 or Visual Studio 2015 Update 3
- Git
- *Optional: CMake 3.10.2

```
set DXSDK_PATH="D:\Tools\DXSDK"
set PKG_CONFIG_EXECUTABLE="D:\Tools\pkg-config\bin\pkg-config.exe"
set PKG_CONFIG_PATH="D:\workspace\ffmpeg-example\vcpkg\installed\x86-windows\lib"
```

## vcpkg

about vcpkg from this url.
 - https://blogs.msdn.microsoft.com/vcblog/2016/09/19/vcpkg-a-tool-to-acquire-and-build-c-open-source-libraries-on-windows/
 - https://docs.microsoft.com/en-us/cpp/vcpkg

install vcpkg to D:\Tools folder
```
d:
mkdir Tools
cd D:\Tools\
```

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

Install any packages for x86(x86-windows), x64(x64-windows), arm(arm-windows), arm64(arm64-winodws) windows with
```
.\vcpkg install ffmpeg:x64-windows ffmpeg:x86-windows --recurse
.\vcpkg install sdl2:x64-windows sdl2:x86-windows
.\vcpkg install pthreads:x64-windows pthreads:x86-windows
.\vcpkg install glew:x86-windows glew:x64-windows
.\vcpkg install glfw3:x86-windows glfw3:x64-windows
.\vcpkg install libpng:x86-windows libpng:x64-windows
.\vcpkg install zlib:x86-windows zlib:x64-windows
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

## depencency package for linux
### For OpenGL
```
sudo apt-get install -y libgl-dev
sudo apt-get install -y mesa-common-dev
sudo apt-get install -y libglu1-mesa-dev freeglut3-dev mesa-common-dev libgl1-mesa-dev
sudo apt-get install -y libxi-dev build-essential libdbus-1-dev libfontconfig1-dev libfreetype6-dev libx11-dev
sudo apt-get install -y libqt4-dev zlib1g-dev libqt4-opengl-dev
sudo apt-get install -y libglew-dev libglfw3-dev
```

### For FFMpeg
```
sudo apt-get install -y libavcodec-dev libavformat-dev libavdevice-dev
sudo apt-get install -y ffmpeg libavcodec-dev libavutil-dev libavformat-dev libavfilter-dev libswscale-dev libavdevice-dev libavdevice-dev libavresample-dev libpostproc-dev libswresample-dev libswscale-dev
```

### For SDL2
```
sudo apt-get install -y libsdl2-2.0 libsdl2-dev libsdl-dev libsdl-image1.2-dev
```

## Build for Windows

Build with IDE
You have to change vcpkg path[E:\Tools\vcpkg\installed\x64-windows-static] to your environment from the this line.
```
cmake -B x64 -G "Visual Studio 16 2019" -A x64 -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_PREFIX_PATH="E:\Tools\vcpkg\installed\x64-windows" -DUSE_SDL2_LIBS=ON -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install
cmake --build x64 --config Release --target install
```

Options
You have to add vcpkg toolchain for ffmpeg, pthread
```
-DCMAKE_TOOLCHAIN_FILE="D:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

build type
```
-DCMAKE_BUILD_TYPE=Release
```

install path
```
-DCMAKE_INSTALL_PREFIX=install
```

build without IDE tools
If you remove the -G option, it will be generate cmake build environment. 
```
cmake -B windows -A x64 -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_PREFIX_PATH="E:\Tools\vcpkg\installed\x64-windows" -DUSE_SDL2_LIBS=ON -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install
cmake --build windows --config Release --target install
```

## Build for Linux
```
export OUT_PATH=./install
cmake .. -G "Unix Makefiles" \
	-DUSE_SDL2_LIBS=ON \
	-DCMAKE_VERBOSE_MAKEFILE=ON \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=${OUT_PATH}
cmake --build linux --config Release --target ${OUT_PATH}
```

## Executable binary
How to usage 

-r: RTSP Camera or Video Server URL like on wowza media server rtsp://<server ip>:1935/vod/sample.mp4
-u: account for authentication using username and password
-i: Interleaved mode for TCP channel for single port for audio, video and texture data
-U: Transport upstream mode using RTSP ANNOUNCE and RECORD option. this mode support video upload to media server to media.

./rtspclient_with_opengl -r <rtsp url> -u username password -i -Unix

```
./rtspclient_with_opengl -r rtsp://example.com:1935/vod/sample.mp4 -u admin password
```


## Reference
http://roxlu.com/2014/039/decoding-h264-and-yuv420p-playback
http://www.voidcn.com/article/p-tlcacfpn-bs.html
https://github.com/sipsorcery/mediafoundationsamples/tree/master/MFWebCamRtp
http://en.pudn.com/Download/item/id/1303846.html
http://blog.chinaunix.net/uid-15063109-id-4482932.html
https://stackoverflow.com/questions/34619418/ffmpeg-does-not-decode-some-h264-streams
https://stackoverflow.com/questions/17579286/sdl2-0-alternative-for-sdl-overlay
https://github.com/sipsorcery/mediafoundationsamples/blob/master/MFWebCamRtp/MFWebCamRtp.cpp
https://stackoverflow.com/questions/19427576/live555-x264-stream-live-source-based-on-testondemandrtspserver

H.264 Decoder using ffmpeg
https://gist.github.com/roxlu/9329339
https://github.com/tzyluen/h.264tzy/blob/master/cpp/H264_Decoder.cpp
https://github.com/xiongziliang/ZLMediaKit
https://github.com/royshil/KinectAtHomeExtension
https://github.com/flowerinthenight/ffmpeg-encode-h264mp4/tree/master/H264Encoder
https://codegists.com/snippet/c/h264_decodercpp_alenstar_c
https://github.com/MrKepzie/openfx-io/blob/master/FFmpeg/FFmpegFile.h
https://github.com/ChaoticConundrum/h264-roi/blob/master/zh264decoder.cpp
https://github.com/shengbinmeng/ffmpeg-h264-dec

Tinylib
http://tiny-lib.readthedocs.io/en/latest/guide.html#introduction-to-tiny-lib

https://www.bountysource.com/issues/46787191-can-not-find-ffmpeg-by-find_package
