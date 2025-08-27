cmake_minimum_required(VERSION 3.29)



project(Shapes VERSION 0.0.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(olcpixelgameengine CONFIG REQUIRED)
find_package(simple_serialization CONFIG REQUIRED)
find_package(pcg-cpp CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)

# Add source to this project's executable.
add_executable (${CMAKE_PROJECT_NAME} 
    src/main.cpp
    src/olcPixelGameEngine.cpp
    src/shape.cpp
    src/weapons/default_weapon.cpp
    src/weapons/weapons.cpp
    src/utilities/utility.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME}
  PRIVATE
  src
)

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)


target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
    olcpixelgameengine::olcpixelgameengine
    simple_serialization::simple_serialization
    pcg-cpp::pcg-cpp
    EnTT::EnTT
    tinyxml2::tinyxml2
    X11
    GL
    pthread
    png
    )
#check out the potential fix for c++20 here
#https://github.com/eliemichel/cpp20-cmake-emscripten-template


#Emscripten things
if (EMSCRIPTEN)
  #set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS ./cmake/EmscriptenScanDepsFix/emscan-deps CACHE FILEPATH "" FORCE)
  set(CMAKE_EXECUTABLE_SUFFIX .html)

  execute_process(COMMAND "${EMSCRIPTEN_ROOT_PATH}/embuilder${EMCC_SUFFIX}" build libpng)

  target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -sALLOW_MEMORY_GROWTH=1
    -sMAX_WEBGL_VERSION=2
    -sMIN_WEBGL_VERSION=2
    -sUSE_LIBPNG=1
    -sSINGLE_FILE
    --shell-file ${CMAKE_CURRENT_LIST_DIR}/basic_template.html
  )

  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    websocket.js
  )
endif()

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
