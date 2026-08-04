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

#include "imgui_app.h"

#include "imgui/backends/imgui_impl_sdl3.h"
#include "imgui/backends/imgui_impl_sdlrenderer3.h"

#include "implot/implot.h"

#include <stdexcept>

namespace imgui_app
{

window::window(const window::settings &s)
{
  window_ = SDL_CreateWindow(s.title.c_str(), s.width, s.height, s.flags);

  renderer_ = SDL_CreateRenderer(window_, nullptr);
  if (!renderer_)
    throw std::runtime_error("Error creating SDL_Renderer");
}

window::~window()
{
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
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
  const unsigned flags(SDL_INIT_VIDEO);

  if (!SDL_Init(flags))
    throw std::runtime_error(SDL_GetError());

  window_ = std::make_unique<window>(settings_.w_related);
}

program::~program()
{
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  SDL_Quit();
}

void program::run(std::function<void (const program &, bool *)> render_main)
{
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO &io(ImGui::GetIO());

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // Setup Platform/Renderer backends.
  ImGui_ImplSDL3_InitForSDLRenderer(window_->get_native_window(),
                                    window_->get_native_renderer());

  ImGui_ImplSDLRenderer3_Init(window_->get_native_renderer());

  running_ = true;
  while (running_)
  {
    SDL_Event event;

    while (SDL_PollEvent(&event) == 1)
    {
      ImGui_ImplSDL3_ProcessEvent(&event);

      if (event.type == SDL_EVENT_QUIT)
        stop();

      if (event.window.windowID
          == SDL_GetWindowID(window_->get_native_window()))
        on_event(event.window);
    }

    // Start the ImGui frame.
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

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

    // Implement your GUI here...
    if (show_main_panel_)
      render_main(*this, &show_main_panel_);

    // ImGUI demo panel.
    if (show_demo_panel_)
      ImGui::ShowDemoWindow(&show_demo_panel_);

    // ---- END FRAME ----

    // Render and present.
    ImGui::Render();

    SDL_SetRenderDrawColor(window_->get_native_renderer(), 100, 100, 100,
                           SDL_ALPHA_OPAQUE);
    SDL_RenderClear(window_->get_native_renderer());

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(),
                                          window_->get_native_renderer());

    SDL_RenderPresent(window_->get_native_renderer());
  }
}

SDL_Rect program::free_area() const
{
  int w, h;
  SDL_GetCurrentRenderOutputSize(window_->get_native_renderer(), &w, &h);

  return {0, menu_height_, w, h - menu_height_};
}

void program::stop()
{
  running_ = false;
}

void program::on_event(const SDL_WindowEvent &event)
{
  switch (event.type)
  {
  case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    return on_close();
  case SDL_EVENT_WINDOW_RESIZED:
    return;
  default:
    // Do nothing otherwise
    return;
  }
}

void program::on_close()
{
  stop();
}

}  // namespace imgui_app
