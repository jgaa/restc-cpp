
set(ACTUAL_SOURCES
    ChunkedReaderImpl.cpp
    IoReaderImpl.cpp
    PlainReaderImpl.cpp
    NoBodyReaderImpl.cpp
    DataReaderStream.cpp
    RestClientImpl.cpp
    RequestImpl.cpp
    ReplyImpl.cpp
    ConnectionPoolImpl.cpp
    Url.cpp
    Body.cpp
    url_encode.cpp
    )

set(HEADERS
    ${RESTC_ROOT_DIR}/include/restc-cpp/restc-cpp.h
    )

if (RESTC_CPP_WITH_ZLIB)
    set(ACTUAL_SOURCES ${ACTUAL_SOURCES} ZipReaderImpl.cpp)
endif()

if (WIN32)
    ADD_MSVC_PRECOMPILED_HEADER(restc-cpp/restc-cpp.h pch.cpp ACTUAL_SOURCES)
    add_definitions(-DWAR_PCH)
    set(SOURCES ${ACTUAL_SOURCES} pch.cpp ${HEADERS} ${RESFILES})
else()
    set(SOURCES ${ACTUAL_SOURCES})
endif()

add_library(restc-cpp ${SOURCES})
set_target_properties(restc-cpp PROPERTIES DEBUG_OUTPUT_NAME restc-cppD)
target_link_libraries(restc-cpp ${BOOST_LIBRARIES})
