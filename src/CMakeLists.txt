cmake_minimum_required(VERSION 3.14)

# Use system compilers (GCC 13)

project(VideoSDKQtDemo CXX)
set(TARGET_NAME VideoSDKQtDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use system Qt5 completely for building, handle library conflicts at runtime
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# Store SDK Qt library path for runtime
set(QT_SDK_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_video_sdk/qt_libs/Qt/lib)

# Set Qt-specific configurations
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)

# Find ALSA library for audio playback
find_package(ALSA REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/zoom_video_sdk)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_video_sdk)
link_directories(${GLIB_LIBRARY_DIRS} ${GIO_LIBRARY_DIRS})
include_directories(${GLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})
add_definitions(${GLIB_CFLAGS_OTHER} ${GIO_CFLAGS_OTHER})

# Qt GUI sources
set(GUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/QtVideoRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QtMainWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QtVideoWidget.cpp
)

add_executable(${TARGET_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/zoom_v-sdk_linux_bot_qt.cpp
    ${GUI_SOURCES}
)

# Simple test executable without Qt GUI
add_executable(simple_join
    ${CMAKE_CURRENT_SOURCE_DIR}/simple_join.cpp
)

# Link libraries for simple_join (matching GTK order)
target_link_libraries(simple_join PkgConfig::deps)
target_link_libraries(simple_join videosdk)
target_link_libraries(simple_join curl)
target_link_libraries(simple_join ${GLIB_LIBRARIES} ${GIO_LIBRARIES})
target_link_libraries(simple_join ${ALSA_LIBRARIES})

# Set RPATH for simple_join too
set_target_properties(simple_join PROPERTIES
    INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_video_sdk/qt_libs/Qt/lib:${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_video_sdk"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Link libraries (matching GTK order)
target_link_libraries(${TARGET_NAME} PkgConfig::deps)
target_link_libraries(${TARGET_NAME} videosdk)
target_link_libraries(${TARGET_NAME} curl)
target_link_libraries(${TARGET_NAME} ${GLIB_LIBRARIES} ${GIO_LIBRARIES})
target_link_libraries(${TARGET_NAME} ${ALSA_LIBRARIES})

# Link Qt5 libraries
target_link_libraries(${TARGET_NAME} Qt5::Core Qt5::Widgets)

# Try to suppress undefined symbol errors
target_link_libraries(${TARGET_NAME} "-Wl,--allow-shlib-undefined")

# Set RPATH so the executable finds the SDK libraries automatically
set_target_properties(${TARGET_NAME} PROPERTIES
    INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_video_sdk/qt_libs/Qt/lib:${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_video_sdk"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Copy config file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.json ${CMAKE_CURRENT_SOURCE_DIR}/bin/config.json COPYONLY)

# Copy Zoom SDK libraries
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_video_sdk/ DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# Download nlohmann/json if not exists
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/json.hpp")
    execute_process(COMMAND wget -O ${CMAKE_CURRENT_SOURCE_DIR}/include/json.hpp https://raw.githubusercontent.com/nlohmann/json/refs/heads/develop/single_include/nlohmann/json.hpp)
endif()

# Create logs directory
execute_process(COMMAND mkdir -p .zoom/logs WORKING_DIRECTORY $ENV{HOME})

# Create symbolic link for videosdk library
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_video_sdk/libvideosdk.so.1")
    execute_process(COMMAND ln -s libvideosdk.so libvideosdk.so.1 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/zoom_video_sdk)
endif()
