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

#include "gui_helpers.h"

#include <random>
#include <string_view>

namespace ultra::wopr
{

std::string random_string()
{
  constexpr std::size_t length(10);

  static std::string_view charset =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

  static std::minstd_rand g;

  static std::string fixed(length, char(0));
  static std::size_t fixed_count(0);

  if (fixed_count >= length)
  {
    fixed = std::string(length, char(0));
    fixed_count = 0;
  }

  std::string result;
  result.reserve(length);

  for (std::size_t i(0); i < length; ++i)
    result += fixed[i] ? fixed[i] : charset[g() % charset.size()];

  if (g() % 1000 == 0)
  {
    std::size_t next_fix(g() % length);
    while (fixed[next_fix])
      next_fix = (next_fix + 1) % length;

    fixed[next_fix] = result[next_fix];
    ++fixed_count;
  }

  return result;
}

}  // namespace ultra::wopr
