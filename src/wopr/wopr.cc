/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include <cassert>
#include <fstream>
#include <stdexcept>

#include "wopr.h"
#include "imgui_app.h"
#include "utility/log.h"

void read_file(const std::filesystem::path &filename)
{
  std::ifstream file(filename);
  if (!file)
      throw std::runtime_error("Failed to open file for reading.");

  std::streampos position(0);

  while (true)
  {
    std::string line;

    // Seek to the last known position.
    file.clear();
    file.seekg(position);

    while (std::getline(file, line))
      if (!file.eof())
      {
        // Update the position for the next read.
        position = file.tellg();

        buffer_.push(line);
      }

    if (file.bad())
      throw std::runtime_error("Error occurred while reading the file.");

    assert(file.eof());

    // Small delay before checking for new data.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

int main()
{
  imgui_app::program prg{"WOPR"};

  const auto render_gui([]
  {
    ImGui::Begin("Simulation Data");

    //if (!ImGui::Button("Pause"))
    //  arena.simulate();

    ImGui::End();
  });

  const auto render_arena([&](imgui_app::program &prg)
  {
  });

  prg.run(render_gui, render_arena);
}
