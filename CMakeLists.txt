cmake_minimum_required(VERSION 4.2.3)
project(AuthoringPlugin CXX)

set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "Limited configurations" FORCE)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Set the build type to Release by default" FORCE)
mark_as_advanced(CMAKE_BUILD_TYPE CMAKE_CONFIGURATION_TYPES)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HOUDINI_INSTALL_PATH "C:/Program Files/Side Effects Software/Houdini 21.0.631")
set(HOUDINI_LIB_PATH "${HOUDINI_INSTALL_PATH}/custom/houdini/dsolib")

if (NOT EXISTS "${HOUDINI_INSTALL_PATH}")
    message(FATAL_ERROR "Houdini install path not found: ${HOUDINI_INSTALL_PATH}")
endif()

if (NOT EXISTS "${HOUDINI_LIB_PATH}")
    message(FATAL_ERROR "Houdini lib path not found: ${HOUDINI_LIB_PATH}")
endif()

include_directories(
    "${HOUDINI_INSTALL_PATH}/toolkit/include"
    "${HOUDINI_INSTALL_PATH}/custom/houdini/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "C:/eigen-5.0.0"
    "."
)

set(SOURCE_FILES
    src/cpp/SOP_AuthoringPlugin.cpp
    src/cpp/Strand.cpp
    src/cpp/StrandSet.cpp
    src/cpp/GuideSet.cpp
    src/cpp/GeometryImporter.cpp
    src/cpp/FeatureComputation.cpp
    src/cpp/GuideSmoothing.cpp
    src/cpp/DSO_AuthoringPlugin.cpp
)

set(HEADER_FILES
    include/SOP_AuthoringPlugin.h
    include/Strand.h
    include/StrandSet.h
    include/GuideSet.h
    include/GeometryImporter.h
    include/FeatureComputation.h
    include/GuideSmoothing.h
)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

link_directories(${HOUDINI_LIB_PATH})

file(GLOB HOUDINI_LIBS "${HOUDINI_LIB_PATH}/*.lib")
target_link_libraries(${PROJECT_NAME} PRIVATE ${HOUDINI_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VERSION="${HOUDINI_VERSION}"
    _USE_MATH_DEFINES
    SESI_LITTLE_ENDIAN
    SIZEOF_VOID_P=8
    MAKING_DSO
    HBOOST_ALL_NO_LIB
)

set(CUSTOM_DSO_DIR $ENV{CUSTOM_DSO_PATH})

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".dll"
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CUSTOM_DSO_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CUSTOM_DSO_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CUSTOM_DSO_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CUSTOM_DSO_DIR}"
)