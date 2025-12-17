# Traymond Modern

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
- **Auto-Minimize List**: Specify programs that should be automatically minimized to tray when they launch
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
2. Press `Win + Shift + Z` to minimize any window to tray
3. Double-click a tray icon to restore that window
4. Double-click the main Traymond icon to open settings

## ⚙️ Settings

Access settings by **double-clicking** the Traymond tray icon or by **right-clicking** and selecting "Settings..."

### Run on System Startup
Check this option to have Traymond start automatically when Windows boots.

### Auto-Minimize Programs
Add executable names (one per line) to automatically minimize those programs when they start.

**Example:**
```
notepad.exe
chrome.exe
spotify.exe
```

When these programs launch, they will automatically be minimized to the tray.

## 🎮 Controls

| Action | Description |
|--------|-------------|
| `Win + Shift + Z` | Minimize the currently focused window to tray |
| Double-click tray icon | Restore a minimized window |
| Double-click Traymond icon | Open settings dialog |
| Right-click Traymond icon | Show context menu |

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

**Note**: Does not work with UWP apps from the Microsoft Store due to Windows security restrictions.

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

## 📝 Version History

### Version 2.0.0 (Modern Rewrite)
- Complete rewrite in modern C++20
- Added settings dialog with startup and auto-minimize options
- Full Unicode support for all languages
- Removed 100-window limit
- x64-safe implementation
- CMake build system
- Improved crash recovery with binary format
- Class-based architecture

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

- Cannot minimize UWP/Microsoft Store apps (Windows security restriction)
- Desktop and taskbar windows are protected from minimization
- Auto-minimize feature requires exact executable name matches
