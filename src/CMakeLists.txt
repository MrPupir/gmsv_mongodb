cmake_minimum_required(VERSION 3.13)

find_package(mongoc-1.0 CONFIG REQUIRED)

add_library(mongodb SHARED
    Client.hpp
    Client.cpp
    Util.cpp
    Util.hpp
    Database.cpp
    Database.hpp
    MongoDB.cpp
    MongoDB.hpp
    Collection.cpp
    Collection.hpp
    Bulk.cpp 
    Bulk.hpp 
    Macros.hpp 
    ObjectID.cpp 
    ObjectID.hpp
)

target_compile_definitions(mongodb PRIVATE 
    MONGOC_STATIC 
    BSON_STATIC
)

target_include_directories(mongodb PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${MONGOC_INCLUDE_DIRS}
)

if(WIN32)
    target_link_options(mongodb PRIVATE 
        "/NODEFAULTLIB:LIBCMT"
        "/NODEFAULTLIB:LIBCMTD"
    )
    target_link_libraries(mongodb PRIVATE 
        ws2_32 
        wsock32
    )
endif()

target_link_libraries(mongodb PRIVATE 
    mongo::mongoc_static
    gmod-module-base
)

set_gmod_suffix_prefix(mongodb)

set_target_properties(mongodb PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/release"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/debug"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/release"
)