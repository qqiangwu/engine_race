cmake_minimum_required(VERSION 3.12)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

file(GLOB_RECURSE headers "*.h")
file(GLOB_RECURSE impl "*.cpp")

add_library(purekv STATIC ${headers} ${impl})
target_include_directories(purekv PUBLIC include)
target_include_directories(purekv PRIVATE include/purekv)

target_compile_features(purekv PUBLIC cxx_std_17)

target_compile_options(purekv PRIVATE -Wfatal-errors)

target_compile_definitions(purekv PRIVATE
        BOOST_THREAD_PROVIDES_FUTURE
        BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION)

find_package(Boost COMPONENTS system thread REQUIRED)
if(NOT ${Boost})
    message(FATAL "boost not found")
endif()

target_include_directories(purekv PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(purekv PRIVATE ${Boost_LIBRARIES} CONAN_PKG::fmt CONAN_PKG::spdlog)