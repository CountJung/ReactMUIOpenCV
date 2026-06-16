#include "app/AppContext.h"
#include "logging/LogStore.h"

#include <memory>
#include <utility>

int main(int argc, char* argv[]) {
  auto config = app::build_app_runtime_config(argc, argv);
  app::configure_file_logging_from_settings_file(config.data_dir / "settings.json");

  auto app_context = std::make_unique<app::AppContext>(std::move(config));
  return app_context->run();
}
