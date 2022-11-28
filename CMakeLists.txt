cmake_minimum_required(VERSION 2.8) 
PROJECT (realbugtracker)

# deps 
find_package(OpenCV 4.6 REQUIRED COMPONENTS core imgproc  )
find_package(BGSLibrary 3.2 REQUIRED )
find_package(LibArchive 3.4 REQUIRED )

# what cpp standard
set (CMAKE_CXX_STANDARD 14)

set( NAME_SRC_ECP
    src/extractcenterpoints.cpp
    src/ttoolbox.cpp
)

set( NAME_HEADERS       
     include/ttoolbox.h
)

set( NAME_SRC_MT
    src/mergetimeline.cpp
)

set( NAME_SRC_PT
    src/producetrajectory.cpp
)

#a fix link because of PATH Error
#https://github.com/andrewssobral/bgslibrary/issues/107
set(BGGFIXLINKLIB /usr/local/lib/libbgslibrary_core.so)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${LibArchive_INCLUDE_DIRS})
include_directories(${BGSLibrary_INCLUDE_DIRS})

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )
link_directories( ${CMAKE_BINARY_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)


#what do we link, defined a group 
#add_library(LibOpenCV INTERFACE)
#target_link_libraries(LibOpenCV INTERFACE ${OpenCV_LIBS}) 
#target_link_libraries(LibOpenCV INTERFACE -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_flann) 

#the main
add_executable( extractcenterpoints ${NAME_SRC_ECP} ${NAME_HEADERS} )
target_link_libraries( extractcenterpoints -larchive) 
#target_link_libraries( realbugtracker ${LibOpenCV})  
target_link_libraries( extractcenterpoints ${BGGFIXLINKLIB} ) 
target_link_libraries( extractcenterpoints ${OpenCV_LIBS}) 
target_link_libraries( extractcenterpoints -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_flann) 


add_executable( mergetimeline ${NAME_SRC_MT} )
#test stuff
target_link_libraries( mergetimeline -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_flann) 


add_executable( producetrajectory ${NAME_SRC_PT} )
#test stuff
target_link_libraries( producetrajectory -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_flann) 


