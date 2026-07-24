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

#if !defined(ULTRA_WOPR_COMMAND_LINE_H)
#define      ULTRA_WOPR_COMMAND_LINE_H

#include "monitor.h"
#include "results.h"

#include <expected>
#include <filesystem>
#include <string>
#include <variant>

namespace ultra::wopr
{

struct help_command {};

using command = std::variant<
  help_command,
  monitor::options,
  rs::run::options,
  rs::summary::options>;

void cmdl_usage();
[[nodiscard]] std::filesystem::path build_path(
  std::filesystem::path, std::filesystem::path, const std::string & = {});
[[nodiscard]] std::expected<command, std::string> parse_args(int, char *[]);

}  // namespace ultra::wopr

#endif  // include guard
