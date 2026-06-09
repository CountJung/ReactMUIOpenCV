#include <WebView2.h>
#include <Windows.h>
#include <WinHttp.h>
#include <wrl.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {

constexpr wchar_t kAppTitle[] = L"ReactMUIOpenCV";
constexpr wchar_t kLocalUrl[] = L"http://127.0.0.1:18730";
constexpr wchar_t kHealthHost[] = L"127.0.0.1";
constexpr wchar_t kHealthPath[] = L"/api/health";
constexpr INTERNET_PORT kHealthPort = 18730;

HWND g_window = nullptr;
ComPtr<ICoreWebView2Controller> g_webview_controller;
ComPtr<ICoreWebView2> g_webview;
PROCESS_INFORMATION g_backend_process{};
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

bool health_check() {
  HINTERNET session = WinHttpOpen(
      L"ReactMUIOpenCVApp/1.0",
      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
      WINHTTP_NO_PROXY_NAME,
      WINHTTP_NO_PROXY_BYPASS,
      0);
  if (!session) {
    return false;
  }

  HINTERNET connect = WinHttpConnect(session, kHealthHost, kHealthPort, 0);
  if (!connect) {
    WinHttpCloseHandle(session);
    return false;
  }

  HINTERNET request = WinHttpOpenRequest(connect, L"GET", kHealthPath, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
  bool ok = false;
  if (request && WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
      WinHttpReceiveResponse(request, nullptr)) {
    DWORD status_code = 0;
    DWORD status_size = sizeof(status_code);
    if (WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status_code,
            &status_size,
            WINHTTP_NO_HEADER_INDEX)) {
      ok = status_code == 200;
    }
  }

  if (request) {
    WinHttpCloseHandle(request);
  }
  WinHttpCloseHandle(connect);
  WinHttpCloseHandle(session);
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
          &g_backend_process)) {
    MessageBoxW(g_window, L"Failed to start the local backend server.", kAppTitle, MB_ICONERROR);
    return false;
  }

  g_started_backend = true;
  if (!wait_for_backend()) {
    MessageBoxW(g_window, L"The local backend server did not become ready.", kAppTitle, MB_ICONERROR);
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
  if (g_started_backend && g_backend_process.hProcess) {
    TerminateProcess(g_backend_process.hProcess, 0);
  }
  if (g_backend_process.hThread) {
    CloseHandle(g_backend_process.hThread);
  }
  if (g_backend_process.hProcess) {
    CloseHandle(g_backend_process.hProcess);
  }
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  switch (message) {
    case WM_SIZE:
      resize_webview();
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

  g_window = CreateWindowExW(
      0,
      window_class.lpszClassName,
      kAppTitle,
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      1280,
      820,
      nullptr,
      nullptr,
      instance,
      nullptr);

  if (!g_window) {
    return 1;
  }

  ShowWindow(g_window, show_command);
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
