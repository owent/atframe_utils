
#include <cli/cmd_option.h>
#include <cli/cmd_option_phoenix.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>

#include "random/uuid_generator.h"

static void on_error(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param params, bool& need_exit) {
  std::stringstream args;
  for (ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::cmd_option_list::cmd_array_type::const_iterator iter =
           params.get_cmd_array().begin();
       iter != params.get_cmd_array().end(); ++iter) {
    args << " \"" << iter->first << '"';
  }
  for (size_t i = 0; i < params.get_params_number(); ++i) {
    if (params[i]) {
      args << " \"" << params[i]->to_cpp_string() << '"';
    }
  }

  ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::cmd_option_list::value_type err_msg = params.get("@ErrorMsg");
  std::cerr << "Unknown Options: " << args.str() << (err_msg ? err_msg->to_string() : "") << std::endl;

  need_exit = true;
}

void on_help(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param, ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::cmd_option* self,
             bool& need_exit) {
  std::cout << "Usage: uuidgen [options...]" << std::endl;
  std::cout << (*self);

  need_exit = true;
}

void on_version(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::callback_param, bool& need_exit) {
  std::cout << "uuidgen from atframe_utils " << ATFRAMEWORK_UTILS_VERSION;

#if (defined(ATFRAMEWORK_UTILS_ENABLE_LIBUUID) && ATFRAMEWORK_UTILS_ENABLE_LIBUUID)
  std::cout << " - libuuid/uuid";
#elif (defined(ATFRAMEWORK_UTILS_ENABLE_UUID_WINRPC) && ATFRAMEWORK_UTILS_ENABLE_UUID_WINRPC)
  std::cout << " - Rpcrt";
#endif

  std::cout << std::endl;
  need_exit = true;
}

int main(int argc, char* argv[]) {
  ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::cmd_option::ptr_type opts =
      ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::cmd_option::create();
  bool using_random = false;
  bool using_time = false;
  bool need_exit = false;

  opts->bind_cmd("@OnError", on_error, std::ref(need_exit));
  opts->bind_cmd("-r, --random", ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::phoenix::set_const(using_random, true))
      ->set_help_msg("Generate UUID using random engine(V4)");
  opts->bind_cmd("-t, --time", ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::phoenix::set_const(using_time, true))
      ->set_help_msg("Generate UUID using time engine(V1)");

  opts->bind_cmd("-h, --help", on_help, opts.get(), std::ref(need_exit))->set_help_msg("Show help messages");

  opts->bind_cmd("-V, --version", on_version, std::ref(need_exit))->set_help_msg("Show version and exit");

  opts->start(argc, argv);
  if (need_exit) {
    return 0;
  }

  if (using_random) {
    std::cout << ATFRAMEWORK_UTILS_NAMESPACE_ID::random::uuid_generator::generate_string_random() << std::endl;
  } else if (using_time) {
    std::cout << ATFRAMEWORK_UTILS_NAMESPACE_ID::random::uuid_generator::generate_string_time() << std::endl;
  } else {
    std::cout << ATFRAMEWORK_UTILS_NAMESPACE_ID::random::uuid_generator::generate_string() << std::endl;
  }
  return 0;
}
