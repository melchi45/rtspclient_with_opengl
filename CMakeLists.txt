cmake_minimum_required(VERSION 2.8)

set ( PROJECT_NAME rtspclient_with_opengl )
set ( LIVE555 live555 )
#set ( SDL2 sdl2 )

# https://www.microsoft.com/en-us/download/details.aspx?id=6812
# set DXSDK_DIR=C:\Tools/DXSDK
# cmake . -Bstatic -G "Visual Studio 15 2017" -DCMAKE_INSTALL_PREFIX=install -DEXTRA_LDFLAGS="vcruntimed.lib;ucrtd.lib"
# cmake --build static --config Release
message ( $ENV{DXSDK_DIR} )

project( ${PROJECT_NAME} )

add_subdirectory( ${LIVE555} )
#add_subdirectory( ${SDL2} )
#add_subdirectory( ${PROJECT_NAME} )


set ( PROJECT_NAME rtspclient_with_opengl )
set ( ${PROJECT_NAME}_HOME ${CMAKE_CURRENT_SOURCE_DIR} )
message("SourceDir: " ${${PROJECT_NAME}_HOME} )
set ( ${LIVE555}_HOME ${${PROJECT_NAME}_HOME}/${LIVE555} )

message ( ${${LIVE555}_HOME} )

if ( CMAKE_COMPILER_IS_GNUCXX )
   set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive" )
endif ()

if ( NOT DEFINED BUILD_SHARED_LIBS )
   set ( BUILD_SHARED_LIBS TRUE )
endif ()

include_directories ( ${${LIVE555}_HOME}/UsageEnvironment/include )
include_directories ( ${${LIVE555}_HOME}/BasicUsageEnvironment/include )
include_directories ( ${${LIVE555}_HOME}/groupsock/include )
include_directories ( ${${LIVE555}_HOME}/liveMedia/include )

# set where to find additional cmake modules if any
# comment it out if not required
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
message ( ${CMAKE_MODULE_PATH} )

# set compiler support for C++11 standard
option(USE_CXX11_STD "Product should be build with C++11 compiler option enabled" ON)

if(USE_CXX11_STD)
    set(CMAKE_CXX_STANDARD 11)
endif()

find_library(OpenGL REQUIRED)

find_package(FFmpeg COMPONENTS AVCODEC AVFORMAT AVUTIL AVDEVICE REQUIRED)

if(FFMPEG_FOUND)
#  FFMPEG_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
#  FFMPEG_LIBRARIES     - Link these to use the required ffmpeg components.
#  FFMPEG_DEFINITIONS   - Compiler switches required for using the required ffmpeg components.
    message("FFMPEG_INCLUDE_DIRS = ${FFMPEG_INCLUDE_DIRS} ")
    message("FFMPEG_LIBRARIES = ${FFMPEG_LIBRARIES} ")
    message("FFMPEG_DEFINITIONS = ${FFMPEG_DEFINITIONS} ")

    include_directories(${FFMPEG_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "FFMPEG not found")
endif()

find_package(X264 REQUIRED)
if(X264_FOUND)
	include_directories(${X264_INCLUDE_DIRS})
endif()

#find_package(SDL2 REQUIRED)
#include_directories(${SDL2_INCLUDE_DIR})

#find_package(SDL2_ttf REQUIRED)
#include_directories(${SDL2_TTF_INCLUDE_DIR})

#find_package(SDL2_image REQUIRED)
#include_directories(${SDL2_IMAGE_INCLUDE_DIR})

if ( WIN32 )
   list ( APPEND ${PROJECT_NAME}_DEFINITIONS
      -DWINNT
   )
else ()
   list ( APPEND ${PROJECT_NAME}_DEFINITIONS
      -DSOCKLEN_T=socklen_t
   )
   if ( APPLE )
      list ( APPEND ${PROJECT_NAME}_DEFINITIONS
         -DHAVE_SOCKADDR_LEN=1
         -DTIME_BASE=int
      )
   endif ()
endif ()
add_definitions ( ${${PROJECT_NAME}_DEFINITIONS} )

if(MSVC)
    add_definitions(/FI"export.h")
	add_definitions(-DWITH_OPENCV2)
else()
    # GCC or Clang
    add_definitions(-include export.h)
endif()

# add library path
if ( WIN32 )
	link_directories ( ${CMAKE_BINARY_DIR}\\${LIVE555}\\${CMAKE_BUILD_TYPE} )
else()
	link_directories( ${CMAKE_BINARY_DIR}/${LIVE555} )
endif()

set ( ${PROJECT_NAME}_LINK_LIBRARIES "" )
if ( WIN32 )
   list ( APPEND ${PROJECT_NAME}_LINK_LIBRARIES
      ws2_32
   )
endif ()
#target_link_libraries ( ${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_LINK_LIBRARIES} )

add_executable(
    ${PROJECT_NAME}
        "src/rtspclient_with_opengl.cpp"
)
target_link_libraries(${PROJECT_NAME} ${LIVE555} )
target_include_directories( ${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIR} )
#target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES} )
#target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY} )

if ( BUILD_SHARED_LIBS )
	target_compile_definitions(${PROJECT_NAME} PRIVATE LIVEMEDIA_IMPORTS)
endif ()