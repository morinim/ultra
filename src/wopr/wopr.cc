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

#include <cstdlib>
#include <iostream>
#include <type_traits>
#include <utility>
#include <variant>

int main(int argc, char *argv[])
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
      using options_t = decltype(options);

      if constexpr (std::is_same_v<options_t, ultra::wopr::help_command>)
      {
        ultra::wopr::cmdl_usage();
        return EXIT_SUCCESS;
      }
      else
      {
        imgui_app::program::settings settings;
        settings.w_related.title = "WOPR";
        settings.w_related.flags |= SDL_WINDOW_MAXIMIZED;
        settings.demo = options.imgui_demo;

        if constexpr (std::is_same_v<options_t,
                                     ultra::wopr::monitor::options>)
          ultra::wopr::monitor::start(settings, std::move(options));
        else if constexpr (std::is_same_v<options_t,
                                          ultra::wopr::rs::run::options>)
        {
          if (!ultra::wopr::rs::run::start(settings, std::move(options)))
            return EXIT_FAILURE;
        }
        else
          ultra::wopr::rs::summary::start(settings, std::move(options));

        return EXIT_SUCCESS;
      }
    },
    std::move(*result));
}
