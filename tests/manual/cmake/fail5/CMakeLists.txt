
cmake_minimum_required(VERSION 2.8)

project(pass5)

find_package(Qt5Core REQUIRED)

include_directories(${Qt5Core_INCLUDE_DIRS})

# Test options. The -i option removes the include "myobject.h" from the moc file
# causing a compile failure. -> Options work
qt5_wrap_cpp(moc_files myobject.h OPTIONS -i)

add_executable(myobject myobject.cpp ${moc_files})
target_link_libraries(myobject ${Qt5Core_LIBRARIES})
