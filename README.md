# ![Traymond](icon/traymond_logo.png) Traymond Modern

A modern, feature-rich Windows application for minimizing any window to the system tray. Built with modern C++20 and full Unicode support.

## ✨ Features

### Core Functionality
- **Minimize to Tray**: Press `Win + Shift + Z` to minimize the currently focused window to the system tray
- **Restore Windows**: Double-click any tray icon to restore the corresponding window
- **Unlimited Windows**: No artificial limit on hidden windows (uses dynamic memory allocation)
- **Crash Recovery**: If Traymond terminates unexpectedly, restart it and all minimized windows will be automatically restored
- **Unicode Support**: Full support for international characters in window titles

### Advanced Features
- **Auto-Startup**: Configure Traymond to launch automatically when Windows starts
- **Auto-Minimize List**: Specify programs that should be automatically minimized to tray when they launch (including Windows Store apps)
- **Quick Add Hotkey**: Press `Win + Shift + A` while focused on any window to add it to the auto-minimize list
- **Customizable Hotkeys**: Configure your own hotkey combinations in Settings
- **Settings Dialog**: Easy-to-use interface for configuration (double-click the Traymond tray icon)
- **System Tray Menu**: Right-click menu with quick access to all functions

### Technical Improvements
- Modern C++20 codebase with RAII and smart pointers
- x64-safe (no crashes on 64-bit Windows)
- Class-based architecture for better maintainability
- CMake build system for easy compilation
- No memory leaks with proper resource management

## 🚀 Quick Start

1. Run `Traymond.exe`
2. Press `Win + Shift + Z` to minimize any window to tray (default, customizable)
3. Double-click a tray icon to restore that window
4. Double-click the main Traymond icon to open settings and configure hotkeys

## ⚙️ Settings

Access settings by **double-clicking** the Traymond tray icon or by **right-clicking** and selecting "Settings..."

### Run on System Startup
Check this option to have Traymond start automatically when Windows boots.

### Auto-Minimize Programs
Add programs to automatically minimize when they launch. You can:
- Click "Add File..." to browse for executables
- Press `Win + Shift + A` while the app is focused to add it quickly (works with Windows Store apps too!)
- Click "Remove" to delete selected entries

**Supported:**
- Regular desktop applications (e.g., Chrome, Notepad)
- Windows Store apps (e.g., Calculator, WhatsApp, Spotify)
- Any application with a main window

When these programs launch, they will automatically be minimized to the tray.

### Hotkey Configuration
Customize your keyboard shortcuts:
- **Minimize Window**: Default is `Win + Shift + Z`
- **Add to Auto-List**: Default is disabled (to avoid conflicts)
- Leave blank to disable a hotkey
- Changes take effect immediately (no restart needed!)

## 🎮 Controls

| Action | Description |
|--------|-------------|
| `Win + Shift + Z` | Minimize the currently focused window to tray (customizable) |
| `Win + Shift + A` | Add focused window to auto-minimize list (optional, customizable) |
| Double-click tray icon | Restore a minimized window |
| Double-click Traymond icon | Open settings dialog |
| Right-click Traymond icon | Show context menu |

**Note**: Hotkeys can be customized in Settings. Default auto-add hotkey is disabled to avoid conflicts.

### Tray Menu Options
- **Restore All Windows**: Bring back all minimized windows at once
- **Settings...**: Open the settings dialog
- **Exit**: Close Traymond and restore all windows

## 🔧 Building from Source

### Prerequisites
- Visual Studio 2022 (or later) with C++ desktop development workload
- CMake 3.20+ (optional, VS can use CMakeLists.txt directly)

### Build Steps

#### Using Visual Studio
1. Open the project folder in Visual Studio 2022
2. VS will automatically detect `CMakeLists.txt`
3. Press `Ctrl + Shift + B` to build
4. Press `F5` to run

#### Using CMake (Command Line)
```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A Win32
cmake --build . --config Release
```

The executable will be in `build/Release/Traymond.exe`

## 📋 System Requirements

- **OS**: Windows 7 or later (Windows 10/11 recommended)
- **Architecture**: x86 or x64
- **Requirements**: No additional dependencies

**Windows Store Apps**: Fully supported! Use `Win + Shift + A` to quickly add Store apps to the auto-minimize list, as they're installed in protected folders that file browsers can't easily access.

## 🛠️ Technical Details

### Modern C++ Features Used
- C++20 standard with `constexpr` and modern syntax
- RAII with `std::unique_ptr` for automatic resource cleanup
- `std::vector` for dynamic window storage (no fixed limits)
- `std::wstring` for Unicode string handling
- `std::filesystem` for file operations
- Lambda expressions and STL algorithms

### Architecture
- **Class-based design**: `TraymondApp` encapsulates all functionality
- **No globals**: Clean, maintainable code structure
- **Unicode-first**: All Windows API calls use wide-character versions
- **x64-safe**: Proper HWND handling for 64-bit compatibility

### Files Created
- `traymond_recovery.dat`: Stores hidden windows for crash recovery (binary format)
- `traymond_auto.txt`: List of programs to auto-minimize (text format, UTF-16)
- `traymond_hotkeys.txt`: Custom hotkey configuration (text format)

## 📝 Version History

### Version 2.0.0 (Modern Rewrite)
- Complete rewrite in modern C++20
- Added settings dialog with startup and auto-minimize options
- **Windows Store app support** with quick-add hotkey (`Win + Shift + A`)
- **Customizable hotkeys** with graceful conflict handling
- Full Unicode support for all languages
- Removed 100-window limit
- x64-safe implementation
- CMake build system
- Improved crash recovery with binary format
- Class-based architecture
- Smart window filtering (only main windows, not dialogs/tool windows)

### Version 1.0.4 (Legacy)
- Original C-style implementation
- Basic minimize-to-tray functionality

## 🤝 Contributing

Contributions are welcome! This is a modern fork focused on:
- Clean, maintainable C++ code
- Modern Windows development practices
- User-friendly features

## 📄 License

Based on the original Traymond by fcFn. This modern fork includes significant enhancements and rewrites.

## ⚠️ Known Limitations

- Desktop and taskbar windows are protected from minimization
- Some system dialogs cannot be minimized (by design)
- Hotkey conflicts are handled gracefully but may prevent registration
