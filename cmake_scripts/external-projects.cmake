# Here are registered all external projects
#
# Usage:
# add_dependencies(TARGET externalProjectName)
# target_link_libraries(TARGET PRIVATE ExternalLibraryName)

set(EXTERNAL_PROJECTS_PREFIX ${CMAKE_BINARY_DIR}/external-projects)
set(EXTERNAL_PROJECTS_INSTALL_PREFIX ${EXTERNAL_PROJECTS_PREFIX}/installed)

include(GNUInstallDirs)

# MUST be called before any add_executable() # https://stackoverflow.com/a/40554704/8766845
link_directories(${EXTERNAL_PROJECTS_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
include_directories($<BUILD_INTERFACE:${EXTERNAL_PROJECTS_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}>)

include(ExternalProject)

ExternalProject_Add(externalRapidJson
    PREFIX "${EXTERNAL_PROJECTS_PREFIX}"
    GIT_REPOSITORY "https://github.com/Tencent/rapidjson.git"
    GIT_TAG "master"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_PROJECTS_INSTALL_PREFIX}
    )

set(EXTERNAL_RAPIDJSON_INCLUDE_DIR ${EXTERNAL_PROJECTS_INSTALL_PREFIX}/include/rapidjson)

ExternalProject_Add(externalLest
    PREFIX "${EXTERNAL_PROJECTS_PREFIX}"
    GIT_REPOSITORY "https://github.com/martinmoene/lest.git"
    GIT_TAG "master"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_PROJECTS_INSTALL_PREFIX} -DLEST_BUILD_EXAMPLE=OFF
    )


message(STATUS "EXTERNAL_RAPIDJSON_INCLUDE_DIR: ${EXTERNAL_RAPIDJSON_INCLUDE_DIR}")

if (INSTALL_RAPIDJSON_HEADERS)
    install(DIRECTORY ${EXTERNAL_RAPIDJSON_INCLUDE_DIR} DESTINATION include)
endif()
