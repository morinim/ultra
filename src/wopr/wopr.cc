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

int main(int argc, char *argv[])
{
  using ultra::wopr::cmdl_result;

  const auto result(ultra::wopr::parse_args(argc, argv));

  if (result == cmdl_result::error)
  {
    std::cerr << "Use `--help` switch for command line description.\n\n"
              << "People sometimes make mistakes.\n";
    return EXIT_FAILURE;
  }

  if (result == cmdl_result::help)
  {
    ultra::wopr::cmdl_usage();
    return EXIT_SUCCESS;
  }

  imgui_app::program::settings settings;
  settings.w_related.title = "WOPR";
  settings.w_related.flags |= SDL_WINDOW_MAXIMIZED;
  settings.demo = ultra::wopr::imgui_demo_panel;

  if (result == cmdl_result::summary)
    ultra::wopr::rs::summary::start(settings);
  else if (result == cmdl_result::monitor)
    ultra::wopr::monitor::start(settings);
  else if (result == cmdl_result::run)
  {
    if (!ultra::wopr::rs::run::start(settings))
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
