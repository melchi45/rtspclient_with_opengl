cmake_minimum_required(VERSION 2.8)

set ( PROJECT_NAME rtspclient_with_opengl )
set ( TEST_DECODE testDecode )
set ( LIVE555 live555 )
set ( TINYLIB tinylib )

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

#set ( PROJECT_NAME rtspclient_with_opengl )
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

#message("LM_SENSORS include dir = ${LM_SENSORS_INCLUDE_DIR}")

message ( "LIVE555_HOME = ${${LIVE555}_HOME}" )

set ( ${LIVE555}_USAGE_ENVIRONMENT_HEADER_DIR ${${LIVE555}_HOME}/UsageEnvironment/include )
set ( ${LIVE555}_BASIC_USAGE_ENVIRONMENT_HEADER_DIR ${${LIVE555}_HOME}/BasicUsageEnvironment/include )
set ( ${LIVE555}_GROUP_SOCK_HEADER_DIR ${${LIVE555}_HOME}/groupsock/include )
set ( ${LIVE555}_LIVE_MEDIA_HEADER_DIR ${${LIVE555}_HOME}/liveMedia/include )

file(GLOB LIVE555_HEADER_FILES ${${LIVE555}_USAGE_ENVIRONMENT_HEADER_DIR} ${${LIVE555}_BASIC_USAGE_ENVIRONMENT_HEADER_DIR} 
${${LIVE555}_GROUP_SOCK_HEADER_DIR} ${${LIVE555}_LIVE_MEDIA_HEADER_DIR} )

message ( "LIVE555_HEADER_FILES = ${LIVE555_HEADER_FILES}" )

#if ( WIN32 )
set ( THREADS_USE_PTHREADS_WIN32 TRUE )
find_package(Threads REQUIRED)

message ( "THREADS_PTHREADS_INCLUDE_DIR = ${THREADS_PTHREADS_INCLUDE_DIR}" )
message ( "THREADS_PTHREADS_WIN32_LIBRARY = ${THREADS_PTHREADS_WIN32_LIBRARY}" )
message ( "CMAKE_THREAD_LIBS_INIT = ${CMAKE_THREAD_LIBS_INIT}" )

#endif ( WIN32)

#include_directories(${THREADS_PTHREADS_INCLUDE_DIR})
#target_link_libraries(foo ${CMAKE_THREAD_LIBS_INIT})

#find_package(GLUT)

find_package(FFmpeg REQUIRED)
if(FFMPEG_FOUND)
  message("Found FFMPEG/LibAV libraries")
  message("FFMPEG_INCLUDE_DIRS = ${FFMPEG_INCLUDE_DIRS} ")
  message("FFMPEG_LIBRARIES = ${FFMPEG_LIBRARIES} ")
  message("FFMPEG_DEFINITIONS = ${FFMPEG_DEFINITIONS} ")
  
  if( SWSCALE_FOUND )
	message("Found SWSCALE_LIBRARIES = ${SWSCALE_LIBRARIES}")
  endif()
  
  if( SWRESAMPLE_FOUND )
	message("Found SWRESAMPLE_LIBRARIES = ${SWRESAMPLE_LIBRARIES}")
  endif()
  
  if( AVDEVICE_FOUND )
	message("Found AVDEVICE_LIBRARIES = ${AVDEVICE_LIBRARIES}")
  endif()
  
  if( AVFILTER_FOUND )
	message("Found AVFILTER_LIBRARIES = ${AVFILTER_LIBRARIES}")
  endif()
  
  if( POSTPROC_FOUND )
	message("Found POSTPROC_LIBRARIES = ${POSTPROC_LIBRARIES}")
  endif()
else( FFMPEG_FOUND )
    message(FATAL_ERROR "FFMPEG not found")
endif( FFMPEG_FOUND )


#find_package(X264 REQUIRED)
#if(X264_FOUND)
#	include_directories(${X264_INCLUDE_DIRS})
#endif()

if ( NOT DEFINED USE_SDL2_LIBS )
   set ( USE_SDL2_LIBS TRUE )
endif ()

if ( USE_SDL2_LIBS )
  message(STATUS "Use SDL2 Library for Renderer windows")
  set(USE_SDL2_LIBS TRUE)
  find_package(SDL2 REQUIRED)
  if(SDL2_FOUND)
    #include_directories(${SDL2_INCLUDE_DIR})
    message("Found SDL2 libraries")
    message("SDL2_INCLUDE_DIR = ${SDL2_INCLUDE_DIR} ")
    message("SDL2_LIBRARY = ${SDL2_LIBRARY} ")
  endif ()
  
  #find_package(SDL2_ttf REQUIRED)
  #include_directories(${SDL2_TTF_INCLUDE_DIR})

  #find_package(SDL2_image REQUIRED)
  #include_directories(${SDL2_IMAGE_INCLUDE_DIR})

  add_definitions(-DUSE_SDL2_LIB)
else ( )
  set(USE_GLFW_LIB TRUE)
  message(STATUS "Use OpenGL/GLFW Library for Renderer windows")
  find_package(OpenGL REQUIRED)
  if(OPENGL_FOUND)
    message("OPENGL_LIBRARIES = ${OPENGL_LIBRARIES} ")
    message("OPENGL_INCLUDE_DIRS = ${OPENGL_INCLUDE_DIRS} ")
  endif()

  find_package(GLEW REQUIRED) # First try to find using the find_package method (works with vcpkg)
  if(GLEW_FOUND) # Works with some other methods
  	message(STATUS "GLEW_INCLUDE_PATH=${GLEW_INCLUDE_PATH}")
  	message(STATUS "GLEW_LIBRARIES=${GLEW_LIBRARIES}")
  endif()
  
  find_package(GLFW3 REQUIRED) # First try to find using the find_package method (works with vcpkg)
  if(GLFW_FOUND) # Works with some other methods
  	message(STATUS "GLFW3_INCLUDE_PATH=${GLFW3_INCLUDE_PATH}")
  	message(STATUS "GLFW_LIBRARIES=${GLFW_LIBRARIES}")
  endif()
  
  #message (STATUS "TINYLIB_HOME = ${${TINYLIB}_HOME}" )
  find_path(
    TINYLIB_INCLUDE_DIR
    NAMES "tinylib.h"
    PATHS "${${TINYLIB}_HOME}/src"
  )
  
  # Define GLFW_LIBRARIES and GLFW_INCLUDE_DIRS
  if ( DEFINED TINYLIB_INCLUDE_DIR )
  	set(TINYLIB_INCLUDE_DIRS ${TINYLIB_INCLUDE_DIR})
  	add_definitions(-DROXLU_USE_LOG)
  	add_definitions(-DROXLU_USE_MATH)
  	add_definitions(-DROXLU_USE_PNG)
  	add_definitions(-DROXLU_USE_OPENGL)
  	add_definitions(-DROXLU_IMPLEMENTATION)
  	message(STATUS "TINYLIB_INCLUDE_DIRS include dir = ${TINYLIB_INCLUDE_DIRS}")
	
	find_package(PNG REQUIRED)
	if(PNG_FOUND)
      message(STATUS "PNG library = ${PNG_LIBRARIES}")
	endif()
  endif()
 
  add_definitions(-DUSE_GLFW_LIB)
endif ( )


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
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
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
link_directories ( ${CMAKE_LIBRARY_PATH} )

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
file(GLOB HDR_FILES ${PROJECT_SOURCE_DIR}/src/*${HDR_EXT_PATTERN} )

# makes source and header files group
source_group("Source Files" FILES ${SRC_FILES})
source_group("Header Files" FILES ${HDR_FILES})

# build target
add_executable(${PROJECT_NAME} ${SRC_FILES} ${HDR_FILES})

target_include_directories( ${PROJECT_NAME} PRIVATE ${LIVE555_HEADER_FILES} )
target_include_directories( ${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS} )
target_include_directories( ${PROJECT_NAME} PRIVATE ${THREADS_PTHREADS_INCLUDE_DIR} )

if(USE_GLFW_LIB)
target_include_directories( ${PROJECT_NAME} PRIVATE ${${TINYLIB}_HOME}/src )
target_include_directories( ${PROJECT_NAME} PRIVATE ${OPENGL_INCLUDE_DIRS} )
endif ()

if(USE_SDL2_LIBS)
target_include_directories( ${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIR} )
endif ()

target_link_libraries( ${PROJECT_NAME} PRIVATE ${LIVE555} )
target_link_libraries( ${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARIES} )
target_link_libraries( ${PROJECT_NAME} PRIVATE ${SWSCALE_LIBRARIES} )
target_link_libraries( ${PROJECT_NAME} PRIVATE ${CMAKE_THREAD_LIBS_INIT})

if(USE_GLFW_LIB)
target_link_libraries( ${PROJECT_NAME} PRIVATE ${GLFW_LIBRARIES} )
target_link_libraries( ${PROJECT_NAME} PRIVATE ${GLEW_LIBRARIES} )
target_link_libraries( ${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES} )
endif ()

if(USE_SDL2_LIBS)
target_link_libraries( ${PROJECT_NAME} PRIVATE ${SDL2_LIBRARY} )
endif ()

if(PNG_FOUND)
target_link_libraries( ${PROJECT_NAME} PRIVATE ${PNG_LIBRARIES})
endif()

if ( BUILD_SHARED_LIBS )
	target_compile_definitions(${PROJECT_NAME} PRIVATE LIVEMEDIA_IMPORTS)
else ( LIVEMEDIA_STATIC )
	target_compile_definitions(${PROJECT_NAME} PRIVATE LIVEMEDIA_STATIC)
endif ()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test.264
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# get all *.cpp files from src to build target
file(GLOB TEST_DECODER_SRC_FILES
	${PROJECT_SOURCE_DIR}/test/*${CXX_SRC_EXT_PATTERN}
	${PROJECT_SOURCE_DIR}/src/H264Decoder.cpp
	${PROJECT_SOURCE_DIR}/src/log_utils.c
)
file(GLOB TEST_DECODER_HDR_FILES
	${PROJECT_SOURCE_DIR}/src/H264Decoder.h
	${PROJECT_SOURCE_DIR}/src/log_utils.h
)
# makes source and header files group
source_group("Source Files" FILES ${TEST_DECODER_SRC_FILES})
source_group("Header Files" FILES ${TEST_DECODER_HDR_FILES})

#add_executable( ${TEST_DECODE} ${TEST_DECODER_SRC_FILES} ${TEST_DECODER_HDR_FILES} )
#target_include_directories( ${TEST_DECODE} PRIVATE ${LIVE555_HEADER_FILES} )
#target_include_directories( ${TEST_DECODE} PRIVATE ${FFMPEG_INCLUDE_DIRS} )
#target_include_directories( ${TEST_DECODE} PRIVATE ${PROJECT_SOURCE_DIR}/src)
#target_include_directories( ${TEST_DECODE} PRIVATE ${GLFW3_INCLUDE_PATH} )
#target_include_directories( ${TEST_DECODE} PRIVATE ${${TINYLIB}_HOME}/src )
#target_link_libraries( ${TEST_DECODE} ${FFMPEG_LIBRARIES} )
#target_link_libraries( ${TEST_DECODE} ${GLFW_LIBRARIES} )

#-r rtsp://192.168.123.37/profile5/media.smp -u admin 1

if (EXECUTE_RTSP_CLIENT)
add_custom_command (
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${PROJECT_NAME} -r rtsp://192.168.123.37/profile5/media.smp -u admin 1
    COMMENT "Execute ${PROJECT_NAME}"
)
endif ()

#add_custom_command (
#    TARGET ${TEST_DECODE}
#    POST_BUILD
#    COMMAND ${TEST_DECODE}
#    COMMENT "Execute ${TEST_DECODE}"
#)