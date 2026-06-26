cmake_minimum_required(VERSION 3.26)

project(ReactMUIOpenCV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(httplib CONFIG REQUIRED)
find_package(ixwebsocket CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Tesseract CONFIG REQUIRED)

set(WEBVIEW2_SDK_ROOT "$ENV{USERPROFILE}/.nuget/packages/microsoft.web.webview2/1.0.3800.47/build/native" CACHE PATH "Microsoft WebView2 native SDK root")
set(WEBVIEW2_INCLUDE_DIR "${WEBVIEW2_SDK_ROOT}/include")
set(WEBVIEW2_LOADER_STATIC_LIB "${WEBVIEW2_SDK_ROOT}/x64/WebView2LoaderStatic.lib")

add_executable(ReactMUIOpenCV
  src/app/AppContext.cpp
  src/common/HttpUtils.cpp
  src/common/OpenCvUtils.cpp
  src/common/Random.cpp
  src/common/StringUtils.cpp
  src/common/Time.cpp
  src/image/DnnModelCatalog.cpp
  src/image/ImageDnnFilters.cpp
  src/image/ImageFilters.cpp
  src/image/ImageResultStore.cpp
  src/jobs/JobQueue.cpp
  src/logging/LogStore.cpp
  src/main.cpp
  src/security/RemoteAccessManager.cpp
  src/server/ApiServer.cpp
  src/server/ApiServerVisionRoutes.cpp
  src/server/EventHub.cpp
  src/server/NetworkInfo.cpp
  src/server/WebSocketGateway.cpp
  src/storage/CalibrationStore.cpp
  src/storage/PerformanceBenchmarkStore.cpp
  src/storage/PipelineStore.cpp
  src/storage/SettingsStore.cpp
  src/storage/VideoDiagnosticsStore.cpp
  src/storage/VideoTrackingStore.cpp
  src/video/VideoService.cpp
  src/vision/CalibrationService.cpp
  src/vision/ContourExtractionService.cpp
  src/vision/PerformanceBenchmarkService.cpp
  src/vision/PipelineExecutor.cpp
  src/vision/TesseractOcr.cpp
)

target_link_libraries(ReactMUIOpenCV
  PRIVATE
    httplib::httplib
    ixwebsocket::ixwebsocket
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
    spdlog::spdlog
    Tesseract::libtesseract
)

if(WIN32)
  target_link_libraries(ReactMUIOpenCV PRIVATE Bcrypt Ws2_32 Winhttp)
endif()

target_compile_definitions(ReactMUIOpenCV
  PRIVATE
    APP_STATIC_ROOT="${CMAKE_SOURCE_DIR}/../frontend/dist"
    APP_TESSDATA_ROOT="${CMAKE_SOURCE_DIR}/../models/tessdata"
)

if(MSVC)
  target_compile_options(ReactMUIOpenCV PRIVATE /W4 /permissive-)
else()
  target_compile_options(ReactMUIOpenCV PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32 AND EXISTS "${WEBVIEW2_INCLUDE_DIR}/WebView2.h" AND EXISTS "${WEBVIEW2_LOADER_STATIC_LIB}")
  add_executable(ReactMUIOpenCVApp WIN32
    src/host/WebViewHost.cpp
  )

  target_include_directories(ReactMUIOpenCVApp PRIVATE "${WEBVIEW2_INCLUDE_DIR}")
  target_link_libraries(ReactMUIOpenCVApp
    PRIVATE
      "${WEBVIEW2_LOADER_STATIC_LIB}"
      Ole32
      Shell32
      Shlwapi
      User32
      Winhttp
  )
  target_compile_definitions(ReactMUIOpenCVApp PRIVATE UNICODE _UNICODE)

  if(MSVC)
    target_compile_options(ReactMUIOpenCVApp PRIVATE /W4 /permissive-)
  endif()
else()
  message(WARNING "WebView2 SDK was not found. ReactMUIOpenCVApp desktop host target will not be built.")
endif()
