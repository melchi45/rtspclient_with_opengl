cmake_minimum_required(VERSION 2.8)

set ( PROJECT_NAME rtspclient_with_opengl )
set ( LIVE555 live555 )
set ( TINYLIB tinylib )
#set ( SDL2 sdl2 )

# https://www.microsoft.com/en-us/download/details.aspx?id=6812
# set DXSDK_DIR=C:\Tools/DXSDK
# cmake . -Bstatic -G "Visual Studio 15 2017" -DCMAKE_INSTALL_PREFIX=install -DEXTRA_LDFLAGS="vcruntimed.lib;ucrtd.lib"
# cmake --build static --config Release
#message ( $ENV{DXSDK_DIR} )

project( ${PROJECT_NAME} )

if ( NOT DEFINED BUILD_SHARED_LIBS )
   set ( BUILD_SHARED_LIBS FALSE )
endif ()

add_subdirectory( ${LIVE555} )
#add_subdirectory( ${SDL2} )
#add_subdirectory( ${PROJECT_NAME} )


set ( PROJECT_NAME rtspclient_with_opengl )
set ( ${PROJECT_NAME}_HOME ${CMAKE_CURRENT_SOURCE_DIR} )
message("SourceDir: " ${${PROJECT_NAME}_HOME} )

if ( CMAKE_COMPILER_IS_GNUCXX )
   set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive" )
endif ()

# set where to find additional cmake modules if any
# comment it out if not required
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
message ( "CMAKE_MODULE_PATH = ${CMAKE_MODULE_PATH}" )

# set compiler support for C++11 standard
option(USE_CXX11_STD "Product should be build with C++11 compiler option enabled" ON)

if(USE_CXX11_STD)
    set(CMAKE_CXX_STANDARD 11)
endif()

set ( ${LIVE555}_HOME ${${PROJECT_NAME}_HOME}/${LIVE555} )
set ( ${TINYLIB}_HOME ${${PROJECT_NAME}_HOME}/${TINYLIB} )

MESSAGE ("TINYLIB_HOME = ${${TINYLIB}_HOME}" )
FIND_PATH(TINYLIB_INCLUDE_DIRS tinylib.h
  HINTS
    ${PC_LIBTINYLIB_INCLUDEDIR}
    ${PC_LIBTINYLIB_INCLUDE_DIRS}
  PATH_SUFFIXES
    ${${TINYLIB}_HOME}/src
)
message("TINYLIB_INCLUDE_DIRS include dir = ${TINYLIB_INCLUDE_DIRS}")

#message("LM_SENSORS include dir = ${LM_SENSORS_INCLUDE_DIR}")

message ( "LIVE555_HOME = ${${LIVE555}_HOME}" )

set ( ${LIVE555}_USAGE_ENVIRONMENT_HEADER_DIR ${${LIVE555}_HOME}/UsageEnvironment/include )
set ( ${LIVE555}_BASIC_USAGE_ENVIRONMENT_HEADER_DIR ${${LIVE555}_HOME}/BasicUsageEnvironment/include )
set ( ${LIVE555}_GROUP_SOCK_HEADER_DIR ${${LIVE555}_HOME}/groupsock/include )
set ( ${LIVE555}_LIVE_MEDIA_HEADER_DIR ${${LIVE555}_HOME}/liveMedia/include )

file(GLOB LIVE555_HEADER_FILES ${${LIVE555}_USAGE_ENVIRONMENT_HEADER_DIR} ${${LIVE555}_BASIC_USAGE_ENVIRONMENT_HEADER_DIR} 
${${LIVE555}_GROUP_SOCK_HEADER_DIR} ${${LIVE555}_LIVE_MEDIA_HEADER_DIR} )

message ( "LIVE555_HEADER_FILES = ${LIVE555_HEADER_FILES}" )

if ( WIN32 )
set ( THREADS_USE_PTHREADS_WIN32 TRUE )
find_package(Threads REQUIRED)

message ( "THREADS_PTHREADS_INCLUDE_DIR = ${THREADS_PTHREADS_INCLUDE_DIR}" )
message ( "THREADS_PTHREADS_WIN32_LIBRARY = ${THREADS_PTHREADS_WIN32_LIBRARY}" )
message ( "CMAKE_THREAD_LIBS_INIT = ${CMAKE_THREAD_LIBS_INIT}" )

endif ( WIN32)

#include_directories(${THREADS_PTHREADS_INCLUDE_DIR})
#target_link_libraries(foo ${CMAKE_THREAD_LIBS_INIT})

find_library(OpenGL REQUIRED)

find_package(FFmpeg COMPONENTS AVCODEC AVFORMAT AVUTIL AVDEVICE REQUIRED)
#find_package(FFmpeg REQUIRED)

if(FFMPEG_FOUND)
  message("Found FFMPEG/LibAV libraries")
  message("FFMPEG_INCLUDE_DIRS = ${FFMPEG_INCLUDE_DIRS} ")
  message("FFMPEG_LIBRARIES = ${FFMPEG_LIBRARIES} ")
  message("FFMPEG_DEFINITIONS = ${FFMPEG_DEFINITIONS} ")

#  include_directories(${FFMPEG_INCLUDE_DIRS})
  
#  include_directories(${FFMPEG_INCLUDE_DIRS})
#  target_link_libraries (${PROJECT_NAME} ${FFMPEG_LIBRARIES})
#  FFMPEG_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
#  FFMPEG_LIBRARIES     - Link these to use the required ffmpeg components.
#  FFMPEG_DEFINITIONS   - Compiler switches required for using the required ffmpeg components.
else()
    message(FATAL_ERROR "FFMPEG not found")
endif()

#find_package(X264 REQUIRED)
#if(X264_FOUND)
#	include_directories(${X264_INCLUDE_DIRS})
#endif()

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
	add_definitions(-D_CONSOLE)
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

# use standard file endings - rename to cxx if needed
set(CXX_SRC_EXT_PATTERN ".cpp")
set(C_SRC_EXT_PATTERN ".c")
set(HDR_EXT_PATTERN ".h")

# get all *.cpp files from src to build target
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*${CXX_SRC_EXT_PATTERN} ${PROJECT_SOURCE_DIR}/src/*${C_SRC_EXT_PATTERN})
file(GLOB HDR_FILES ${PROJECT_SOURCE_DIR}/src/*${HDR_EXT_PATTERN})
# makes source and header files group
source_group("Source Files" FILES ${SRC_FILES})
source_group("Header Files" FILES ${HDR_FILES})

# build target
add_executable(${PROJECT_NAME} ${SRC_FILES} ${HDR_FILES})

target_include_directories( ${PROJECT_NAME} PRIVATE ${LIVE555_HEADER_FILES} )
target_include_directories( ${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS} )
#target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${SDL2_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${THREADS_PTHREADS_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${${TINYLIB}_HOME}/src)

target_link_libraries(${PROJECT_NAME} ${LIVE555} )
target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES} )
#target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARY} )
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

if ( BUILD_SHARED_LIBS )
	target_compile_definitions(${PROJECT_NAME} PRIVATE LIVEMEDIA_IMPORTS)
else ( LIVEMEDIA_STATIC )
	target_compile_definitions(${PROJECT_NAME} PRIVATE LIVEMEDIA_STATIC)
endif ()