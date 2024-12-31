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

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_sdlrenderer2.h"

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
    int width     {1000};
    int height     {720};
  };

  explicit window(const settings &settings);
  ~window();

  window(const window &) = delete;
  window(window &&) = delete;
  window &operator=(window) = delete;
  window &operator=(window &&) = delete;

  [[nodiscard]] SDL_Window *get_native_window() const;
  [[nodiscard]] SDL_Renderer *get_native_renderer() const;

 private:
  SDL_Window *window_ {nullptr};
  SDL_Renderer *renderer_ {nullptr};
};

namespace dpi_handler
{

[[nodiscard]] float get_scale();
[[nodiscard]] window_size get_dpi_aware_window_size(const window::settings &);

void set_render_scale(SDL_Renderer *);
void set_global_font_scaling(ImGuiIO *);

}  // namespace dpi_handler

namespace resources
{

[[nodiscard]] ::std::filesystem::path resource_path(
  const ::std::filesystem::path &);

[[nodiscard]] ::std::filesystem::path font_path(const std::string_view &);

}  // namespace resources

class program
{
public:
  explicit program(const std::string &);
  ~program();

  program(const program &) = delete;
  program(program &&) = delete;
  program &operator=(program) = delete;
  program& operator=(program &&) = delete;

  void run(std::function<void(const program &)>);
  void stop();

  window &get_window() { return *window_; }

  SDL_Rect free_area() const;

  void on_event(const SDL_WindowEvent &);
  void on_minimize();
  void on_shown();
  void on_close();

 private:
  std::unique_ptr<window> window_ {nullptr};

  int menu_height_ {0};

  bool running_ {true};
  bool minimized_ {false};
  bool show_main_panel_ {true};
  bool show_demo_panel_ {false};
};

}  // namespace imgui_app

#endif  // include guard
