#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <psapi.h>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <memory>
#include <set>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "psapi.lib")

// Enable Visual Styles for modern Windows UI
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Constants using modern constexpr
constexpr UINT WM_TRAYICON = WM_APP + 1;
constexpr UINT WM_AUTO_MINIMIZE = WM_APP + 2;
constexpr UINT MENU_EXIT_ID = 1001;
constexpr UINT MENU_RESTORE_ALL_ID = 1002;
constexpr UINT MENU_SETTINGS_ID = 1003;
constexpr wchar_t APP_CLASS_NAME[] = L"Traymond_Modern_Class";
constexpr wchar_t APP_TITLE[] = L"Traymond";
constexpr wchar_t MUTEX_NAME[] = L"Global\\Traymond_Single_Instance_Mutex";

// Settings dialog resource IDs
#define IDD_SETTINGS 102
#define IDC_CHK_STARTUP 1001
#define IDC_EDIT_AUTOLIST 1002
#define IDC_LIST_APPS 1005
#define IDC_BTN_ADD 1006
#define IDC_BTN_REMOVE 1007
#define IDC_HOTKEY_MINIMIZE 1008
#define IDC_HOTKEY_AUTOADD 1009

// Data files
const std::wstring DATA_FILENAME = L"traymond_recovery.dat";
const std::wstring AUTO_MINIMIZE_FILE = L"traymond_auto.txt";
const std::wstring HOTKEY_SETTINGS_FILE = L"traymond_hotkeys.txt";

struct HiddenWindow {
    HWND hWnd;
    NOTIFYICONDATAW iconData;
};

// Global auto-minimize list
std::vector<std::wstring> g_autoMinimizeList;

// Global ImageList for dialog icons
HIMAGELIST g_hImageList = nullptr;

// Global event hook and main window handle for auto-minimize
HWINEVENTHOOK g_hEventHook = nullptr;
HWND g_hMainWnd = nullptr;

// Set of windows being restored (to prevent immediate re-minimization)
std::set<HWND> g_restoringWindows;

// Track settings dialog state
HWND g_hSettingsDlg = nullptr;

// Hotkey configuration (modifier | virtualKey, 0 means disabled)
struct HotkeyConfig {
    UINT modifiers;
    UINT vk;
    bool enabled;
};
HotkeyConfig g_hotkeyMinimize = { MOD_WIN | MOD_SHIFT, 'Z', true };  // Default: Win+Shift+Z
HotkeyConfig g_hotkeyAutoAdd = { MOD_WIN | MOD_SHIFT, 'A', false };   // Default: disabled (to avoid conflicts)

// Startup registry functions
void SetStartup(bool enable) {
    HKEY hKey;
    const wchar_t* czAppName = L"Traymond";
    const wchar_t* czRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    wchar_t szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, MAX_PATH);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, czRunKey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            // Quote the path to handle spaces in directory names
            wchar_t szQuotedPath[MAX_PATH + 3];
            swprintf_s(szQuotedPath, L"\"%s\"", szPath);
            DWORD pathLen = (wcslen(szQuotedPath) + 1) * sizeof(wchar_t);
            RegSetValueExW(hKey, czAppName, 0, REG_SZ, (BYTE*)szQuotedPath, pathLen);
        } else {
            RegDeleteValueW(hKey, czAppName);
        }
        RegCloseKey(hKey);
    }
}

bool IsStartupEnabled() {
    HKEY hKey;
    bool exists = false;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"Traymond", NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            exists = true;
        }
        RegCloseKey(hKey);
    }
    return exists;
}

// Auto-minimize list management
void LoadAutoList() {
    std::wifstream infile(AUTO_MINIMIZE_FILE);
    std::wstring line;
    g_autoMinimizeList.clear();
    while (std::getline(infile, line)) {
        // Remove carriage return if present
        line.erase(std::remove(line.begin(), line.end(), L'\r'), line.end());
        if (!line.empty()) g_autoMinimizeList.push_back(line);
    }
}

void SaveAutoList() {
    std::wofstream outfile(AUTO_MINIMIZE_FILE);
    for (const auto &name : g_autoMinimizeList) {
        outfile << name << std::endl;
    }
}

void LoadHotkeySettings() {
    std::wifstream infile(HOTKEY_SETTINGS_FILE);
    if (!infile) return;
    
    std::wstring line;
    // Line 1: Minimize hotkey
    if (std::getline(infile, line)) {
        int mods, vk, enabled;
        if (swscanf_s(line.c_str(), L"%d,%d,%d", &mods, &vk, &enabled) == 3) {
            g_hotkeyMinimize = { (UINT)mods, (UINT)vk, enabled != 0 };
        }
    }
    // Line 2: Auto-add hotkey
    if (std::getline(infile, line)) {
        int mods, vk, enabled;
        if (swscanf_s(line.c_str(), L"%d,%d,%d", &mods, &vk, &enabled) == 3) {
            g_hotkeyAutoAdd = { (UINT)mods, (UINT)vk, enabled != 0 };
        }
    }
}

void SaveHotkeySettings() {
    std::wofstream outfile(HOTKEY_SETTINGS_FILE);
    outfile << g_hotkeyMinimize.modifiers << L"," << g_hotkeyMinimize.vk << L"," << (g_hotkeyMinimize.enabled ? 1 : 0) << std::endl;
    outfile << g_hotkeyAutoAdd.modifiers << L"," << g_hotkeyAutoAdd.vk << L"," << (g_hotkeyAutoAdd.enabled ? 1 : 0) << std::endl;
}

// Event hook callback to detect new windows and auto-minimize them
void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, 
                           LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) 
{
    UNREFERENCED_PARAMETER(hWinEventHook);
    UNREFERENCED_PARAMETER(dwEventThread);
    UNREFERENCED_PARAMETER(dwmsEventTime);
    
    // Only interested in main windows being shown
    if (event == EVENT_OBJECT_SHOW && idObject == OBJID_WINDOW && idChild == CHILDID_SELF) {
        // Skip if window is being restored by user
        if (g_restoringWindows.count(hwnd) > 0) return;
        
        // Only process visible windows with title bars (main application windows)
        if (!IsWindowVisible(hwnd)) return;
        
        // Check window styles - must be a main application window
        LONG style = GetWindowLongW(hwnd, GWL_STYLE);
        LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
        
        // Skip if not a main window (must have caption and be overlapped/popup style)
        if (!(style & WS_CAPTION)) return;
        if (!(style & (WS_OVERLAPPEDWINDOW | WS_POPUP))) return;
        
        // Skip tool windows, app bar windows, and other auxiliary windows
        if (exStyle & WS_EX_TOOLWINDOW) return;
        if (exStyle & WS_EX_NOACTIVATE) return;
        
        // Must have a window title
        if (GetWindowTextLengthW(hwnd) == 0) return;

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProcess) {
            wchar_t processPath[MAX_PATH];
            if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH)) {
                std::wstring fullPath = processPath;
                
                // Check if path exists in our auto-minimize list
                for (const auto& target : g_autoMinimizeList) {
                    // Case-insensitive comparison
                    if (_wcsicmp(fullPath.c_str(), target.c_str()) == 0) {
                        // Found a match! Send message to main window to minimize it safely
                        PostMessageW(g_hMainWnd, WM_AUTO_MINIMIZE, (WPARAM)hwnd, 0);
                        break;
                    }
                }
            }
            CloseHandle(hProcess);
        }
    }
}

// Refresh the application list in the settings dialog
void RefreshAppList(HWND hDlg) {
    HWND hList = GetDlgItem(hDlg, IDC_LIST_APPS);
    ListView_DeleteAllItems(hList);
    
    // Create new ImageList for small icons
    if (g_hImageList) ImageList_Destroy(g_hImageList);
    g_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 1, 1);
    ListView_SetImageList(hList, g_hImageList, LVSIL_SMALL);

    int index = 0;
    for (const auto& path : g_autoMinimizeList) {
        // Extract icon from file
        SHFILEINFOW sfi = { 0 };
        SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON);
        
        int iconIndex = -1;
        if (sfi.hIcon) {
            iconIndex = ImageList_AddIcon(g_hImageList, sfi.hIcon);
            DestroyIcon(sfi.hIcon);
        }

        // Add to list
        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_TEXT | LVIF_IMAGE;
        lvi.iItem = index++;
        lvi.iImage = iconIndex;
        lvi.pszText = const_cast<LPWSTR>(path.c_str());
        ListView_InsertItem(hList, &lvi);
    }
}

// Settings dialog procedure
INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        {
            // Store dialog handle globally
            g_hSettingsDlg = hDlg;
            
            CheckDlgButton(hDlg, IDC_CHK_STARTUP, IsStartupEnabled() ? BST_CHECKED : BST_UNCHECKED);
            
            // Initialize ListView columns
            HWND hList = GetDlgItem(hDlg, IDC_LIST_APPS);
            LVCOLUMNW lvc = { 0 };
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = 300;
            lvc.pszText = (LPWSTR)L"Application Path";
            ListView_InsertColumn(hList, 0, &lvc);
            
            // Set extended style for full row select
            ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);

            RefreshAppList(hDlg);
            
            // Initialize hotkey controls
            HWND hHotkeyMin = GetDlgItem(hDlg, IDC_HOTKEY_MINIMIZE);
            HWND hHotkeyAdd = GetDlgItem(hDlg, IDC_HOTKEY_AUTOADD);
            
            if (g_hotkeyMinimize.enabled) {
                WORD modifiers = 0;
                if (g_hotkeyMinimize.modifiers & MOD_ALT) modifiers |= HOTKEYF_ALT;
                if (g_hotkeyMinimize.modifiers & MOD_CONTROL) modifiers |= HOTKEYF_CONTROL;
                if (g_hotkeyMinimize.modifiers & MOD_SHIFT) modifiers |= HOTKEYF_SHIFT;
                if (g_hotkeyMinimize.modifiers & MOD_WIN) modifiers |= HOTKEYF_EXT;
                SendMessageW(hHotkeyMin, HKM_SETHOTKEY, MAKEWORD(g_hotkeyMinimize.vk, modifiers), 0);
            }
            
            if (g_hotkeyAutoAdd.enabled) {
                WORD modifiers = 0;
                if (g_hotkeyAutoAdd.modifiers & MOD_ALT) modifiers |= HOTKEYF_ALT;
                if (g_hotkeyAutoAdd.modifiers & MOD_CONTROL) modifiers |= HOTKEYF_CONTROL;
                if (g_hotkeyAutoAdd.modifiers & MOD_SHIFT) modifiers |= HOTKEYF_SHIFT;
                if (g_hotkeyAutoAdd.modifiers & MOD_WIN) modifiers |= HOTKEYF_EXT;
                SendMessageW(hHotkeyAdd, HKM_SETHOTKEY, MAKEWORD(g_hotkeyAutoAdd.vk, modifiers), 0);
            }
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_ADD) {
            // Open file browser dialog
            wchar_t filename[MAX_PATH] = { 0 };
            // Use proper null terminator array for filter
            static wchar_t szFilter[] = L"Executables (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
            OPENFILENAMEW ofn = { 0 };
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hDlg;
            ofn.lpstrFilter = szFilter;
            ofn.lpstrFile = filename;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            
            if (GetOpenFileNameW(&ofn)) {
                // Check if already exists
                bool exists = false;
                std::wstring newPath = filename;
                for(const auto& s : g_autoMinimizeList) {
                    if(s == newPath) { exists = true; break; }
                }
                
                if(!exists) {
                    g_autoMinimizeList.push_back(newPath);
                    RefreshAppList(hDlg);
                }
            }
        }
        else if (LOWORD(wParam) == IDC_BTN_REMOVE) {
            // Remove selected item
            HWND hList = GetDlgItem(hDlg, IDC_LIST_APPS);
            int selected = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (selected != -1 && selected < (int)g_autoMinimizeList.size()) {
                g_autoMinimizeList.erase(g_autoMinimizeList.begin() + selected);
                RefreshAppList(hDlg);
            }
        }
        else if (LOWORD(wParam) == IDOK) {
            // Save startup setting
            SetStartup(IsDlgButtonChecked(hDlg, IDC_CHK_STARTUP) == BST_CHECKED);
            SaveAutoList();
            
            // Read hotkey settings from controls
            HWND hHotkeyMin = GetDlgItem(hDlg, IDC_HOTKEY_MINIMIZE);
            HWND hHotkeyAdd = GetDlgItem(hDlg, IDC_HOTKEY_AUTOADD);
            
            DWORD hotkeyMin = (DWORD)SendMessageW(hHotkeyMin, HKM_GETHOTKEY, 0, 0);
            DWORD hotkeyAdd = (DWORD)SendMessageW(hHotkeyAdd, HKM_GETHOTKEY, 0, 0);
            
            // Parse minimize hotkey
            BYTE vkMin = LOBYTE(hotkeyMin);
            BYTE modMin = HIBYTE(hotkeyMin);
            if (vkMin != 0) {
                g_hotkeyMinimize.vk = vkMin;
                g_hotkeyMinimize.modifiers = 0;
                if (modMin & HOTKEYF_ALT) g_hotkeyMinimize.modifiers |= MOD_ALT;
                if (modMin & HOTKEYF_CONTROL) g_hotkeyMinimize.modifiers |= MOD_CONTROL;
                if (modMin & HOTKEYF_SHIFT) g_hotkeyMinimize.modifiers |= MOD_SHIFT;
                if (modMin & HOTKEYF_EXT) g_hotkeyMinimize.modifiers |= MOD_WIN;
                g_hotkeyMinimize.enabled = true;
            } else {
                g_hotkeyMinimize.enabled = false;
            }
            
            // Parse auto-add hotkey
            BYTE vkAdd = LOBYTE(hotkeyAdd);
            BYTE modAdd = HIBYTE(hotkeyAdd);
            if (vkAdd != 0) {
                g_hotkeyAutoAdd.vk = vkAdd;
                g_hotkeyAutoAdd.modifiers = 0;
                if (modAdd & HOTKEYF_ALT) g_hotkeyAutoAdd.modifiers |= MOD_ALT;
                if (modAdd & HOTKEYF_CONTROL) g_hotkeyAutoAdd.modifiers |= MOD_CONTROL;
                if (modAdd & HOTKEYF_SHIFT) g_hotkeyAutoAdd.modifiers |= MOD_SHIFT;
                if (modAdd & HOTKEYF_EXT) g_hotkeyAutoAdd.modifiers |= MOD_WIN;
                g_hotkeyAutoAdd.enabled = true;
            } else {
                g_hotkeyAutoAdd.enabled = false;
            }
            
            SaveHotkeySettings();
            
            // Apply hotkey changes immediately without restart
            // Unregister old hotkeys
            UnregisterHotKey(g_hMainWnd, 1);
            UnregisterHotKey(g_hMainWnd, 2);
            
            // Register new hotkeys with graceful error handling
            bool hotkey1Success = true;
            bool hotkey2Success = true;
            
            if (g_hotkeyMinimize.enabled) {
                if (!RegisterHotKey(g_hMainWnd, 1, g_hotkeyMinimize.modifiers | MOD_NOREPEAT, g_hotkeyMinimize.vk)) {
                    g_hotkeyMinimize.enabled = false;
                    hotkey1Success = false;
                }
            }
            
            if (g_hotkeyAutoAdd.enabled) {
                if (!RegisterHotKey(g_hMainWnd, 2, g_hotkeyAutoAdd.modifiers | MOD_NOREPEAT, g_hotkeyAutoAdd.vk)) {
                    g_hotkeyAutoAdd.enabled = false;
                    hotkey2Success = false;
                }
            }
            
            // Show appropriate message based on registration results
            if (!hotkey1Success || !hotkey2Success) {
                MessageBoxW(hDlg, L"Some hotkeys could not be registered (conflict with another application). They have been disabled.", 
                           L"Hotkey Conflict", MB_ICONWARNING);
            }
            
            g_hSettingsDlg = nullptr; // Clear dialog handle
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        } 
        else if (LOWORD(wParam) == IDCANCEL) {
            // Reload to cancel unsaved changes
            LoadAutoList();
            g_hSettingsDlg = nullptr; // Clear dialog handle
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

class TraymondApp {
public:
    TraymondApp(HINSTANCE hInstance) : m_hInstance(hInstance), m_mainWindow(nullptr), m_trayMenu(nullptr) {}

    ~TraymondApp() {
        // Unhook the window event monitoring
        if (g_hEventHook) {
            UnhookWinEvent(g_hEventHook);
            g_hEventHook = nullptr;
        }
        
        RestoreAllWindows();
        if (m_mainWindow) {
            UnregisterHotKey(m_mainWindow, 1);
            UnregisterHotKey(m_mainWindow, 2);
            DestroyWindow(m_mainWindow);
        }
        if (m_trayMenu) DestroyMenu(m_trayMenu);
    }

    bool Initialize() {
        // Prevent multiple instances
        m_hMutex.reset(CreateMutexW(nullptr, TRUE, MUTEX_NAME));
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            MessageBoxW(nullptr, L"Traymond is already running.", APP_TITLE, MB_ICONERROR);
            return false;
        }

        WNDCLASSEXW wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = m_hInstance;
        wc.lpszClassName = APP_CLASS_NAME;

        if (!RegisterClassExW(&wc)) return false;

        // Create a message-only window
        m_mainWindow = CreateWindowExW(0, APP_CLASS_NAME, APP_TITLE, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, m_hInstance, this);
        if (!m_mainWindow) return false;

        // Store main window handle globally for WinEventProc callback
        g_hMainWnd = m_mainWindow;

        // Register WinEventHook to monitor new windows for auto-minimize
        g_hEventHook = SetWinEventHook(
            EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW,
            nullptr,
            WinEventProc,
            0, 0,
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
        );

        // Load hotkey settings
        LoadHotkeySettings();
        
        // Register hotkeys with graceful error handling
        if (g_hotkeyMinimize.enabled) {
            if (!RegisterHotKey(m_mainWindow, 1, g_hotkeyMinimize.modifiers | MOD_NOREPEAT, g_hotkeyMinimize.vk)) {
                // Failed to register - just disable it, don't crash
                g_hotkeyMinimize.enabled = false;
            }
        }
        
        if (g_hotkeyAutoAdd.enabled) {
            if (!RegisterHotKey(m_mainWindow, 2, g_hotkeyAutoAdd.modifiers | MOD_NOREPEAT, g_hotkeyAutoAdd.vk)) {
                // Failed to register - just disable it, don't crash
                g_hotkeyAutoAdd.enabled = false;
            }
        }

        CreateTrayIcon();
        CreateTrayMenu();
        LoadState(); // Recovery from crash
        LoadAutoList(); // Load auto-minimize settings

        return true;
    }

    void Run() {
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

private:
    HINSTANCE m_hInstance;
    HWND m_mainWindow;
    HMENU m_trayMenu;
    std::vector<HiddenWindow> m_hiddenWindows;
    
    // RAII wrapper for Handle
    struct HandleDeleter { void operator()(HANDLE h) { if (h) CloseHandle(h); } };
    std::unique_ptr<void, HandleDeleter> m_hMutex;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        TraymondApp* app = nullptr;
        if (uMsg == WM_NCCREATE) {
            auto createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            app = reinterpret_cast<TraymondApp*>(createStruct->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        } else {
            app = reinterpret_cast<TraymondApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (app) return app->HandleMessage(hwnd, uMsg, wParam, lParam);
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    LRESULT HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_TRAYICON:
            // CRITICAL FIX: wParam contains the icon ID, not lParam
            if (wParam == 0) {
                // Main Traymond icon
                if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
                    // Double-click opens settings
                    // Check if dialog is already open
                    if (g_hSettingsDlg && IsWindow(g_hSettingsDlg)) {
                        // Focus existing dialog
                        SetForegroundWindow(g_hSettingsDlg);
                        SetActiveWindow(g_hSettingsDlg);
                    } else {
                        // Open new dialog
                        DialogBoxW(m_hInstance, MAKEINTRESOURCE(IDD_SETTINGS), m_mainWindow, SettingsDlgProc);
                    }
                }
                else if (LOWORD(lParam) == WM_RBUTTONUP) {
                    // Right-click shows main menu
                    POINT pt;
                    GetCursorPos(&pt);
                    SetForegroundWindow(hwnd);
                    TrackPopupMenu(m_trayMenu, TPM_BOTTOMALIGN | TPM_RIGHTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
                }
            }
            else {
                // Minimized window icon (wParam is the HWND)
                if (LOWORD(lParam) == WM_LBUTTONDBLCLK || LOWORD(lParam) == WM_LBUTTONUP) {
                    // Double-click or single click restores the window
                    RestoreWindowById((UINT)wParam);
                }
                else if (LOWORD(lParam) == WM_RBUTTONUP) {
                    // Right-click also restores (or could show context menu)
                    RestoreWindowById((UINT)wParam);
                }
            }
            break;

        case WM_HOTKEY:
            if (wParam == 1) { // Win+Shift+Z (Minimize)
                MinimizeForegroundWindow();
            }
            else if (wParam == 2) { // Win+Shift+A (Add to auto-list)
                AddForegroundToAutoList();
            }
            break;

        case WM_AUTO_MINIMIZE:
            // Sent by WinEventProc when a window from the auto-minimize list is detected
            MinimizeWindow((HWND)wParam);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case MENU_RESTORE_ALL_ID: RestoreAllWindows(); break;
            case MENU_SETTINGS_ID: 
                // Check if dialog is already open
                if (g_hSettingsDlg && IsWindow(g_hSettingsDlg)) {
                    // Focus existing dialog
                    SetForegroundWindow(g_hSettingsDlg);
                    SetActiveWindow(g_hSettingsDlg);
                } else {
                    // Open new dialog
                    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETTINGS), hwnd, SettingsDlgProc);
                }
                break;
            case MENU_EXIT_ID: PostQuitMessage(0); break;
            }
            break;
            
        case WM_DESTROY:
            SaveState(true); // Clear state file on clean exit
            PostQuitMessage(0);
            break;
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    // Core minimize logic - can be called for any window
    void MinimizeWindow(HWND hTarget) {
        if (!hTarget || !IsWindow(hTarget)) return;

        // Validation: Don't minimize self or desktop/taskbar
        if (hTarget == GetDesktopWindow() || hTarget == FindWindowW(L"Shell_TrayWnd", nullptr)) return;
        
        // Check if already hidden
        for (const auto& hw : m_hiddenWindows) {
            if (hw.hWnd == hTarget) return; 
        }

        // Get Icon
        HICON hIcon = (HICON)SendMessageW(hTarget, WM_GETICON, ICON_SMALL, 0);
        if (!hIcon) hIcon = (HICON)GetClassLongPtrW(hTarget, GCLP_HICONSM);
        if (!hIcon) hIcon = LoadIconW(nullptr, IDI_APPLICATION); // Fallback

        // Setup Tray Icon
        NOTIFYICONDATAW nid = { sizeof(NOTIFYICONDATAW) };
        nid.hWnd = m_mainWindow;
        nid.uID = static_cast<UINT>(reinterpret_cast<UINT_PTR>(hTarget)); // Unique ID based on HWND
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = hIcon;
        
        // Get Window Title for Tooltip
        GetWindowTextW(hTarget, nid.szTip, 128);

        if (Shell_NotifyIconW(NIM_ADD, &nid)) {
            ShowWindow(hTarget, SW_HIDE);
            m_hiddenWindows.push_back({ hTarget, nid });
            SaveState();
        }
    }

    // Wrapper for hotkey - minimizes the currently focused window
    void MinimizeForegroundWindow() {
        HWND hTarget = GetForegroundWindow();
        MinimizeWindow(hTarget);
    }

    // Add foreground window to auto-minimize list (Win+Shift+A)
    void AddForegroundToAutoList() {
        HWND hTarget = GetForegroundWindow();
        if (!hTarget) return;

        DWORD pid = 0;
        GetWindowThreadProcessId(hTarget, &pid);
        if (pid == 0) {
            ShowBalloonTip(L"Error", L"Could not determine process.");
            return;
        }

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProcess) {
            ShowBalloonTip(L"Error", L"Could not open process.");
            return;
        }

        wchar_t processPath[MAX_PATH];
        if (!GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH)) {
            CloseHandle(hProcess);
            ShowBalloonTip(L"Error", L"Could not get process path.");
            return;
        }
        CloseHandle(hProcess);

        std::wstring procPath = processPath;

        // Check if already exists
        bool exists = false;
        for (const auto& s : g_autoMinimizeList) {
            if (_wcsicmp(s.c_str(), procPath.c_str()) == 0) {
                exists = true;
                break;
            }
        }

        if (exists) {
            ShowBalloonTip(L"Info", L"Application is already in auto-minimize list.");
        } else {
            g_autoMinimizeList.push_back(procPath);
            SaveAutoList();
            
            // Refresh dialog if open
            if (g_hSettingsDlg && IsWindow(g_hSettingsDlg)) {
                RefreshAppList(g_hSettingsDlg);
            }
            
            wchar_t msg[MAX_PATH + 50];
            swprintf_s(msg, L"Added to auto-minimize:\n%s", procPath.c_str());
            ShowBalloonTip(L"Success", msg);
        }
    }

    // Show balloon tip notification in tray
    void ShowBalloonTip(const wchar_t* title, const wchar_t* msg) {
        NOTIFYICONDATAW nid = { sizeof(NOTIFYICONDATAW) };
        nid.hWnd = m_mainWindow;
        nid.uID = 0; // Main icon
        nid.uFlags = NIF_INFO;
        wcscpy_s(nid.szInfoTitle, title);
        wcscpy_s(nid.szInfo, msg);
        nid.dwInfoFlags = NIIF_INFO;
        Shell_NotifyIconW(NIM_MODIFY, &nid);
    }

    void RestoreWindowById(UINT uID) {
        auto it = std::find_if(m_hiddenWindows.begin(), m_hiddenWindows.end(), 
            [uID](const HiddenWindow& hw) { return hw.iconData.uID == uID; });

        if (it != m_hiddenWindows.end()) {
            HWND hwnd = it->hWnd;
            
            // Add to restoring set to prevent immediate re-minimization
            g_restoringWindows.insert(hwnd);
            
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
            Shell_NotifyIconW(NIM_DELETE, &it->iconData);
            m_hiddenWindows.erase(it);
            SaveState();
            
            // Remove from restoring set after 500ms delay
            SetTimer(m_mainWindow, (UINT_PTR)hwnd, 500, [](HWND, UINT, UINT_PTR idEvent, DWORD) {
                g_restoringWindows.erase((HWND)idEvent);
            });
        }
    }

    void RestoreAllWindows() {
        for (auto& hw : m_hiddenWindows) {
            // Add to restoring set to prevent immediate re-minimization
            g_restoringWindows.insert(hw.hWnd);
            ShowWindow(hw.hWnd, SW_SHOW);
            Shell_NotifyIconW(NIM_DELETE, &hw.iconData);
        }
        m_hiddenWindows.clear();
        SaveState();
        
        // Clear restoring set after 500ms delay
        SetTimer(m_mainWindow, 9999, 500, [](HWND, UINT, UINT_PTR, DWORD) {
            g_restoringWindows.clear();
        });
    }

    void CreateTrayIcon() {
        NOTIFYICONDATAW nid = { sizeof(NOTIFYICONDATAW) };
        nid.hWnd = m_mainWindow;
        nid.uID = 0; // ID 0 is for the main app icon
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCE(101)); // Icon resource ID 101
        wcscpy_s(nid.szTip, APP_TITLE);
        Shell_NotifyIconW(NIM_ADD, &nid);
    }

    void CreateTrayMenu() {
        m_trayMenu = CreatePopupMenu();
        AppendMenuW(m_trayMenu, MF_STRING, MENU_RESTORE_ALL_ID, L"Restore All Windows");
        AppendMenuW(m_trayMenu, MF_STRING, MENU_SETTINGS_ID, L"Settings...");
        AppendMenuW(m_trayMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(m_trayMenu, MF_STRING, MENU_EXIT_ID, L"Exit");
    }

    // --- State Management (Modernized) ---
    void SaveState(bool clear = false) {
        if (clear) {
            std::filesystem::remove(DATA_FILENAME);
            return;
        }

        std::ofstream outFile(DATA_FILENAME, std::ios::binary | std::ios::trunc);
        if (!outFile) return;

        for (const auto& hw : m_hiddenWindows) {
            // Save HWND as 64-bit integer (safe for x64)
            uint64_t handleVal = reinterpret_cast<uint64_t>(hw.hWnd);
            outFile.write(reinterpret_cast<const char*>(&handleVal), sizeof(handleVal));
        }
    }

    void LoadState() {
        if (!std::filesystem::exists(DATA_FILENAME)) return;

        std::ifstream inFile(DATA_FILENAME, std::ios::binary);
        if (!inFile) return;

        uint64_t handleVal;
        int restoredCount = 0;
        
        while (inFile.read(reinterpret_cast<char*>(&handleVal), sizeof(handleVal))) {
            HWND hwnd = reinterpret_cast<HWND>(handleVal);
            if (IsWindow(hwnd)) {
                // Re-minimize valid windows after a crash
                NOTIFYICONDATAW nid = { sizeof(NOTIFYICONDATAW) };
                nid.hWnd = m_mainWindow;
                nid.uID = static_cast<UINT>(handleVal);
                nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
                nid.uCallbackMessage = WM_TRAYICON;
                
                HICON hIcon = (HICON)SendMessageW(hwnd, WM_GETICON, ICON_SMALL, 0);
                if (!hIcon) hIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
                if (!hIcon) hIcon = LoadIconW(nullptr, IDI_APPLICATION);
                nid.hIcon = hIcon;

                GetWindowTextW(hwnd, nid.szTip, 128);

                if (Shell_NotifyIconW(NIM_ADD, &nid)) {
                    ShowWindow(hwnd, SW_HIDE);
                    m_hiddenWindows.push_back({ hwnd, nid });
                    restoredCount++;
                }
            }
        }
        
        if (restoredCount > 0) {
            std::wstring msg = L"Restored " + std::to_wstring(restoredCount) + L" hidden windows from previous session.";
            MessageBoxW(nullptr, msg.c_str(), APP_TITLE, MB_OK | MB_ICONINFORMATION);
        }
    }
};

// Main Entry Point (Unicode)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    TraymondApp app(hInstance);
    if (app.Initialize()) {
        app.Run();
    }
    return 0;
}
