#include "app/AppContext.h"
#include "logging/LogStore.h"

int main(int argc, char* argv[]) {
  app::configure_file_logging();

  app::AppContext app_context(app::build_app_runtime_config(argc, argv));
  return app_context.run();
}
