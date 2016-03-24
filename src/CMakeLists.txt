
set(ACTUAL_SOURCES
    RestClientImpl.cpp
    RequestImpl.cpp
    ReplyImpl.cpp
    ConnectionPoolImpl.cpp
    Url.cpp
    )
set(HEADERS
    ${RESTC_ROOT_DIR}/include/restc-cpp/restc-cpp.h
    )

if (WIN32)
    ADD_MSVC_PRECOMPILED_HEADER(restc-cpp.h pch.cpp ACTUAL_SOURCES)
    add_definitions(-DWAR_PCH)
    set(SOURCES ${ACTUAL_SOURCES} pch.cpp ${HEADERS} ${RESFILES})
else()
    set(SOURCES ${ACTUAL_SOURCES})
endif()

add_library(restc-cpp ${SOURCES})
set_target_properties(restc-cpp PROPERTIES DEBUG_OUTPUT_NAME restc-cppD)
target_link_libraries(restc-cpp ${BOOST_LIBRARIES})
