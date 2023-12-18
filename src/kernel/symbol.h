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
  static constexpr category_t default_category = 0;
  static constexpr category_t undefined_category =
    std::numeric_limits<category_t>::max();

  /// This is the type used as key for symbol identification.
  using opcode_t = unsigned;

  /// Symbol rendering format.
  enum format {c_format, cpp_format, python_format, sup_format};

  explicit symbol(const std::string &, category_t = default_category);
  virtual ~symbol() = default;

  void category(category_t);

  [[nodiscard]] category_t category() const;
  [[nodiscard]] opcode_t opcode() const;
  [[nodiscard]] std::string name() const;

  [[nodiscard]] virtual bool is_valid() const;

private:
  std::string name_;
  category_t category_;
  opcode_t opcode_;
};

template<class S> concept Symbol = std::derived_from<S, symbol>;

///
/// \return a pointer to the `const S *` value stored in the symbol pointed to
///         by `s`. Otherwise, returns a null pointer value.
///
template<Symbol S>
[[nodiscard]] constexpr auto get_if(const symbol *s)
{
  return dynamic_cast<
    const std::remove_cvref_t<std::remove_pointer_t<S>> *>(s);
}

///
/// \return a pointer to the `const S *` value stored in the symbol referenced
///         by `s`. Otherwise, returns a null pointer value.
///
template<Symbol S>
[[nodiscard]] constexpr auto get_if(const symbol &s)
{
  return get_if<S>(&s);
}

///
/// \return a pointer to the `const S *` value stored in the symbol pointed to
///         by `s`. Otherwise, returns a null pointer value.
///
template<Symbol S>
[[nodiscard]] constexpr bool is(const symbol *s)
{
  return get_if<S>(s);
}

///
/// \return a pointer to the `const S *` value stored in the symbol pointed to
///         by `s`. Otherwise, returns a null pointer value.
///
template<Symbol S>
[[nodiscard]] constexpr bool is(const symbol &s)
{
  return is<S>(&s);
}

}  // namespace ultra

#endif  // include guard
