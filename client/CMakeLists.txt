cmake_minimum_required (VERSION 3.15)

# This directive is ignored for non OSX environments
set (CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version")

project (Noo C CXX) # Your project name

if (NOT LIB_PLATFORM)
    message("No LIB_PLATFORM is set, defaulting to works/sevana/platform/libraries")
    set (LIB_PLATFORM ${CMAKE_CURRENT_SOURCE_DIR}/../../../sevana/platform/libraries)
endif()

include (${LIB_PLATFORM}/platform_libs.cmake)

# C++ 20 standard
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Find includes in corresponding build directories
set (CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)
set (CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run uic automatically when needed.
set (CMAKE_AUTOUIC ON)

set (APP_SKIP_BUILD_NUMBER OFF CACHE BOOL "Skip Noo build number increase.")

# App version is expected as APP_VERSION variable
#if (NOT APP_VERSION)
#    message(FATAL "Requires APP_VERSION to be set")
#endif()

#add_definitions(-DAPP_VERSION="\"${APP_VERSION}\"")

# This will find the Qt5 files. You will need a QT5_DIR env variable
find_package (Qt5Core REQUIRED)
find_package (Qt5Widgets REQUIRED) # Equivalent of QT += widgets
find_package (Qt5PrintSupport REQUIRED)
find_package (Qt5OpenGL REQUIRED)
find_package (Qt5Network REQUIRED)

if (Qt5OpenGL_FOUND)
    message("Qt5 OpenGL found")
else (Qt5OpenGL_FOUND)
    message("Qt5 OpenGL not found")
endif(Qt5OpenGL_FOUND)

# List all source files in project directory
set (SOURCES
    aboutdlg.cpp
    appevents.cpp
    attachmentsdialog.cpp
    attachmentslist.cpp
    browserwidget.cpp
    connectdb_widget.cpp
    encryption.cpp
    finddialog.cpp
    findsupport.cpp
    helper.cpp
    # initial_ui.cpp
    logger.cpp
    main.cpp
    mainwindow.cpp
    obsidianexport.cpp
    newpassworddlg.cpp
    openorcreatedb_widget.cpp
    passworddlg.cpp
    preferencesdlg.cpp
    #qrc_mainwindow.cpp
    #qrc_media.cpp
    #qrc_style.cpp
    settings.cpp
    startworkdialog.cpp
    stopworkdialog.cpp
    storage.cpp
    task.cpp
    taskaction.cpp
    tasktreemodel.cpp
    timeintervaldlg.cpp
    timereportwizard.cpp
    timetreedlg.cpp
    timetreemodel.cpp
    twofish.cpp
    )

set (C_SOURCES
    sqlite3.c
    )

set (HEADERS
    aboutdlg.h
    appevents.h
    attachmentsdialog.h
    attachmentslist.h
    browserwidget.h
    config.h
    connectdb_widget.h
    encryption.h
    finddialog.h
    findsupport.h
    helper.h
    # initial_ui.h
    logger.h
    mainwindow.h
    obsidianexport.h
    #moc_predefs.h
    newpassworddlg.h
    openorcreatedb_widget.h
    passworddlg.h
    preferencesdlg.h
    settings.h
    sqlite3.h
    sqlite3ext.h
    startworkdialog.h
    stopworkdialog.h
    storage.h
    task.h
    taskaction.h
    tasktreemodel.h
    timeintervaldlg.h
    timereportwizard.h
    timetreedlg.h
    timetreemodel.h
    twofish.h
    )

set (UI
    aboutdlg.ui
    attachmentsdialog.ui
    attachmentslist.ui
    browserwidget.ui
    connectdb_widget.ui
    finddialog.ui
    mainwindow.ui
    newpassworddlg.ui
    openorcreatedb_widget.ui
    passworddlg.ui
    preferencesdlg.ui
    startworkdialog.ui
    stopworkdialog.ui
    timeintervaldlg.ui
    timetreedlg.ui)

if (CMAKE_SYSTEM MATCHES "Windows*")
    add_definitions(-DTARGET_WIN)
    set (TARGET_WIN ON)
endif()

if (CMAKE_SYSTEM MATCHES "Darwin*")
    add_definitions(-DTARGET_OSX)
    set (TARGET_OSX ON)
endif()

if (CMAKE_SYSTEM MATCHES "Linux*")
    add_definitions (-DTARGET_LINUX)
    set (TARGET_LINUX ON)
endif()

# sevana pvqa
if (TARGET_LINUX)
    set (PLATFORM_LIBS pthread m z dl)
    set (SCRIPT_PROCESSOR "/bin/bash")
    set (BUILD_NUMBER_SCRIPT build_number.sh)

    # Override UUID library from 'platform/libraries' to avoid problem when linking
    set(UUID_LIB uuid)
endif()

if(TARGET_OSX)
    message ("MacOS build target")
    # OS X Specific flags
    add_definitions(-DTARGET_OSX)

    set (SCRIPT_PROCESSOR "/bin/bash")
    set (BUILD_NUMBER_SCRIPT build_number.sh)
    set (ADDITIONAL_EXE_OPTIONS MACOSX_BUNDLE)

    set (MACOS_ICON_FILENAME appicon-osx.icns)
    set (MACOS_ICON_PATH     ${CMAKE_CURRENT_SOURCE_DIR}/icons/${MACOS_ICON_FILENAME})

    set (SOURCE_FILES    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/osx/sleeptracker_osx.h
                         ${CMAKE_CURRENT_SOURCE_DIR}/platforms/osx/sleeptracker_osx.mm
                         ${CMAKE_CURRENT_SOURCE_DIR}/platforms/osx/hidtrackerimpl_osx.h
                         ${CMAKE_CURRENT_SOURCE_DIR}/platforms/osx/hidtrackerimpl_osx.mm
                         )


     find_library(COREFOUNDATION_LIBRARY    CoreFoundation)
     find_library(COCOA                     Cocoa)
     find_library(IOKIT_LIBRARY             IOKit)
     find_library(AUDIOTOOLBOX_LIBRARY      AudioToolbox)
     find_library(COREAUDIO_LIBRARY         CoreAudio)
     find_library(CARBON_LIBRARY            Carbon)

     set (PLATFORM_LIBS
         ${COCOA} ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY}
         ${AUDIOTOOLBOX_LIBRARY} ${COREAUDIO_LIBRARY}
         ${CARBON_LIBRARY} Fervor)
endif()

if (TARGET_WIN)
    # Windows Specific flags - MSVC expected
    add_definitions(
	-D_CRT_SECURE_NO_WARNINGS -DHAVE_WINSOCK2_H
	-D_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS -DUNICODE -D_UNICODE )
    set (PLATFORM_LIBS Qt5::WinMain Ws2_32 crashrpt)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    link_directories(
              ${CMAKE_CURRENT_SOURCE_DIR}/../libs/crashrpt/lib/dll
              )
    set (SCRIPT_PROCESSOR "Powershell.exe" )
    set (BUILD_NUMBER_SCRIPT build_number.ps1)

    # Application icon
    set (WINDOWS_RESOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/icon_soundruler.rc)
    #qt5_add_resources(UI_RESOURCES ${WINDOWS_RESOURCE_PATH})
endif()

# List resources
qt5_add_resources(UI_RESOURCES resources/qdarkstyle/dark/style.qrc mainwindow.qrc)

# Global defines
add_definitions(-DQTKEYCHAIN_NO_EXPORT -DSQLITE_HAS_CODEC -DSQLITE_OMIT_LOAD_EXTENSION)

# Add sqlitecpp & qtkeychain
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sqlitecpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/qtkeychain)
if (TARGET_OSX)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/fervor)
endif()

# This will create you executable
set (EXE_NAME noo)
set (PRODUCT_NAME Noo)

add_executable(${EXE_NAME}
    ${ADDITIONAL_EXE_OPTIONS}
    ${SOURCES} ${C_SOURCES} ${HEADERS} ${UI}
    ${CMAKE_CURRENT_SOURCE_DIR}/qmarkdowntextedit/qmarkdowntextedit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qmarkdowntextedit/markdownhighlighter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qmarkdowntextedit/qplaintexteditsearchwidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/hidtracker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/hidtrackerimpl.cpp
    ${SOURCE_FILES}
    ${UI_RESOURCES}
    ${MACOS_ICON_PATH}
    ${RESOURCE_FILES}
    ${WINDOWS_RESOURCE_PATH}
    mainwindow.qrc
    )

message("OpenSSL include: ${OPENSSL_INCLUDE}")

target_include_directories(${EXE_NAME} SYSTEM BEFORE
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/sqlitecpp/include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/sqlitecpp/sqlite3/include
    PUBLIC ${OPENSSL_INCLUDE}
    # PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../lib/uuid/include
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/fervor
    )

# message("App version: ${APP_VERSION}")
if (TARGET_OSX)
    set_source_files_properties( ${MACOS_ICON_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
    set_target_properties( ${EXE_NAME} PROPERTIES   MACOSX_BUNDLE_ICON_FILE ${MACOS_ICON_FILENAME} )
    set_target_properties( ${EXE_NAME} PROPERTIES   OUTPUT_NAME ${PRODUCT_NAME})
    set_target_properties( ${EXE_NAME} PROPERTIES   MACOSX_BUNDLE_BUNDLE_NAME ${PRODUCT_NAME} )
    # set_target_properties( ${EXE_NAME} PROPERTIES   MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}")
    set_target_properties( ${EXE_NAME} PROPERTIES   MACOSX_BUNDLE_LONG_VERSION_STRING "${APP_VERSION}")

    set_target_properties( ${EXE_NAME} PROPERTIES   MACOSX_BUNDLE TRUE
                                                    MACOSX_BUNDLE_GUI_IDENTIFIER "com.satorilight.noo"
                                                    RESOURCE "${RESOURCE_FILES}")
endif()

# This will link necessary Qt5 libraries to your project
target_link_libraries(${EXE_NAME}
    SQLiteCpp
    qtkeychain
    ${UUID_LIB}
    ${OPENSSL_CRYPTO}
    Qt5::Core Qt5::Widgets Qt5::PrintSupport Qt5::OpenGL Qt5::Network   # Qt
    ${PLATFORM_LIBS}                                                    # System specific libraries
    )

if (TARGET_WIN)
    set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
    set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
    set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
    set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")

    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        # It happens from Qt Creator usually - so use raw output dir
        set (OUTPUT_DIR "${CMAKE_BINARY_DIR}")
    else(CMAKE_BUILD_TYPE MATCHES "Debug")
        # It happens from build script - add configuration name
        set (OUTPUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
    endif(CMAKE_BUILD_TYPE MATCHES "Debug")

    add_custom_command(TARGET ${EXE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/../libs/crashrpt/lib/dll/crashrpt.dll"
        "${PROJECT_SOURCE_DIR}/../libs/crashrpt/bin/dbghelp.dll"
        "${PROJECT_SOURCE_DIR}/../libs/crashrpt/bin/crashrpt_lang.ini"
        "${PROJECT_SOURCE_DIR}/../libs/crashrpt/bin/CrashSender1403.exe"
        "${OUTPUT_DIR}")
endif()

if (NOT APP_SKIP_BUILD_NUMBER)
    add_custom_command( TARGET ${EXE_NAME}
                        POST_BUILD
                        COMMAND ${SCRIPT_PROCESSOR} ${CMAKE_CURRENT_SOURCE_DIR}/build-number/${BUILD_NUMBER_SCRIPT}
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build-number/ )
endif(NOT APP_SKIP_BUILD_NUMBER)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
