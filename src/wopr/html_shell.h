/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_WOPR_HTML_SHELL_H)
#define      ULTRA_WOPR_HTML_SHELL_H

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ultra::wopr
{

/// Escapes a string for safe embedding inside a JSON literal inside HTML.
[[nodiscard]] std::string json_escape(std::string_view);

/// Appends a JSON-encoded string literal to `out`.
void append_json_string(std::string &, std::string_view);

/// Appends a JSON number (`null` when non-finite) to `out`.
void append_json_number(std::string &, double);

inline void append_json(std::string &out, std::string_view value)
{
  append_json_string(out, value);
}

inline void append_json(std::string &out, const char *value)
{
  append_json_string(out, std::string_view(value));
}

inline void append_json(std::string &out, bool value)
{
  out += value ? "true" : "false";
}

inline void append_json(std::string &out, std::nullptr_t)
{
  out += "null";
}

inline void append_json(std::string &out, double value)
{
  append_json_number(out, value);
}

template<std::integral T>
requires (!std::same_as<T, bool>)
void append_json(std::string &out, T value)
{
  out += std::to_string(value);
}

/// Appends a JSON array from `values` using `append_fn(out, element)`.
template<typename T, typename AppendFn>
void append_json_array(std::string &out, const std::vector<T> &values,
                       AppendFn append_fn)
{
  out.push_back('[');
  for (std::size_t i(0); i < values.size(); ++i)
  {
    if (i)
      out.push_back(',');
    append_fn(out, static_cast<const T &>(values[i]));
  }
  out.push_back(']');
}

template<typename T>
void append_json(std::string &out, const std::vector<T> &values)
{
  append_json_array(out, values,
                    [](std::string &s, const T &v) { append_json(s, v); });
}

/// Appends a JSON object key-value pair to `out`.
template<typename T>
void append_json_field(std::string &out, std::string_view key, T &&value,
                       bool comma = true)
{
  if (comma)
    out.push_back(',');
  append_json_string(out, key);
  out.push_back(':');
  if constexpr (std::invocable<T &, std::string &>)
    std::forward<T>(value)(out);
  else
    append_json(out, value);
}

/// Wraps page content, embedding ECharts unless a companion script is supplied.
[[nodiscard]] std::string build_echarts_html(std::string_view,
                                             std::string_view,
                                             std::string_view,
                                             std::string_view,
                                             std::string_view script_source = {});

/// Writes a page, using a per-report companion ECharts file during live
/// updates and removing it once the final self-contained HTML is written.
[[nodiscard]] bool write_echarts_html(const std::filesystem::path &,
                                     std::string_view title,
                                     std::string_view json_payload,
                                     std::string_view body_html,
                                     std::string_view app_js, bool live);

/// Atomically writes content to disk via a sibling temporary file.
[[nodiscard]] bool write_file_atomically(const std::filesystem::path &,
                                         std::string_view);

}  // namespace ultra::wopr

#endif  // include guard
