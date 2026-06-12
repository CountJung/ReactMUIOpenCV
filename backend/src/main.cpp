#include "app/AppContext.h"
#include "logging/LogStore.h"

#include <memory>

int main(int argc, char* argv[]) {
  app::configure_file_logging();

  auto app_context = std::make_unique<app::AppContext>(app::build_app_runtime_config(argc, argv));
  return app_context->run();
}
