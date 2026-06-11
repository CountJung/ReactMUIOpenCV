#include <WebView2.h>
#include <Windows.h>
#include <WinHttp.h>
#include <wrl.h>

#include <chrono>
#include <cwchar>
#include <filesystem>
#include <string>
#include <thread>
#include <utility>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {

constexpr wchar_t kAppTitle[] = L"ReactMUIOpenCV";
constexpr wchar_t kLocalUrl[] = L"http://127.0.0.1:18730";
constexpr wchar_t kHealthHost[] = L"127.0.0.1";
constexpr wchar_t kHealthPath[] = L"/api/health";
constexpr wchar_t kWindowStateFile[] = L"window-state.ini";
constexpr INTERNET_PORT kHealthPort = 18730;
constexpr int kDefaultWindowWidth = 1280;
constexpr int kDefaultWindowHeight = 820;
constexpr int kMinimumWindowWidth = 900;
constexpr int kMinimumWindowHeight = 640;

struct WindowState {
  RECT normal_rect{CW_USEDEFAULT, CW_USEDEFAULT, kDefaultWindowWidth, kDefaultWindowHeight};
  bool maximized = false;
  bool restored = false;
};

class WinHandle {
 public:
  WinHandle() = default;
  explicit WinHandle(HANDLE handle) : handle_(handle) {}
  ~WinHandle() { reset(); }

  WinHandle(const WinHandle&) = delete;
  WinHandle& operator=(const WinHandle&) = delete;

  WinHandle(WinHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  WinHandle& operator=(WinHandle&& other) noexcept {
    if (this != &other) {
      reset(std::exchange(other.handle_, nullptr));
    }
    return *this;
  }

  HANDLE get() const { return handle_; }
  explicit operator bool() const { return handle_ && handle_ != INVALID_HANDLE_VALUE; }

  void reset(HANDLE handle = nullptr) {
    if (*this) {
      CloseHandle(handle_);
    }
    handle_ = handle;
  }

 private:
  HANDLE handle_ = nullptr;
};

class WinHttpHandle {
 public:
  WinHttpHandle() = default;
  explicit WinHttpHandle(HINTERNET handle) : handle_(handle) {}
  ~WinHttpHandle() { reset(); }

  WinHttpHandle(const WinHttpHandle&) = delete;
  WinHttpHandle& operator=(const WinHttpHandle&) = delete;

  WinHttpHandle(WinHttpHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

  WinHttpHandle& operator=(WinHttpHandle&& other) noexcept {
    if (this != &other) {
      reset(std::exchange(other.handle_, nullptr));
    }
    return *this;
  }

  HINTERNET get() const { return handle_; }
  explicit operator bool() const { return handle_ != nullptr; }

  void reset(HINTERNET handle = nullptr) {
    if (handle_) {
      WinHttpCloseHandle(handle_);
    }
    handle_ = handle;
  }

 private:
  HINTERNET handle_ = nullptr;
};

HWND g_window = nullptr;
ComPtr<ICoreWebView2Controller> g_webview_controller;
ComPtr<ICoreWebView2> g_webview;
WinHandle g_backend_process;
WinHandle g_backend_thread;
bool g_started_backend = false;

std::filesystem::path executable_dir() {
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  return std::filesystem::path(path).parent_path();
}

std::filesystem::path local_app_data_dir() {
  wchar_t buffer[MAX_PATH]{};
  const auto length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
  if (length == 0 || length >= MAX_PATH) {
    return executable_dir();
  }

  return std::filesystem::path(buffer) / L"ReactMUIOpenCV";
}

std::filesystem::path window_state_path() {
  return local_app_data_dir() / kWindowStateFile;
}

bool read_ini_int(const std::filesystem::path& path, const wchar_t* key, int* value) {
  wchar_t buffer[32]{};
  GetPrivateProfileStringW(L"Window", key, L"", buffer, static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])), path.c_str());
  if (buffer[0] == L'\0') {
    return false;
  }

  wchar_t* end = nullptr;
  const long parsed = std::wcstol(buffer, &end, 10);
  if (end == buffer || *end != L'\0') {
    return false;
  }

  *value = static_cast<int>(parsed);
  return true;
}

bool is_visible_window_rect(const RECT& rect) {
  const int width = rect.right - rect.left;
  const int height = rect.bottom - rect.top;
  if (width < kMinimumWindowWidth || height < kMinimumWindowHeight) {
    return false;
  }

  const HMONITOR monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);
  if (!monitor) {
    return false;
  }

  MONITORINFO monitor_info{};
  monitor_info.cbSize = sizeof(monitor_info);
  if (!GetMonitorInfoW(monitor, &monitor_info)) {
    return false;
  }

  RECT intersection{};
  return IntersectRect(&intersection, &rect, &monitor_info.rcWork) != FALSE;
}

WindowState load_window_state() {
  WindowState state;
  const auto path = window_state_path();
  if (!std::filesystem::exists(path)) {
    return state;
  }

  RECT loaded{};
  int left = 0;
  int top = 0;
  int right = 0;
  int bottom = 0;
  int maximized = 0;
  if (!read_ini_int(path, L"Left", &left) || !read_ini_int(path, L"Top", &top) ||
      !read_ini_int(path, L"Right", &right) || !read_ini_int(path, L"Bottom", &bottom)) {
    return state;
  }
  (void)read_ini_int(path, L"Maximized", &maximized);
  loaded.left = left;
  loaded.top = top;
  loaded.right = right;
  loaded.bottom = bottom;

  if (!is_visible_window_rect(loaded)) {
    return state;
  }

  state.normal_rect = loaded;
  state.maximized = maximized != 0;
  state.restored = true;
  return state;
}

void write_ini_int(const std::filesystem::path& path, const wchar_t* key, int value) {
  const auto text = std::to_wstring(value);
  WritePrivateProfileStringW(L"Window", key, text.c_str(), path.c_str());
}

void save_window_state(HWND hwnd) {
  WINDOWPLACEMENT placement{};
  placement.length = sizeof(placement);
  if (!GetWindowPlacement(hwnd, &placement)) {
    return;
  }

  const RECT rect = placement.rcNormalPosition;
  if (!is_visible_window_rect(rect)) {
    return;
  }

  const auto directory = local_app_data_dir();
  std::filesystem::create_directories(directory);
  const auto path = directory / kWindowStateFile;

  write_ini_int(path, L"Left", rect.left);
  write_ini_int(path, L"Top", rect.top);
  write_ini_int(path, L"Right", rect.right);
  write_ini_int(path, L"Bottom", rect.bottom);
  write_ini_int(path, L"Maximized", IsZoomed(hwnd) ? 1 : 0);
}

bool health_check() {
  WinHttpHandle session(WinHttpOpen(
      L"ReactMUIOpenCVApp/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
  if (!session) {
    return false;
  }

  WinHttpHandle connect(WinHttpConnect(session.get(), kHealthHost, kHealthPort, 0));
  if (!connect) {
    return false;
  }

  WinHttpHandle request(
      WinHttpOpenRequest(connect.get(), L"GET", kHealthPath, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0));
  bool ok = false;
  if (request && WinHttpSendRequest(request.get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
      WinHttpReceiveResponse(request.get(), nullptr)) {
    DWORD status_code = 0;
    DWORD status_size = sizeof(status_code);
    if (WinHttpQueryHeaders(
            request.get(),
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status_code,
            &status_size,
            WINHTTP_NO_HEADER_INDEX)) {
      ok = status_code == 200;
    }
  }

  return ok;
}

bool wait_for_backend() {
  for (int attempt = 0; attempt < 50; ++attempt) {
    if (health_check()) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
  }

  return false;
}

void shutdown_backend();

bool start_backend_if_needed() {
  if (health_check()) {
    return true;
  }

  const auto backend_exe = executable_dir() / L"ReactMUIOpenCV.exe";
  if (!std::filesystem::exists(backend_exe)) {
    MessageBoxW(g_window, L"ReactMUIOpenCV.exe was not found next to the app host.", kAppTitle, MB_ICONERROR);
    return false;
  }

  STARTUPINFOW startup_info{};
  startup_info.cb = sizeof(startup_info);
  PROCESS_INFORMATION process_info{};
  std::wstring command_line = L"\"" + backend_exe.wstring() + L"\"";
  auto mutable_command_line = command_line;

  if (!CreateProcessW(
          backend_exe.c_str(),
          mutable_command_line.data(),
          nullptr,
          nullptr,
          FALSE,
          CREATE_NO_WINDOW,
          nullptr,
          executable_dir().c_str(),
          &startup_info,
          &process_info)) {
    MessageBoxW(g_window, L"Failed to start the local backend server.", kAppTitle, MB_ICONERROR);
    return false;
  }

  g_backend_process.reset(process_info.hProcess);
  g_backend_thread.reset(process_info.hThread);
  g_started_backend = true;
  if (!wait_for_backend()) {
    MessageBoxW(g_window, L"The local backend server did not become ready.", kAppTitle, MB_ICONERROR);
    shutdown_backend();
    return false;
  }

  return true;
}

void resize_webview() {
  if (!g_webview_controller) {
    return;
  }

  RECT bounds{};
  GetClientRect(g_window, &bounds);
  g_webview_controller->put_Bounds(bounds);
}

void initialize_webview() {
  const auto user_data_folder = local_app_data_dir() / L"WebView2UserData";
  std::filesystem::create_directories(user_data_folder);

  CreateCoreWebView2EnvironmentWithOptions(
      nullptr,
      user_data_folder.c_str(),
      nullptr,
      Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
          [](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
            if (FAILED(result) || !environment) {
              MessageBoxW(g_window, L"Failed to initialize WebView2 environment.", kAppTitle, MB_ICONERROR);
              return result;
            }

            environment->CreateCoreWebView2Controller(
                g_window,
                Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [](HRESULT controller_result, ICoreWebView2Controller* controller) -> HRESULT {
                      if (FAILED(controller_result) || !controller) {
                        MessageBoxW(g_window, L"Failed to create WebView2 controller.", kAppTitle, MB_ICONERROR);
                        return controller_result;
                      }

                      g_webview_controller = controller;
                      g_webview_controller->get_CoreWebView2(&g_webview);
                      resize_webview();
                      g_webview->Navigate(kLocalUrl);
                      return S_OK;
                    })
                    .Get());
            return S_OK;
          })
          .Get());
}

void shutdown_backend() {
  if (g_started_backend && g_backend_process) {
    const auto wait_result = WaitForSingleObject(g_backend_process.get(), 1500);
    if (wait_result == WAIT_TIMEOUT) {
      TerminateProcess(g_backend_process.get(), 0);
      WaitForSingleObject(g_backend_process.get(), 1500);
    }
  }
  g_backend_thread.reset();
  g_backend_process.reset();
  g_started_backend = false;
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  switch (message) {
    case WM_SIZE:
      resize_webview();
      return 0;
    case WM_CLOSE:
      save_window_state(hwnd);
      DestroyWindow(hwnd);
      return 0;
    case WM_DESTROY:
      shutdown_backend();
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProcW(hwnd, message, wparam, lparam);
  }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
  WNDCLASSEXW window_class{};
  window_class.cbSize = sizeof(WNDCLASSEXW);
  window_class.hInstance = instance;
  window_class.lpfnWndProc = window_proc;
  window_class.lpszClassName = L"ReactMUIOpenCVAppWindow";
  window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
  window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

  RegisterClassExW(&window_class);

  const WindowState window_state = load_window_state();
  const int initial_x = window_state.restored ? window_state.normal_rect.left : CW_USEDEFAULT;
  const int initial_y = window_state.restored ? window_state.normal_rect.top : CW_USEDEFAULT;
  const int initial_width =
      window_state.restored ? window_state.normal_rect.right - window_state.normal_rect.left : kDefaultWindowWidth;
  const int initial_height =
      window_state.restored ? window_state.normal_rect.bottom - window_state.normal_rect.top : kDefaultWindowHeight;

  g_window = CreateWindowExW(
      0,
      window_class.lpszClassName,
      kAppTitle,
      WS_OVERLAPPEDWINDOW,
      initial_x,
      initial_y,
      initial_width,
      initial_height,
      nullptr,
      nullptr,
      instance,
      nullptr);

  if (!g_window) {
    return 1;
  }

  ShowWindow(g_window, window_state.maximized ? SW_MAXIMIZE : show_command);
  UpdateWindow(g_window);

  if (!start_backend_if_needed()) {
    return 1;
  }

  initialize_webview();

  MSG message{};
  while (GetMessageW(&message, nullptr, 0, 0)) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }

  return 0;
}
