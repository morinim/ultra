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
  static constexpr category_t undefined_category =
    std::numeric_limits<category_t>::max();

  /// This is the type used as key for symbol identification.
  using opcode_t = unsigned;

  /// Symbol rendering format.
  enum format {c_format, cpp_format, python_format, sup_format};

  [[nodiscard]] category_t category() const;
  [[nodiscard]] opcode_t opcode() const;
  [[nodiscard]] std::string name() const;

  void category(category_t);

  [[nodiscard]] virtual bool is_valid() const;

protected:
  explicit symbol(const std::string &, category_t = 0);
  virtual ~symbol() = default;

private:
  std::string name_;
  category_t category_;
  opcode_t opcode_;
};

///
/// The type (a.k.a. category) of the symbol.
///
/// \return the category
///
/// In strongly typed GP every terminal and every function argument / return
/// value has a type (a.k.a. category).
/// For GAs / DE category is used to define a valid interval for numeric
/// arguments.
///
inline symbol::category_t symbol::category() const
{
  return category_;
}

///
/// An opcode is a unique, numerical session ID for a symbol.
///
/// \return the opcode
///
/// The opcode is a fast way to uniquely identify a symbol and is primarily
/// used for hashing.
///
/// \remark
/// A symbol can be identified also by its name (a `std::string`). The name
/// is often a better option since it doesn't change among executions.
///
inline symbol::opcode_t symbol::opcode() const
{
  return opcode_;
}

}  // namespace ultra

#endif  // include guard
