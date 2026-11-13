/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "command_line.h"
#include "monitor.h"
#include "results.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <utility>
#include <variant>

namespace
{

using namespace ultra::wopr;

[[nodiscard]] int execute(const help_command &help)
{
  std::cout << help.message;
  return EXIT_SUCCESS;
}

[[nodiscard]] int execute(monitor::options options)
{
#if defined(ULTRA_WOPR_GUI)
  monitor::start(std::move(options));
  return EXIT_SUCCESS;
#else
  (void)options;
  std::cerr << "The `monitor` subcommand requires GUI support (WOPR was compiled in headless mode).\n";
  return EXIT_FAILURE;
#endif
}

[[nodiscard]] int execute(rs::run::options options)
{
  return rs::run::start(std::move(options)) ? EXIT_SUCCESS : EXIT_FAILURE;
}

[[nodiscard]] int execute(rs::summary::options options)
{
#if defined(ULTRA_WOPR_GUI)
  rs::summary::start(std::move(options));
  return EXIT_SUCCESS;
#else
  (void)options;
  std::cerr << "The `summary` subcommand requires GUI support (WOPR was compiled in headless mode).\n";
  return EXIT_FAILURE;
#endif
}

}  // namespace


int main(int argc, char *argv[])
{
  try
  {
    auto result(ultra::wopr::parse_args(argc, argv));

    if (!result)
    {
      std::cerr << result.error() << "\n\n"
                << "Use `--help` switch for command line description.\n\n"
                << "People sometimes make mistakes.\n";
      return EXIT_FAILURE;
    }

    return std::visit(
      [](auto options)
      {
        return execute(std::move(options));
      },
      std::move(*result));
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
  catch (...)
  {
    std::cerr << "Fatal error: unexpected exception.\n";
    return EXIT_FAILURE;
  }
}
