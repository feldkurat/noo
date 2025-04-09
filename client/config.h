#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

// Application name
#define APPNAME             "noo"

// Company name
#define COMPANY             "satorilight.com"

// Default database name
#define DATABASENAME        "database.noo"

// Log file name
#define LOGNAME             "noo-log.txt"

// About text
#define ABOUTTEXT           "Tiny outliner with time tracking capabilities"

// Text for message box with "not implemented" notification
#define NOTIMPLEMENTEDTEXT  "Not implemented in this version"

// Settings file name
#define SETTINGS_FILENAME   "noo.ini"

// No password string
#define NOPASSWORDSTRING    ""

// Mime type of task items
#define NODE_MIME_TYPE      "application/noo-node"

// Exit code to signal about problems with database
#define EXITCODE_NO_DATABASE (1)

// Timeline save to DB interval in seconds
#define TIMELINE_FLUSH_INTERVAL (300)

// Text flushing interval
#define TEXT_FLUSH_INTERVAL (10)

#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 9
#define APP_VERSION_SUFFIX 8

//#ifdef TARGET_OSX
#define ICONS                   ":/assets/images"
#define ICONS_BUTTONS           ":/assets/images/buttons"
#define NAMED_BUTTON(X)         QIcon(":/assets/images/buttons/" #X)
#define TRAY_START_ICON_NAME    ":/assets/images/buttons/starttracking-osx.png"
#define TRAY_STOP_ICON_NAME     ":/assets/images/buttons/stoptracking-osx.png"

#define TRAY_DEFAULT_ICON_NAME  ":/assets/images/buttons/tray-default.png"
#define TRAY_RUNNING_ICON_NAME  ":/assets/images/buttons/tray-running.png"

#define ACTION_START_ICON_NAME  ":/assets/images/buttons/clock-32x32.png"
#define ACTION_STOP_ICON_NAME   ":/assets/images/buttons/process-stop.png"
//#endif

#ifndef USE_ENCRYPTED_DB
# define USE_ENCRYPTED_DB
#endif

#endif
