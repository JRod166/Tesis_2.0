project( Calibration_Ring )

message( "Configuring main application: " ${PROJECT_NAME} )

file(GLOB SOURCES "*.cpp" )
file(GLOB HEADERS "*.h" )

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
message( "RUNTIME Output: " "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" )
message( "EXECUTABLE Output: " "${CMAKE_EXECUTABLE_OUTPUT_DIRECTORY}" )

find_package( OpenCV REQUIRED core imgproc calib3d highgui )
#find_package( OpenCV REQUIRED )
#find_package( Threads REQUIRED )

###################################################
# Set dependencies CMINPACK SDK
###################################################
# Requires CMINPACK libraries
if( WIN32 )
	set( CMINPACK_SDK_PATH 
	${CMAKE_SOURCE_DIR}/dep/cminpack )
	message(STATUS "CMINPACK_SDK_PATH: "  ${CMINPACK_SDK_PATH} )
	if(CMINPACK_SDK_PATH)
		file( COPY ${CMINPACK_SDK_PATH}/lib/Release/cminpack.dll
			DESTINATION ${CMAKE_SOURCE_DIR}/bin/Release)
		file( COPY ${CMINPACK_SDK_PATH}/lib/Debug/cminpack_d.dll
			DESTINATION ${CMAKE_SOURCE_DIR}/bin/Debug)
		set( CMINPACK_INCLUDE_DIRS ${CMINPACK_SDK_PATH}/include )
		set( CMINPACK_LIBRARY_DIRS ${CMINPACK_SDK_PATH}/lib )
	
		set( CMINPACK_LIBRARY_RELEASE
		optimized ${CMINPACK_LIBRARY_DIRS}/Release/cminpack.lib	)
		set( CMINPACK_LIBRARY_DEBUG
		debug ${CMINPACK_LIBRARY_DIRS}/Debug/cminpack_d.lib	)
		
		message(STATUS "\nCMINPACK_INCLUDE_DIRS: " ${CMINPACK_INCLUDE_DIRS} )
		message(STATUS "\nCMINPACK_LIBRARY_DIRS: " ${CMINPACK_LIBRARY_DIRS} )
		message(STATUS "\nCMINPACK_LIBRARY_RELEASE: " ${CMINPACK_LIBRARY_RELEASE} )
		message(STATUS "\nCMINPACK_LIBRARY_DEBUG: " ${CMINPACK_LIBRARY_DEBUG} )
		
		set(INFO_USE_CMINPACK_SDK "Yes")
		message( STATUS "\nINFO_USE_CMINPACK_SDK => YES" )
	else(CMINPACK_SDK_PATH)
		set(INFO_USE_CMINPACK_SDK "\nNo (SDK not found)")
		message("\nINFO_USE_CMINPACK_SDK => NO (SDK not found)" )
	endif(CMINPACK_SDK_PATH)   	
else( WIN32 ) #Testing in direct network connection
  	message(ERROR "\nCMINPACK set to Linux!")
endif( WIN32 )


set( DEPENDENCY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Calibration_Ring/)

include_directories( 
${DEPENDENCY_DIR}/ring/ 
${OpenCV_INCLUDE_DIRS} )
 
add_executable( ${PROJECT_NAME}  
				${HEADERS} 
				${SOURCES})
				
target_link_libraries( ${PROJECT_NAME} 
                       ${OpenCV_LIBS}
					   ${CMINPACK_LIBRARY_RELEASE}
					   ${CMINPACK_LIBRARY_DEBUG} 
					)
					   
                           