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
#include "imgui_app.h"
#include "monitor.h"
#include "results.h"

#include <SDL3/SDL_video.h>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <utility>
#include <variant>

namespace
{

using namespace ultra::wopr;

[[nodiscard]] imgui_app::program::settings make_settings(bool imgui_demo)
{
  imgui_app::program::settings settings;
  settings.w_related.title = "WOPR";
  settings.w_related.flags |= SDL_WINDOW_MAXIMIZED;
  settings.demo = imgui_demo;
  return settings;
}

[[nodiscard]] int execute(help_command)
{
  cmdl_usage();
  return EXIT_SUCCESS;
}

[[nodiscard]] int execute(monitor::options options)
{
  const auto demo(options.imgui_demo);
  monitor::start(make_settings(demo), std::move(options));
  return EXIT_SUCCESS;
}

[[nodiscard]] int execute(rs::run::options options)
{
  const auto demo(options.imgui_demo);
  return rs::run::start(make_settings(demo), std::move(options))
         ? EXIT_SUCCESS : EXIT_FAILURE;
}

[[nodiscard]] int execute(rs::summary::options options)
{
  const auto demo(options.imgui_demo);
  rs::summary::start(make_settings(demo), std::move(options));
  return EXIT_SUCCESS;
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
