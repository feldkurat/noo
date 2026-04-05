#-------------------------------------------------
#
# Project created by QtCreator 2014-01-21T12:27:27
#
#-------------------------------------------------

QT += core gui widgets printsupport

CONFIG += c++17
TARGET = noo
TEMPLATE = app

# Send version to app
VERSION = 0.9.4
# VERSION_STRING = '\\"$${VERSION}\\"'
# DEFINES += APP_VERSION=\"$${VERSION_STRING}\"

win32 {
    DEFINES += TARGET_WIN
    LIBS += $$PWD/../lib/openssl/win64/ssleay32.lib $$PWD/../lib/openssl/win64/libeay32.lib gdi32.lib user32.lib
    ICON = icons/app/noo_128x128.png
}

macx {
    QMAKE_MAC_SDK = macosx10.14
    DEFINES += TARGET_OSX
    # DEFINES += USE_LOGGER
    LIBS += ../lib/osx/libssl.a ../lib/osx/libcrypto.a
    LIBS += -framework CoreFoundation
    LIBS += -framework Cocoa
    QMAKE_CXXFLAGS += -std=c++11
    QMAKE_CXXFLAGS += -std=c++0x
    QMAKE_LFLAGS +=  -std=c++11

    License.files = License.rtf
    License.path = Contents/Resources
    QMAKE_BUNDLE_DATA += License
    ICON = icons/appicon-osx.icns
}

unix {
    DEFINES += TARGET_LINUX
    ICON = icons/app/noo_128x128.png
    LIBS += -ldl -luuid $$PWD/../lib/openssl/linux_x64/libssl.a $$PWD/../lib/openssl/linux_x64/libcrypto.a
    INCLUDEPATH += $$PWD/../lib/openssl/include
}


DEFINES += USE_ENCRYPTED_DB
DEFINES += SQLITE_HAS_CODEC SQLITE_TEMP_STORE=2 SQLITE_THREADSAFE
INCLUDEPATH += $$PWD/sqlitecpp/include
INCLUDEPATH += $$PWD/../lib/include


SOURCES += main.cpp                 \
    mainwindow.cpp                  \
    nodepropertieswidget.cpp \
    storage.cpp                     \
    task.cpp                        \
    tasktreemodel.cpp               \
    helper.cpp                      \
    encryption.cpp                  \
    newpassworddlg.cpp              \
    passworddlg.cpp                 \
    appevents.cpp                   \
    sqlitecpp/src/Column.cpp        \
    sqlitecpp/src/Database.cpp      \
    sqlitecpp/src/Statement.cpp     \
    sqlitecpp/src/Transaction.cpp   \
    sqlite3.c                       \
    preferencesdlg.cpp              \
    settings.cpp                    \
    timetreedlg.cpp                 \
    timetreemodel.cpp               \
    timereportwizard.cpp            \
    platforms/hidtracker.cpp        \
    platforms/hidtrackerimpl.cpp    \
    platforms/linux/autostart.cpp   \
    platforms/linux/idle_tracking.cpp \
    logger.cpp                      \
    timeintervaldlg.cpp             \
    aboutdlg.cpp                    \
    attachmentslist.cpp             \
    attachmentsdialog.cpp           \
    taskaction.cpp                  \
    finddialog.cpp                  \
    findsupport.cpp                 \
    stopworkdialog.cpp              \
    startworkdialog.cpp             \
    twofish.cpp                     \
    openorcreatedb_widget.cpp       \
    connectdb_widget.cpp            \
    runguard.cpp                    \
    obsidianexport.cpp              \


HEADERS  += mainwindow.h            \
    nodepropertieswidget.h \
    storage.h                       \
    task.h                          \
    tasktreemodel.h                 \
    config.h                        \
    helper.h                        \
    encryption.h                    \
    appevents.h                     \
    newpassworddlg.h                \
    passworddlg.h                   \
    appevents.h                     \
    sqlitecpp/include/SQLiteCpp/Assertion.h     \
    sqlitecpp/include/SQLiteCpp/Column.h        \
    sqlitecpp/include/SQLiteCpp/Database.h      \
    sqlitecpp/include/SQLiteCpp/Exception.h     \
    sqlitecpp/include/SQLiteCpp/SQLiteCpp.h     \
    sqlitecpp/include/SQLiteCpp/Statement.h     \
    sqlitecpp/include/SQLiteCpp/Transaction.h   \
    preferencesdlg.h                            \
    settings.h                                  \
    timetreedlg.h                               \
    timetreemodel.h                             \
    timereportwizard.h                          \
    sqlite3.h                                   \
    sqlite3ext.h                                \
    platforms/hidtracker.h                      \
    platforms/hidtrackerimpl.h                  \
    platforms/linux/autostart.h                 \
    platforms/linux/idle_tracking.h             \
    logger.h                                    \
    timeintervaldlg.h                           \
    aboutdlg.h                                  \
    attachmentslist.h                           \
    attachmentsdialog.h                         \
    taskaction.h                                \
    finddialog.h                                \
    findsupport.h                               \
    stopworkdialog.h                            \
    startworkdialog.h                           \
    twofish.h                                   \
    openorcreatedb_widget.h                     \
    connectdb_widget.h                          \
    runguard.h                                  \
    obsidianexport.h

macx {
    HEADERS += platforms/osx/hidtrackerimpl_osx.h
}

unix {
    HEADERS +=                                      \
        platforms/linux/hidtracker_linux.h          \
        platforms/linux/sleeptracker_linux.h

    SOURCES +=                                      \
        platforms/linux/hidtracker_linux.cpp        \
        platforms/linux/sleeptracker_linux.cpp
}

FORMS    += mainwindow.ui       \
    newpassworddlg.ui           \
    nodepropertieswidget.ui \
    passworddlg.ui              \
    preferencesdlg.ui           \
    timetreedlg.ui              \
    timeintervaldlg.ui          \
    aboutdlg.ui                 \
    attachmentslist.ui          \
    attachmentsdialog.ui        \
    finddialog.ui               \
    stopworkdialog.ui           \
    startworkdialog.ui          \
    openorcreatedb_widget.ui    \
    connectdb_widget.ui

RESOURCES = mainwindow.qrc assets/qdarkstyle/dark/darkstyle.qrc
                           assets/qdarkstyle/dark/lightstyle.qrc

macx {
    OBJECTIVE_SOURCES +=                        \
        platforms/osx/hidtrackerimpl_osx.mm     \
        platforms/osx/sleeptracker_osx.mm
}

!include("fervor/Fervor.pri") {
	error("Unable to include Fervor autoupdater.")
}

!include("qtkeychain/qt5keychain.pri") {
        error("Unable to include QTKeyChain library.")
}

!include("qmarkdowntextedit/qmarkdowntextedit.pri") {
        error("Unable to include QMarkdownTextEdit library")
}
