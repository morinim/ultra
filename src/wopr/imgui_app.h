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

#if !defined(ULTRA_IMGUI_APP_H)
#define      ULTRA_IMGUI_APP_H

#include <functional>
#include <string>

#include <SDL3/SDL.h>

namespace imgui_app
{

class program
{
public:
  struct settings
  {
    std::string title {};

    int width {1000};
    int height {720};

    SDL_WindowFlags flags {SDL_WINDOW_RESIZABLE
                           | SDL_WINDOW_HIGH_PIXEL_DENSITY};

    bool demo {false};
  };

  explicit program(const settings &);
  ~program();

  program(const program &) = delete;
  program(program &&) = delete;
  program &operator=(const program &) = delete;
  program &operator=(program &&) = delete;

  void run(std::function<void(const program &, bool *)>);
  void stop();

  SDL_Rect free_area() const;

private:
  settings settings_ {};

  SDL_Window *window_ {nullptr};
  SDL_Renderer *renderer_ {nullptr};

  int menu_height_ {0};

  bool running_ {true};
  bool show_main_panel_ {true};
  bool show_demo_panel_ {false};
};

}  // namespace imgui_app

#endif  // include guard
