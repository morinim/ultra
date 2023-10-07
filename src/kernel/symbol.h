/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_SYMBOL_H)
#define      ULTRA_SYMBOL_H

#include <limits>
#include <string>

#include "kernel/value.h"
#include "utility/assert.h"

namespace ultra
{

///
/// Together functions and terminals are referred to as symbols.
///
/// EA assembles program structures from basic units called functions and
/// terminals. Functions perform operations on their inputs, which are either
/// terminals or output from other functions.
///
class symbol
{
public:
  /// A category provide operations which supplement or supersede those of the
  /// domain but which are restricted to values lying in the (sub)domain by
  /// which is parametrized.
  /// For instance the number 4.0 (in the real domain) may be present in two
  /// distinct categories: 2 (e.g. the category "km/h") and 3 (e.g. the
  /// category "kg").
  /// Categories are the way:
  /// - strong typing GP is enforced;
  /// - ranges of GA variables are managed.
  using category_t = unsigned;
  static constexpr category_t default_category =
    std::numeric_limits<category_t>::max();
  static constexpr category_t undefined_category =
    std::numeric_limits<category_t>::max();

  /// This is the type used as key for symbol identification.
  using opcode_t = unsigned;

  /// Symbol rendering format.
  enum format {c_format, cpp_format, python_format, sup_format};

  explicit symbol(const std::string &, category_t = default_category);
  virtual ~symbol() = default;

  [[nodiscard]] category_t category() const;
  [[nodiscard]] opcode_t opcode() const;
  [[nodiscard]] std::string name() const;

  void category(category_t);

  [[nodiscard]] virtual std::string to_string(const value_t &,
                                              format = c_format) const = 0;
  [[nodiscard]] virtual bool is_valid() const;

private:
  std::string name_;
  category_t category_;
  opcode_t opcode_;
};

}  // namespace ultra

#endif  // include guard
