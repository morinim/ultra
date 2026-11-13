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

program::program(const settings &s) : settings_(s)
{
  if (!SDL_Init(SDL_INIT_VIDEO))
    throw std::runtime_error(SDL_GetError());

  window_ = SDL_CreateWindow(settings_.title.c_str(), settings_.width,
                             settings_.height, settings_.flags);
  if (!window_)
    throw std::runtime_error("Error creating SDL_Window");

  renderer_ = SDL_CreateRenderer(window_, nullptr);
  if (!renderer_)
    throw std::runtime_error("Error creating SDL_Renderer");
}

program::~program()
{
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  if (renderer_)
    SDL_DestroyRenderer(renderer_);
  if (window_)
    SDL_DestroyWindow(window_);

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
  ImGui_ImplSDL3_InitForSDLRenderer(window_, renderer_);
  ImGui_ImplSDLRenderer3_Init(renderer_);

  running_ = true;
  while (running_)
  {
    SDL_Event event;

    while (SDL_PollEvent(&event) == 1)
    {
      ImGui_ImplSDL3_ProcessEvent(&event);

      if (event.type == SDL_EVENT_QUIT
          || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED
              && event.window.windowID == SDL_GetWindowID(window_)))
        stop();
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

    SDL_SetRenderDrawColor(renderer_, 100, 100, 100, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer_);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);

    SDL_RenderPresent(renderer_);
  }
}

SDL_Rect program::free_area() const
{
  int w, h;
  SDL_GetCurrentRenderOutputSize(renderer_, &w, &h);

  return {0, menu_height_, w, h - menu_height_};
}

void program::stop()
{
  running_ = false;
}

}  // namespace imgui_app
