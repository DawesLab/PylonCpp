cmake_minimum_required(VERSION 2.8)
project( FFTimage )
find_package( OpenCV REQUIRED )
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package( Boost 1.40 COMPONENTS program_options REQUIRED)

add_executable( FFTimage FFTimage.cpp )

include_directories( ${Boost_INCLUDE_DIRS} )
target_link_libraries( FFTimage ${Boost_LIBRARIES} )
target_link_libraries( FFTimage ${OpenCV_LIBRARIES} )
target_link_libraries( FFTimage picam )
include_directories( "/opt/PrincetonInstruments/picam/includes" )

