cmake_minimum_required(VERSION 2.8)

set ( PROJECT_NAME rtspclient_with_opengl )
set ( ${PROJECT_NAME}_HOME ${CMAKE_CURRENT_SOURCE_DIR} )
message("SourceDir: " ${${PROJECT_NAME}_HOME} )

set ( LIVE555 live555 )

set ( ${LIVE555}_HOME ${${PROJECT_NAME}_HOME}/../${LIVE555} )


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
target_link_libraries(${PROJECT_NAME} ${LIVE555})
if ( BUILD_SHARED_LIBS )
	target_compile_definitions(${PROJECT_NAME} PRIVATE LIVEMEDIA_IMPORTS)
endif ()
