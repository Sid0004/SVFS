cmake_minimum_required(VERSION 3.16)
project(SecureVFS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

# Enable automatic processing for Qt features (signals, slots, etc.)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find required packages
find_package(Qt6 COMPONENTS Widgets Sql REQUIRED)
find_package(OpenSSL REQUIRED)

# Optional compression libraries
find_package(ZSTD QUIET)
find_package(LZ4 QUIET)
# Also try pkg-config if CMake packages not found
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    if(NOT ZSTD_FOUND)
        pkg_check_modules(ZSTD QUIET libzstd)
    endif()
    if(NOT LZ4_FOUND)
        pkg_check_modules(LZ4 QUIET liblz4)
    endif()
endif()

# Optional: LZ4 and Zstd for advanced compression
# Uncomment when libraries are available:
# find_package(PkgConfig)
# if(PKG_CONFIG_FOUND)
#     pkg_check_modules(LZ4 liblz4)
#     pkg_check_modules(ZSTD libzstd)
# endif()

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS
    include/core/*.h
    include/ui/*.h
    include/scan/*.h
)

add_executable(SecureVFS ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# Public include directories
target_include_directories(SecureVFS PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/scan
)

# Link libraries
target_link_libraries(SecureVFS 
    Qt6::Widgets 
    Qt6::Sql
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Link optional compression libs and defines
if(ZSTD_FOUND)
    if(TARGET ZSTD::ZSTD)
        target_link_libraries(SecureVFS ZSTD::ZSTD)
    else()
        target_include_directories(SecureVFS PRIVATE ${ZSTD_INCLUDE_DIRS})
        target_link_libraries(SecureVFS ${ZSTD_LIBRARIES})
    endif()
    target_compile_definitions(SecureVFS PRIVATE HAVE_ZSTD=1)
endif()

if(LZ4_FOUND)
    if(TARGET LZ4::LZ4)
        target_link_libraries(SecureVFS LZ4::LZ4)
    else()
        target_include_directories(SecureVFS PRIVATE ${LZ4_INCLUDE_DIRS})
        target_link_libraries(SecureVFS ${LZ4_LIBRARIES})
    endif()
    target_compile_definitions(SecureVFS PRIVATE HAVE_LZ4=1)
endif()

# Set application properties
set_target_properties(SecureVFS PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Copy Qt plugins for deployment
if(WIN32)
    set_target_properties(SecureVFS PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
