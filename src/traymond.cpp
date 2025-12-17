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
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <memory>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

// Enable Visual Styles for modern Windows UI
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Constants using modern constexpr
constexpr UINT WM_TRAYICON = WM_APP + 1;
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

// Data files
const std::wstring DATA_FILENAME = L"traymond_recovery.dat";
const std::wstring AUTO_MINIMIZE_FILE = L"traymond_auto.txt";

struct HiddenWindow {
    HWND hWnd;
    NOTIFYICONDATAW iconData;
};

// Global auto-minimize list
std::vector<std::wstring> g_autoMinimizeList;

// Global ImageList for dialog icons
HIMAGELIST g_hImageList = nullptr;

// Track settings dialog state
HWND g_hSettingsDlg = nullptr;

// Startup registry functions
void SetStartup(bool enable) {
    HKEY hKey;
    const wchar_t* czAppName = L"Traymond";
    const wchar_t* czRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    wchar_t szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, MAX_PATH);

    if (RegOpenKeyExW(HKEY_CURRENT_USER, czRunKey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            DWORD pathLen = (wcslen(szPath) + 1) * sizeof(wchar_t);
            RegSetValueExW(hKey, czAppName, 0, REG_SZ, (BYTE*)szPath, pathLen);
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
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_ADD) {
            // Open file browser dialog
            wchar_t filename[MAX_PATH] = { 0 };
            OPENFILENAMEW ofn = { 0 };
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hDlg;
            ofn.lpstrFilter = L"Executables (*.exe)\\0*.exe\\0All Files (*.*)\\0*.*\\0";
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
        RestoreAllWindows();
        if (m_mainWindow) DestroyWindow(m_mainWindow);
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

        if (!RegisterHotKey(m_mainWindow, 1, MOD_WIN | MOD_SHIFT | MOD_NOREPEAT, 'Z')) {
            MessageBoxW(nullptr, L"Could not register hotkey Win+Shift+Z.", APP_TITLE, MB_ICONERROR);
            return false;
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
            MinimizeForegroundWindow();
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

    void MinimizeForegroundWindow() {
        HWND hTarget = GetForegroundWindow();
        if (!hTarget) return;

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

    void RestoreWindowById(UINT uID) {
        auto it = std::find_if(m_hiddenWindows.begin(), m_hiddenWindows.end(), 
            [uID](const HiddenWindow& hw) { return hw.iconData.uID == uID; });

        if (it != m_hiddenWindows.end()) {
            ShowWindow(it->hWnd, SW_SHOW);
            SetForegroundWindow(it->hWnd);
            Shell_NotifyIconW(NIM_DELETE, &it->iconData);
            m_hiddenWindows.erase(it);
            SaveState();
        }
    }

    void RestoreAllWindows() {
        for (auto& hw : m_hiddenWindows) {
            ShowWindow(hw.hWnd, SW_SHOW);
            Shell_NotifyIconW(NIM_DELETE, &hw.iconData);
        }
        m_hiddenWindows.clear();
        SaveState();
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
