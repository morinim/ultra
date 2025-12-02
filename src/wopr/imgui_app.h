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

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
//#include <SDL3_image/SDL_image.h>
//#include <SDL3_ttf/SDL_ttf.h>
#include "imgui/backends/imgui_impl_sdl3.h"
#include "imgui/backends/imgui_impl_sdlrenderer3.h"

#include "implot/implot.h"

namespace imgui_app
{

struct window_size
{
  int width;
  int height;
};

class window
{
public:
  struct settings
  {
    std::string title {};

    int width  {1000};
    int height  {720};

    SDL_WindowFlags flags {SDL_WINDOW_RESIZABLE
                           | SDL_WINDOW_HIGH_PIXEL_DENSITY};
  };

  explicit window(const settings &settings);
  ~window();

  window(const window &) = delete;
  window(window &&) = delete;
  window &operator=(const window &) = delete;
  window &operator=(window &&) = delete;

  [[nodiscard]] SDL_Renderer *get_native_renderer() const;
  [[nodiscard]] SDL_Window *get_native_window() const;

private:
  SDL_Window *window_ {nullptr};
  SDL_Renderer *renderer_ {nullptr};
};

namespace resources
{

[[nodiscard]] ::std::filesystem::path resource_path(
  const ::std::filesystem::path &);

[[nodiscard]] ::std::filesystem::path font_path(const std::string_view &);

}  // namespace resources

class program
{
public:
  struct settings
  {
    window::settings w_related {};

    bool demo {false};
  };

  explicit program(const settings &);
  ~program();

  program(const program &) = delete;
  program(program &&) = delete;
  program &operator=(const program &) = delete;
  program& operator=(program &&) = delete;

  void run(std::function<void(const program &, bool *)>);
  void stop();

  [[nodiscard]] window &get_window() { return *window_; }

  SDL_Rect free_area() const;

  void on_event(const SDL_WindowEvent &);
  void on_minimize();
  void on_show();
  void on_close();

private:
  std::unique_ptr<window> window_ {nullptr};

  settings settings_ {};

  int menu_height_ {0};

  bool running_ {true};
  bool minimized_ {false};
  bool show_main_panel_ {true};
  bool show_demo_panel_ {false};
};

}  // namespace imgui_app

#endif  // include guard
