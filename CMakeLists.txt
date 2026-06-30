cmake_minimum_required(VERSION 3.16)
project(TimelineVideo VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Sql Core Svg)

set(VLC_SDK_DIR "C:/vlc-sdk" CACHE PATH "Path to VLC SDK")
set(VLC_INSTALL_DIR "C:/Program Files/VideoLAN/VLC" CACHE PATH "Path to VLC installation")

find_path(LIBVLC_INCLUDE_DIRS vlc/vlc.h PATHS ${VLC_SDK_DIR}/include NO_DEFAULT_PATH)
find_library(LIBVLC_LIBRARY libvlc.dll.a libvlc.lib PATHS ${VLC_SDK_DIR}/lib ${VLC_INSTALL_DIR}/lib NO_DEFAULT_PATH)
find_library(LIBVLCCORE_LIBRARY libvlccore.dll.a libvlccore.lib PATHS ${VLC_SDK_DIR}/lib ${VLC_INSTALL_DIR}/lib NO_DEFAULT_PATH)

if(NOT LIBVLC_INCLUDE_DIRS)
    message(FATAL_ERROR "VLC SDK headers not found. Set VLC_SDK_DIR to the SDK root.")
endif()
if(NOT LIBVLC_LIBRARY)
    message(FATAL_ERROR "libvlc library not found. Set VLC_SDK_DIR or VLC_INSTALL_DIR.")
endif()

set(SOURCES
    src/main.cpp
    src/themes.cpp
    src/i18n.cpp
    src/mainwindow.cpp
    src/database/dbmanager.cpp
    src/player/vlcplayer.cpp
    src/player/controls.cpp
    src/playlist/manager.cpp
    src/playlist/scanner.cpp
    src/widgets/sidebar.cpp
    src/widgets/videolist.cpp
    src/widgets/playlistinfo.cpp
    src/widgets/toolbar.cpp
    src/widgets/timelinebar.cpp
    src/utils/logger.cpp
    src/utils/timeutils.cpp
    src/utils/thumbnail.cpp
    src/utils/workers.cpp
)

set(HEADERS
    src/themes.h
    src/i18n.h
    src/mainwindow.h
    src/database/dbmanager.h
    src/player/vlcplayer.h
    src/player/controls.h
    src/playlist/manager.h
    src/playlist/scanner.h
    src/widgets/sidebar.h
    src/widgets/videolist.h
    src/widgets/playlistinfo.h
    src/widgets/toolbar.h
    src/widgets/timelinebar.h
    src/utils/logger.h
    src/utils/timeutils.h
    src/utils/thumbnail.h
    src/utils/workers.h
)

set(RESOURCES resources.qrc appicon.rc)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBVLC_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Widgets
    Qt5::Sql
    Qt5::Core
    Qt5::Svg
    ${LIBVLC_LIBRARY}
    ${LIBVLCCORE_LIBRARY}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
    DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    LOCALE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/locales"
)

file(COPY locales/ DESTINATION ${CMAKE_BINARY_DIR}/locales)
file(COPY data/ DESTINATION ${CMAKE_BINARY_DIR}/data)
file(COPY Img/ DESTINATION ${CMAKE_BINARY_DIR}/Img)
