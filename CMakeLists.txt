cmake_minimum_required(VERSION 3.10)

project(concertina-pbqp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(llvm_libs support)

add_library(midifile STATIC
    midifile/src/Binasc.cpp
    midifile/src/MidiEvent.cpp
    midifile/src/MidiEventList.cpp
    midifile/src/MidiFile.cpp
    midifile/src/MidiMessage.cpp
    midifile/src/Options.cpp
)
target_include_directories(midifile PUBLIC midifile/include)

add_executable(concertina-pbqp main.cpp)
target_link_libraries(concertina-pbqp ${llvm_libs} midifile)
