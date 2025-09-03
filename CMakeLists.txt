cmake_minimum_required(VERSION 3.16)
project(SecureVFS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

# Enable automatic processing for Qt features (signals, slots, etc.)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find required packages
find_package(Qt6 COMPONENTS Widgets Sql REQUIRED)

# Source files
set(SOURCES
    main.cpp
    MainWindow.cpp
    LoginDialog.cpp
    DatabaseManager.cpp
    EncryptionManager.cpp
    CompressionManager.cpp
    VFSManager.cpp
)

# Header files
set(HEADERS
    MainWindow.h
    LoginDialog.h
    DatabaseManager.h
    EncryptionManager.h
    CompressionManager.h
    VFSManager.h
)

# Create executable
add_executable(SecureVFS ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(SecureVFS 
    Qt6::Widgets 
    Qt6::Sql
)

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
