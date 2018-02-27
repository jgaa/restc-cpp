# Here are registered all external projects
#
# Usage:
# add_dependencies(TARGET externalProjectName)
# target_link_libraries(TARGET PRIVATE ExternalLibraryName)

set(EXTERNAL_PROJECTS_PREFIX ${CMAKE_BINARY_DIR}/external-projects)
set(EXTERNAL_PROJECTS_INSTALL_PREFIX ${EXTERNAL_PROJECTS_PREFIX}/installed)

include(ExternalProject)

ExternalProject_Add(
    externalRapidJson
    PREFIX "${EXTERNAL_PROJECTS_PREFIX}"
    GIT_REPOSITORY "https://github.com/Tencent/rapidjson.git"
    GIT_TAG "master"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory
        <SOURCE_DIR>/include ${CMAKE_BINARY_DIR}/installed/include
    LOG_DOWNLOAD ON
    LOG_INSTALL ON
    )

set(EXTERNAL_RAPIDJSON_INCLUDE_DIR ${CMAKE_BINARY_DIR}/installed/include/rapidjson)

ExternalProject_Add(
    externalLest
    PREFIX "${EXTERNAL_PROJECTS_PREFIX}"
    GIT_REPOSITORY "https://github.com/martinmoene/lest.git"
    GIT_TAG "master"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory
        <SOURCE_DIR>/include ${CMAKE_BINARY_DIR}/installed/include
    LOG_DOWNLOAD ON
    LOG_INSTALL ON
    )

message(STATUS "EXTERNAL_RAPIDJSON_INCLUDE_DIR: ${EXTERNAL_RAPIDJSON_INCLUDE_DIR}")

if (INSTALL_RAPIDJSON_HEADERS)
    install(DIRECTORY ${EXTERNAL_RAPIDJSON_INCLUDE_DIR} DESTINATION include)
endif()

include_directories(${CMAKE_BINARY_DIR}/installed/include)
