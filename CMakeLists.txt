cmake_minimum_required(VERSION 3.23)
project(AutoManipulateFile)

set(CMAKE_CXX_STANDARD 17)
find_package(libssh CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_executable(AutoManipulateFile src/main.cpp src/ConfigReader/ConfigReader.h src/ConfigReader/ConfigReader.cpp src/ConsolePrinter/ConsolePrinter.h src/ConsolePrinter/ConsolePrinter.cpp src/YamlComment/YamlCommentSaver.cpp src/YamlComment/YamlCommentSaver.h src/YamlComment/YamlCommentPrinter.cpp src/YamlComment/YamlCommentPrinter.h)
target_link_libraries(AutoManipulateFile PRIVATE ssh)
target_link_libraries(AutoManipulateFile PRIVATE yaml-cpp)


