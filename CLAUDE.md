# CLAUDE.md - Noo Project Guide

## Project Overview

**Noo** is a cross-platform desktop outliner application with time tracking capabilities, built with Qt and C++.

- **Description**: Tiny outliner with time tracking
- **Version**: 0.9.8
- **Platforms**: Linux, macOS, Windows
- **Framework**: Qt 5 (CMake), Qt 5/6 (qmake — Qt6 migration in progress)
- **Language**: C++17 (qmake), C++20 (CMake)

## Build Instructions

### Prerequisites

**Linux:**
```bash
# Ubuntu/Debian
sudo apt install mesa-common-dev libglu1-mesa-dev libssl-dev uuid-dev qt6-base-dev
```

**macOS:**
- Xcode command line tools
- Qt via Homebrew or installer

### Building

**Using qmake (primary):**
```bash
cd client
qmake noo.pro
make -j$(nproc)
```

**Using CMake:**
```bash
cd client
mkdir build && cd build
cmake ..
make -j$(nproc)
```

**Linux AppImage (release):**
```bash
cd scripts
python3 build_outliner.py
# Output: scripts/releases/noo-x.x.x-x86_64.AppImage
```

## Project Structure

```
noo/
├── client/                  # Main Qt application
│   ├── main.cpp            # Entry point
│   ├── mainwindow.*        # Main window controller
│   ├── storage.*           # SQLite database layer
│   ├── task.*              # Task/node data model
│   ├── tasktreemodel.*     # Qt model for task tree
│   ├── taskaction.*        # Undo/redo action system (command pattern)
│   ├── encryption.*        # Blowfish encryption (OpenSSL)
│   ├── twofish.*           # Twofish cipher (bundled implementation)
│   ├── settings.*          # Application preferences
│   ├── helper.*            # Utility functions (paths, date/time, theme)
│   ├── config.h            # Version and app constants
│   ├── logger.*            # File logging
│   ├── appevents.*         # Custom Qt events
│   ├── runguard.*          # Single-instance guard (shared memory)
│   ├── finddialog.*        # Find text in current document
│   ├── findsupport.*       # Find/search across tasks
│   ├── browserwidget.*     # Browser/preview widget
│   ├── connectdb_widget.*  # UI for connecting to existing database
│   ├── openorcreatedb_widget.* # UI for creating/opening database
│   ├── nodepropertieswidget.*  # Task node properties panel
│   ├── timetreedlg.*       # Timeline tree dialog
│   ├── timetreemodel.*     # Qt model for time tree view
│   ├── timereportwizard.*  # Time report generation wizard
│   ├── timeintervaldlg.*   # Edit time intervals dialog
│   ├── startworkdialog.*   # Prompt to start tracking on activity
│   ├── stopworkdialog.*    # Prompt to stop tracking on idle
│   ├── aboutdlg.*          # About dialog
│   ├── preferencesdlg.*    # Preferences dialog
│   ├── passworddlg.*       # Password entry dialog
│   ├── newpassworddlg.*    # New password creation dialog
│   ├── attachmentsdialog.* # Attachments management dialog
│   ├── attachmentslist.*   # Attachments list widget
│   ├── platforms/          # Platform-specific implementations
│   │   ├── hidtracker.*    # Cross-platform HID tracker interface
│   │   ├── hidtrackerimpl.*# Default HID tracker implementation
│   │   ├── updater.h       # Cross-platform updater interface
│   │   ├── linux/          # Linux-specific implementations
│   │   └── osx/            # macOS implementations (.mm)
│   ├── sqlitecpp/          # SQLite C++ wrapper (bundled)
│   ├── qmarkdowntextedit/  # Markdown editor widget (bundled)
│   ├── qtkeychain/         # Secure credential storage (bundled)
│   ├── fervor/             # Auto-update framework (bundled)
│   └── assets/             # Icons, stylesheets (dark/light), resources
├── scripts/                # Build and deployment
│   ├── build_outliner.py   # Main build orchestrator
│   ├── build_linux.sh      # Linux build script
│   ├── build_utils.py      # Build utility functions
│   └── linuxdeployqt       # AppImage creation tool
├── lib/                    # External libraries (OpenSSL per-platform)
├── server/                 # Backend server (Python)
│   ├── backend/            # Server application code
│   └── sql/                # Database schemas
├── redist/                 # Legacy build/deploy scripts (macOS, updater)
├── site/                   # Project website
├── articles/               # Articles/documentation
└── TODO.txt                # Project TODO list
```

## Architecture

### Core Components

| Component | Files | Purpose |
|-----------|-------|---------|
| MainWindow | `mainwindow.h/cpp` | Application controller, UI management (~1660 lines) |
| Task | `task.h/cpp` | Hierarchical task data model with timeline |
| Storage | `storage.h/cpp` | SQLite database abstraction (singleton) |
| TaskTreeModel | `tasktreemodel.h/cpp` | Qt model for tree view |
| TaskAction | `taskaction.h/cpp` | Undo/redo command pattern actions |
| ChangesHistory | `taskaction.h/cpp` | Undo/redo history manager (singleton) |
| Encryption | `encryption.h/cpp` | Blowfish cipher, SHA256, IV generation |
| TwofishCipher | `twofish.h/cpp` | Twofish symmetric cipher |
| Settings | `settings.h/cpp` | Preferences management (singleton) |
| AppGeometry | `settings.h/cpp` | Window geometry persistence |
| RunGuard | `runguard.h/cpp` | Single-instance enforcement via shared memory |
| Logger | `logger.h/cpp` | File-based logging |
| HIDActivityTracker | `platforms/hidtracker.*` | HID activity/idle detection |
| FindSupport | `findsupport.h/cpp` | Search across task tree |
| Helper | `helper.h/cpp` | Path, date/time, theme, keychain utilities |

### Data Model

- **Task**: Hierarchical node with ID, parent, title, HTML content, timeline, attachments, flags, cursor position
- **TimeRecord**: Work interval with start/end timestamps and WorldId
- **TimeLine**: Aggregated time records per task with start/stop/flush mechanics
- **WorldId**: UUID-based distributed ID system
- **Attachment**: File attachments with metadata and content stored in DB

### Undo/Redo Actions (TaskAction subclasses)

- `NewTaskAction`, `DeleteTaskAction`, `RenameTaskAction`
- `MoveTaskAction`, `IncreaseLevelAction`, `DecreaseLevelAction`
- `ImportAttachmentAction`, `RenameAttachmentAction`, `DeleteAttachmentAction`
- `SaveTaskAction`

### Key Features

1. **Hierarchical Outlining**: Nested tasks with drag-and-drop reordering
2. **Time Tracking**: Manual and smart (idle-aware) tracking with start/stop dialogs
3. **Encrypted Database**: SQLite with Blowfish/Twofish encryption, optional password protection
4. **Rich Text**: Markdown editor via qmarkdowntextedit
5. **System Tray**: Background operation with tray icon and context menu
6. **Undo/Redo**: Full action history for task and attachment operations
7. **Find**: Search within current document and across all tasks
8. **Time Reporting**: Timeline tree view and report generation wizard
9. **Attachments**: File attachment management per task
10. **Single Instance**: RunGuard prevents multiple app instances
11. **Auto-Update**: Fervor framework for update checking (macOS)
12. **Cross-Platform**: Linux, macOS, Windows support

## Coding Conventions

### Naming

- `m` prefix for member variables: `mTaskTreeModel`, `mLogger`
- `P` prefix for shared pointer typedefs: `PTask`, `PAttachment`, `PTimeLine`
- `KEY_` prefix for settings key macros: `KEY_DB_FILENAME`
- `VALUE_` prefix for settings value macros: `VALUE_TIMECOUNTER_THISDAY`
- `TARGET_` macros for platform detection: `TARGET_LINUX`, `TARGET_OSX`, `TARGET_WIN`

### Patterns

- **Singleton**: `Storage::instance()`, `Settings::instance()`, `ChangesHistory::instance()`
- **Command**: `TaskAction` subclasses with `commit()`/`rollback()` for undo/redo
- **Model-View**: Qt's QAbstractItemModel for task tree and time tree
- **Signals/Slots**: Qt event handling throughout
- **Smart Pointers**: `QSharedPointer` for resource management
- **Single Instance**: `RunGuard` using `QSharedMemory` + `QSystemSemaphore`

### Style

```cpp
// Member variables with 'm' prefix
class MainWindow {
    TaskTreeModel* mTaskTreeModel;
    QSharedPointer<Task> mCurrentTask;
};

// Settings keys as #define macros (NOT static const QString)
#define KEY_DB_FILENAME "DbFilename"
#define KEY_DARK_THEME  "DarkTheme"

// Platform-specific code via preprocessor
#if defined(TARGET_LINUX)
    // Linux implementation
#elif defined(TARGET_OSX)
    // macOS implementation
#endif

// Undo/redo via macro
MAKE_ACTION(new RenameTaskAction(task, newTitle));
```

## Key Files Reference

| File | Purpose |
|------|---------|
| `config.h` | App version, database name, settings file, icon paths |
| `mainwindow.cpp` | Main application logic (~1660 lines) |
| `storage.cpp` | Database operations, schema upgrades, encryption |
| `task.cpp` | Task model with timeline logic (~1130 lines) |
| `tasktreemodel.cpp` | Tree view model implementation (~650 lines) |
| `taskaction.cpp` | All undo/redo action implementations |
| `encryption.cpp` | Blowfish encryption, SHA256, IV generation |
| `twofish.cpp` | Twofish cipher implementation |
| `settings.cpp` | Preferences storage and retrieval |
| `helper.cpp` | Path resolution, date/time conversion, theme |

## Dependencies

### Bundled Libraries

- **SQLiteCpp**: C++ wrapper for SQLite (`client/sqlitecpp/`)
- **qmarkdowntextedit**: Markdown editor (`client/qmarkdowntextedit/`)
- **Qt Keychain**: Credential storage (`client/qtkeychain/`)
- **Fervor**: Auto-updater (`client/fervor/`)
- **Twofish**: Symmetric cipher (`client/twofish.cpp`)
- **SQLite3**: Embedded database source (`client/sqlite3.c`)

### System Libraries

- **Qt 5/6**: Core, Gui, Widgets, PrintSupport, Network, OpenGL
- **OpenSSL**: libssl, libcrypto (Blowfish, SHA256)
- **libuuid**: UUID generation (Linux)

## Configuration

### Settings Keys (settings.h)

Settings keys are `#define` macros mapping to string values stored in the INI file:

- `KEY_DB_FILENAME` (`"DbFilename"`): Database file path
- `KEY_AUTOSAVE_PASSWORD` (`"AutosavePassword"`): Store password in keychain
- `KEY_SHOW_SECONDS` (`"ShowSeconds"`): Time display format
- `KEY_DARK_THEME` (`"DarkTheme"`): UI theme selection
- `KEY_SMART_START` (`"SmartStart"`): Auto-start tracking on activity
- `KEY_SMART_STOP` (`"SmartStop"`): Auto-stop tracking on idle
- `KEY_SMART_STOP_MINUTES` (`"SmartStopIntervalInMinutes"`): Idle threshold
- `KEY_ASK_START` (`"AskStart"`): Prompt before auto-start
- `KEY_ASK_STOP` (`"AskStop"`): Prompt before auto-stop
- `KEY_SHOW_TRAY_ICON` (`"ShowTrayIcon"`): System tray visibility
- `KEY_CUMULATIVE_REPORT` (`"CumulativeReport"`): Cumulative time report mode
- `KEY_TIMECOUNTER_TYPE` (`"TimecounterType"`): Counter display mode
- `KEY_ASK_BEFORE_DELETE` (`"AskBeforeDelete"`): Confirm task deletion
- `KEY_APP_FONT` (`"AppFont"`): Application font

### App Constants (config.h)

```cpp
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 9
#define APP_VERSION_SUFFIX 8
#define DATABASENAME     "database.noo"
#define SETTINGS_FILENAME "noo.ini"
#define LOGNAME          "noo-log.txt"
#define NODE_MIME_TYPE   "application/noo-node"
#define TIMELINE_FLUSH_INTERVAL (300)  // seconds
#define TEXT_FLUSH_INTERVAL     (10)   // seconds
```

## Platform-Specific Code

### Shared (`platforms/`)
- `hidtracker.h/cpp`: Cross-platform HID tracker interface
- `hidtrackerimpl.h/cpp`: Default HID tracker implementation
- `updater.h`: Cross-platform updater interface

### Linux (`platforms/linux/`)
- `hidtracker_linux.cpp/h`: HID activity tracking
- `sleeptracker_linux.cpp/h`: Sleep state detection
- `idle_tracking.cpp/h`: User idle detection
- `autostart.cpp/h`: Desktop autostart integration
- `updater_linux.cpp/h`: Linux updater implementation

### macOS (`platforms/osx/`)
- `hidtrackerimpl_osx.mm/h`: Objective-C++ HID tracking
- `sleeptracker_osx.mm/h`: Sleep/wake detection
- `updater_osx.mm/h`: macOS updater implementation
- Uses IOKit, CoreFoundation, Cocoa, Carbon frameworks

## Testing

- SQLiteCpp includes unit tests in `client/sqlitecpp/tests/`
- No dedicated test suite for main application

## Common Tasks

### Adding a New Setting

1. Add key macro in `settings.h`: `#define KEY_NEW_SETTING "NewSetting"`
2. Access via `Settings::instance().data()` map using the key macro
3. Add UI control in `preferencesdlg.ui` if needed

### Adding Platform-Specific Code

1. Create implementation in `platforms/<platform>/`
2. Use preprocessor guards: `#if defined(TARGET_LINUX)`
3. Update build files (CMakeLists.txt and/or noo.pro)

### Database Schema Changes

1. Increment version in `storage.cpp`
2. Add migration logic in upgrade function
3. Update relevant queries

### Adding an Undoable Action

1. Create a `TaskAction` subclass in `taskaction.h/cpp`
2. Implement `commit()` and `rollback()` methods
3. Use via `MAKE_ACTION(new YourAction(...))` macro

## Notes

- Recent migration to Qt6 (commit 1d67a97) — qmake path; CMakeLists.txt still references Qt5
- Application was renamed from "Litt" to "Noo"
- Uses encrypted SQLite database with optional password (Blowfish or Twofish)
- MIME type for drag-drop: `application/noo-node`
- Bundle identifier: `com.satorilight.noo`
- Company: `satorilight.com`
