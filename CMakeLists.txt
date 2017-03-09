cmake_minimum_required (VERSION 2.8)

project (Go-CamRecorder)

include(CMakeModule/DomsModule.cmake)

# Set variable
set(CWD ${CMAKE_CURRENT_SOURCE_DIR})
set(PARENTWD ${CMAKE_CURRENT_SOURCE_DIR}/..)

# If USE_KINECT is defined
if(DEFINED USE_KINECT)
	MESSAGE( STATUS "USE_KINECT defined. Check if Kinect is installed/configured." )
	if( NOT CMAKE_SYSTEM MATCHES "Windows")
		MESSAGE( FATAL_ERROR "Kinect is supported only on Windows in this version." )
	endif()
	if ( NOT DEFINED ENV{KINECTSDK10_DIR} )
		MESSAGE( FATAL_ERROR "KINECTSDK10_DIR env variable not defined. Reinstall Kinect SDK of manually define this variable (if ths SDK is installed)." )
	endif()
	add_definitions(-DGO_CAM_KINECT_VERSION -DKINECT_1)
	# Windows include, in future checl here for libfreenect version
	include_directories("$ENV{KINECTSDK10_DIR}/inc/")
	set(Kinect_LIBS "$ENV{KINECTSDK10_DIR}/lib/amd64/Kinect10.lib")

	include_directories("Kinect/")#include "RecordingManagement.h"
	set(Kinect_SRC "./Kinect/KinectSensor-v1.cpp" "./Kinect/KinectSensor.cpp"
				   "./Kinect/VideoKinectRecording.cpp"  "./Kinect/KinectRecording.cpp"
				   "./Kinect/RecordingManagement.cpp" "./Kinect/RecordingContextFactory.cpp")
    set(Kinect_HDRS "./Kinect/KinectSensorCommon.h" "./Kinect/KinectSensor-v1.h" "./Kinect/KinectSensor.h" 
					"./Kinect/VideoKinectRecording.h" "./Kinect/KinectRecording.h"
					"./Kinect/RecordingManagement.h" "./Kinect/RecordingContextFactory.h")
else()
	MESSAGE( STATUS "USE_KINECT not defined. Webcam mode." )
	set(Kinect_LIBS "")
endif()

if ( MSVC )
	foreach(flag_var
	        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
	        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
	   if (${flag_var} MATCHES "/MD")
	      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
	   endif()
	endforeach(flag_var)
	
	# Add performance profil to Release compilation
	AddFlag( CMAKE_CXX_FLAGS_RELEASE "/O2" )
	AddFlag( CMAKE_CXX_FLAGS_RELEASE "/Ob2" )
	AddFlag( CMAKE_CXX_FLAGS_RELEASE "/Oi" )
	AddFlag( CMAKE_CXX_FLAGS_RELEASE "/Ot" )
	AddFlag( CMAKE_CXX_FLAGS_RELEASE "/Oy" )
	AddFlag( CMAKE_CXX_FLAGS_RELEASE "/GL" )
	# MESSAGE ( ${CMAKE_CXX_FLAGS_RELEASE} )
	
	# add flag to remove warning about usage of non secure versions of fopen, fprintf...
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)

	# Multi processor flag
	AddFlag( CMAKE_CXX_FLAGS_DEBUG "/MP" )
	AddFlag( CMAKE_CXX_FLAGS_RELEASE "/MP" )
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=c++11 -w -fpermissive -D_FILE_OFFSET_BITS=64)
endif()

set(Omiscid_DIR "./Omiscid/")
find_package( Omiscid REQUIRED COMPONENTS Messaging )

find_package( OpenCV REQUIRED )
include_directories(${OpenCV_INCLUDE_DIRS})

include_directories(${SRCS}/DataManagement)
AddSourceInGroup( ${CWD} "./DataManagement/" DataManagement_SRC)
file(GLOB DataManagement_HDRS RELATIVE ${CWD}
	"${CWD}/DataManagement/*.h"
)


# Add main files from the project
file(GLOB SRCS RELATIVE ${CWD}
	"${CWD}/*.cpp"
)

file(GLOB HDRS RELATIVE ${CWD}
	"${CWD}/*.h"
)

add_executable(Go-CamRecorder MainGo.cpp ${SRCS} ${HDRS} ${Omiscid_SRCS} ${Omiscid_HDRS} ${DataManagement_SRC} ${Kinect_HDRS} ${Kinect_SRC})
add_dependencies(Go-CamRecorder Omiscid)
target_link_libraries(Go-CamRecorder ${OpenCV_LIBS} ${Kinect_LIBS})

if ( MSVC )
	target_link_libraries(Go-CamRecorder ws2_32.lib)
endif()

# Microsoft specific case, no effect on other systems
source_group("DataManagement" FILES ${DataManagement_SRC} ${DataManagement_HDRS})
if(DEFINED USE_KINECT)
	source_group("Kinect" FILES ${Kinect_SRC} ${Kinect_HDRS})
endif()
source_group("Omiscid\\System" FILES ${OmiscidSystem_SRCS} ${OmiscidSystem_HDRS})
source_group("Omiscid\\Messaging" FILES ${OmiscidMessaging_SRCS} ${OmiscidMessaging_HDRS})
