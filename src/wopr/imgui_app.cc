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

#include <stdexcept>

#include "imgui_app.h"

namespace
{

static const std::string BASE_PATH(SDL_GetBasePath());

}

namespace imgui_app
{

window::window(const window::settings &s)
{
  const auto w_flags(static_cast<SDL_WindowFlags>(
                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));

  const window_size size(dpi_handler::get_dpi_aware_window_size(s));

  window_ = SDL_CreateWindow(s.title.c_str(),
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             size.width, size.height,
                             w_flags);

  auto r_flags(static_cast<SDL_RendererFlags>(
                 SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED));

  renderer_ = SDL_CreateRenderer(window_, -1, r_flags);
  if (!renderer_)
    throw std::runtime_error("Error creating SDL_Renderer");

  SDL_RendererInfo info;
  SDL_GetRendererInfo(renderer_, &info);
  dpi_handler::set_render_scale(renderer_);

  if (const std::filesystem::path ttf_file("/prj/ultra/build/wopr/vectorb.ttf");
      std::filesystem::exists(ttf_file))
  {
    font_ = TTF_OpenFont(ttf_file.string().c_str(), 28);
    if (!font_)
      throw std::runtime_error("Font could not be loaded! TTF_Error");
  }
}

window::~window()
{
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
}

TTF_Font *window::get_native_font() const
{
  return font_;
}

SDL_Window *window::get_native_window() const
{
  return window_;
}

SDL_Renderer *window::get_native_renderer() const
{
  return renderer_;
}

program::program(const settings &s) : settings_(s)
{
  const unsigned flags(SDL_INIT_VIDEO | SDL_INIT_TIMER
                       | SDL_INIT_GAMECONTROLLER);

  if (SDL_Init(flags) != 0)
    throw std::runtime_error(SDL_GetError());

  window_ = std::make_unique<window>(settings_.w_related);
}

program::~program()
{
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  SDL_Quit();
}

void render_text(SDL_Renderer *renderer, TTF_Font *font,
                 const std::string &text, int x, int y, SDL_Color color)
{
  SDL_Surface *surface(TTF_RenderText_Solid(font, text.c_str(), color));
  SDL_Texture *texture(SDL_CreateTextureFromSurface(renderer, surface));
  SDL_Rect dest = {x, y, surface->w, surface->h};

  SDL_RenderCopy(renderer, texture, nullptr, &dest);
  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
}

void program::run(std::function<void (const program &, bool *)> render_main)
{
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO &io(ImGui::GetIO());

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  //const std::string user_config_path(SDL_GetPrefPath(
  //                                     COMPANY_NAMESPACE.c_str(),
  //                                     APP_NAME.c_str()));

  // Absolute imgui.ini path to preserve settings independent of app location.
  //static const std::string imgui_ini_filename(user_config_path + "imgui.ini");
  //io.IniFilename = imgui_ini_filename.c_str();

  // ImGUI font.
  //const float font_scaling_factor(dpi_handler::get_scale());
  //const float font_size(18.0f * font_scaling_factor);
  //const std::string font_path(
  //  resources::font_path("Manrope.ttf").generic_string());

  //io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_size);
  //io.FontDefault = io.Fonts->AddFontFromFileTTF(font_path.c_str(), font_size);
  dpi_handler::set_global_font_scaling(&io);

  // Setup Platform/Renderer backends.
  ImGui_ImplSDL2_InitForSDLRenderer(window_->get_native_window(),
                                    window_->get_native_renderer());

  ImGui_ImplSDLRenderer2_Init(window_->get_native_renderer());

  running_ = true;
  while (running_)
  {
    SDL_Event event;

    while (SDL_PollEvent(&event) == 1)
    {
      ImGui_ImplSDL2_ProcessEvent(&event);

      if (event.type == SDL_QUIT)
        stop();

      if (event.type == SDL_WINDOWEVENT
          && event.window.windowID
             == SDL_GetWindowID(window_->get_native_window()))
        on_event(event.window);
    }

    // Start the ImGui frame.
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (!minimized_)
    {
      if (ImGui::BeginMainMenuBar())
      {
        if (ImGui::BeginMenu("File"))
        {
          if (ImGui::MenuItem("Exit", "Alt+Q"))
            stop();

          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
          ImGui::MenuItem("Main", nullptr, &show_main_panel_);
          if (settings_.demo)
            ImGui::MenuItem("ImGui Demo Panel", nullptr, &show_demo_panel_);
          ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
      }

      menu_height_ = ImGui::GetFrameHeight();

      // Whatever GUI to implement here ...
      if (show_main_panel_)
        render_main(*this, &show_main_panel_);

      // ImGUI demo panel.
      if (show_demo_panel_)
        ImGui::ShowDemoWindow(&show_demo_panel_);
    }

    // Render and present.
    ImGui::Render();

    SDL_SetRenderDrawColor(window_->get_native_renderer(), 100, 100, 100,
                           SDL_ALPHA_OPAQUE);
    SDL_RenderClear(window_->get_native_renderer());

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(),
                                          window_->get_native_renderer());

    SDL_RenderPresent(window_->get_native_renderer());
  }
}

SDL_Rect program::free_area() const
{
  int w, h;
  SDL_GetRendererOutputSize(window_->get_native_renderer(), &w, &h);

  return {0, menu_height_, w, h - menu_height_};
}

void program::stop()
{
  running_ = false;
}

void program::on_event(const SDL_WindowEvent &event)
{
  switch (event.event)
  {
  case SDL_WINDOWEVENT_CLOSE:
    return on_close();
  case SDL_WINDOWEVENT_MINIMIZED:
    return on_minimize();
  case SDL_WINDOWEVENT_RESIZED:
    return;
  case SDL_WINDOWEVENT_SHOWN:
    return on_shown();
  default:
    // Do nothing otherwise
    return;
  }
}

void program::on_minimize()
{
  minimized_ = true;
}

void program::on_shown()
{
  minimized_ = false;
}

void program::on_close()
{
  stop();
}

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

float dpi_handler::get_scale()
{
  const int display_index(0);
  const float default_dpi(96.0f);
  float dpi(default_dpi);

  SDL_GetDisplayDPI(display_index, nullptr, &dpi, nullptr);

  return dpi / default_dpi;
}

window_size dpi_handler::get_dpi_aware_window_size(
  const window::settings &settings)
{
  const float scale(dpi_handler::get_scale());
  const int width(static_cast<int>(static_cast<float>(settings.width)*scale));
  const int height(static_cast<int>(static_cast<float>(settings.height)*scale));

  return {width, height};
}

void dpi_handler::set_render_scale(SDL_Renderer *)
{
  // do nothing
}

void dpi_Handler::set_global_font_scaling(ImGuiIO *)
{
  // do nothing
}

std::filesystem::path resources::resource_path(
  const std::filesystem::path &file_path)
{
  std::filesystem::path font_path(BASE_PATH);
  font_path /= "../share" / file_path;
  return font_path;
}

std::filesystem::path resources::font_path(const std::string_view &font_file)
{
  return resource_path("fonts") / font_file;
}

#elif defined(__APPLE__)

float dpi_handler::get_scale()
{
  const int display_index(0);
  const float default_dpi(96.0f);
  float dpi(default_dpi);

  SDL_GetDisplayDPI(display_index, nullptr, &dpi, nullptr);

  return std::floor(dpi / default_dpi);
}

window_size dpi_andler::get_dpi_aware_window_size(
  const window::settings &settings)
{
  return {settings.width, settings.height};
}

void dpi_handler::set_render_scale(SDL_Renderer *renderer)
{
  auto scale{get_scale()};
  SDL_RenderSetScale(renderer, scale, scale);
}

void dpi_handler::set_global_font_scaling(ImGuiIO *io)
{
  io->FontGlobalScale = 1.0f / get_scale();
}

std::filesystem::path resources::resource_path(
  const std::filesystem::path &file_path)
{
  std::filesystem::path font_path{BASE_PATH};
  font_path /= file_path;
  return font_path;
}

std::filesystem::path resources::font_path(const std::string_view &font_file)
{
  return resource_path(font_file);
}

#else

float dpi_handler::get_scale()
{
  const int display_index(0);
  const float default_dpi(96.0f);
  float dpi(default_dpi);

  SDL_GetDisplayDPI(display_index, nullptr, &dpi, nullptr);

  return dpi / default_dpi;
}

window_size dpi_handler::get_dpi_aware_window_size(
  const window::settings &settings)
{
  return {settings.width, settings.height};
}

void dpi_handler::set_render_scale(SDL_Renderer *)
{
  // do nothing
}

void dpi_handler::set_global_font_scaling(ImGuiIO *)
{
  // do nothing
}

std::filesystem::path resources::resource_path(
  const std::filesystem::path &file_path)
{
  std::filesystem::path font_path{BASE_PATH};
  font_path /= "../share";
  font_path /= "fonts" / file_path;
  return font_path;
}

std::filesystem::path resources::font_path(const std::string_view &font_file)
{
  return resource_path(font_file);
}

#endif

}  // namespace imgui_app
