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

#include "monitor.h"

#include "html_monitor.h"
#include "monitor_data.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

namespace fs = std::filesystem;

namespace ultra::wopr
{

namespace
{

std::atomic<std::sig_atomic_t> g_stop_requested {0};

extern "C" void handle_stop_signal(int)
{
  g_stop_requested.store(1, std::memory_order_relaxed);
}

class signal_guard
{
public:
  signal_guard()
    : prev_int_(std::signal(SIGINT, handle_stop_signal)),
      prev_term_(std::signal(SIGTERM, handle_stop_signal))
  {
    g_stop_requested = 0;
  }

  ~signal_guard()
  {
    std::signal(SIGINT, prev_int_);
    std::signal(SIGTERM, prev_term_);
  }

  signal_guard(const signal_guard &) = delete;
  signal_guard &operator=(const signal_guard &) = delete;

private:
  using handler_t = void (*)(int);
  handler_t prev_int_;
  handler_t prev_term_;
};

class read_log_file
{
public:
  explicit read_log_file(const fs::path &);

  [[nodiscard]] bool is_open() const { return file_.is_open(); }
  std::optional<std::string> get_line();

private:
  std::ifstream file_;
  std::streampos position_ {0};
};

read_log_file::read_log_file(const fs::path &filename)
  : file_(filename)
{
}

//
// Reads complete lines from a single log file.
// Takes advantage of the fact that a line is complete if a newline has been
// reached.
//
std::optional<std::string> read_log_file::get_line()
{
  if (file_.is_open())
  {
    // Seek to the last known position.
    file_.clear();
    file_.seekg(position_);

    if (std::string line; std::getline(file_, line) && !file_.eof())
    {
      position_ = file_.tellg();  // update the position for the next read
      return line;
    }

    if (file_.bad())
      throw std::runtime_error("Error occurred while reading the file.");
  }

  return {};
}

[[nodiscard]] std::optional<read_log_file> open_log_file(
  const fs::path &filename)
{
  if (filename.empty())
    return {};

  read_log_file file(filename);
  if (!file.is_open())
  {
    std::cerr << "Stopped monitoring " << filename
              << ": Failed to open file for reading\n";
    return {};
  }

  return file;
}

template<typename DataClass>
[[nodiscard]] bool poll_log_stream(std::optional<read_log_file> &log,
                                   monitor_state &state,
                                   const fs::path &filename)
{
  if (!log)
    return false;

  bool read_any(false);
  try
  {
    while (true)
    {
      const auto line(log->get_line());
      if (!line)
        break;

      state.push(DataClass(*line));
      read_any = true;
    }
  }
  catch (const std::runtime_error &e)
  {
    std::cerr << "Stopped monitoring " << filename << ": " << e.what() << '\n';
    log.reset();
  }

  return read_any;
}

[[nodiscard]] bool poll_log_streams(std::optional<read_log_file> &dynamic_log,
                                    std::optional<read_log_file> &population_log,
                                    std::optional<read_log_file> &layers_log,
                                    const ultra::search_log &slog,
                                    monitor_state &state)
{
  const bool dyn(poll_log_stream<dynamic_data>(dynamic_log, state,
                                               slog.dynamic_file_path));
  const bool pop(poll_log_stream<population_line>(population_log, state,
                                                  slog.population_file_path));
  const bool lay(poll_log_stream<layers_line>(layers_log, state,
                                              slog.layers_file_path));

  return dyn || pop || lay;
}

}  // namespace

bool monitor::start(options options)
{
  if (options.html_output.empty())
  {
    std::cerr << "Missing HTML output path for `monitor`.\n";
    return false;
  }

  monitor_state state;
  auto dynamic_log(open_log_file(options.slog.dynamic_file_path));
  auto population_log(open_log_file(options.slog.population_file_path));
  auto layers_log(open_log_file(options.slog.layers_file_path));

  (void)poll_log_streams(dynamic_log, population_log, layers_log,
                         options.slog, state);

  const bool one_shot(options.interval == 0
                      || (!dynamic_log && !population_log && !layers_log));
  if (one_shot)
    return write_monitor_html(options.html_output, state, options, false);

  const signal_guard signals;
  if (!write_monitor_html(options.html_output, state, options, true))
    return false;

  std::cout << "HTML monitor: " << options.html_output
            << " (refreshing every " << options.interval
            << "s, press Ctrl+C to stop)\n";

  using namespace std::chrono_literals;
  const auto period(std::chrono::seconds(options.interval));

  while (!g_stop_requested.load(std::memory_order_relaxed))
  {
    const auto wake_at(std::chrono::steady_clock::now() + period);
    while (!g_stop_requested.load(std::memory_order_relaxed)
           && std::chrono::steady_clock::now() < wake_at)
      std::this_thread::sleep_for(100ms);

    if (g_stop_requested.load(std::memory_order_relaxed))
      break;

    const bool updated(poll_log_streams(dynamic_log, population_log, layers_log,
                                        options.slog, state));
    const bool streams_open(dynamic_log || population_log || layers_log);

    if (updated || !streams_open)
      if (!write_monitor_html(options.html_output, state, options, streams_open))
        return false;

    if (!streams_open)
      return true;
  }

  (void)poll_log_streams(dynamic_log, population_log, layers_log,
                         options.slog, state);
  return write_monitor_html(options.html_output, state, options, false);
}

}  // namespace ultra::wopr
