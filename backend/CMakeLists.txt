cmake_minimum_required(VERSION 3.26)

project(ReactMUIOpenCV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(httplib CONFIG REQUIRED)
find_package(ixwebsocket CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(ReactMUIOpenCV
  src/common/HttpUtils.cpp
  src/common/Random.cpp
  src/common/Time.cpp
  src/image/ImageFilters.cpp
  src/image/ImageResultStore.cpp
  src/jobs/JobQueue.cpp
  src/logging/LogStore.cpp
  src/main.cpp
  src/security/RemoteAccessManager.cpp
  src/server/ApiServer.cpp
  src/server/EventHub.cpp
  src/server/NetworkInfo.cpp
  src/server/WebSocketGateway.cpp
  src/storage/PipelineStore.cpp
  src/storage/SettingsStore.cpp
)

target_link_libraries(ReactMUIOpenCV
  PRIVATE
    httplib::httplib
    ixwebsocket::ixwebsocket
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
    spdlog::spdlog
)

if(WIN32)
  target_link_libraries(ReactMUIOpenCV PRIVATE Bcrypt Ws2_32)
endif()

target_compile_definitions(ReactMUIOpenCV
  PRIVATE
    APP_STATIC_ROOT="${CMAKE_SOURCE_DIR}/../frontend/dist"
)

if(MSVC)
  target_compile_options(ReactMUIOpenCV PRIVATE /W4 /permissive-)
else()
  target_compile_options(ReactMUIOpenCV PRIVATE -Wall -Wextra -Wpedantic)
endif()
