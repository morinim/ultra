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

#if !defined(ULTRA_TERMINAL_H)
#define      ULTRA_TERMINAL_H

#include "kernel/symbol.h"
#include "kernel/value.h"

namespace ultra
{
///
/// A terminal might be a variable (input to the program), a constant value
/// or a function taking no arguments (e.g. `move-north()`).
///
class terminal : public symbol
{
public:
  template<class V> terminal(const std::string &, V, category_t = 0);

  const value_t &value() const;

  [[nodiscard]] bool nullary() const;

  [[nodiscard]] bool operator==(const terminal &) const;

  [[nodiscard]] virtual std::string display(format = c_format) const;
  [[nodiscard]] virtual terminal random() const;

protected:
  value_t data_;
};

[[nodiscard]] bool operator!=(const terminal &, const terminal &);

///
/// \param[in] name name of the terminal
/// \param[in] v    value of the terminal
/// \param[in] c    category of the terminal
///
template<class V> terminal::terminal(const std::string &name, V v,
                                     category_t c)
  : symbol(name, c), data_(v)
{
}

///
/// \return `true` for a nullary terminal
///
inline bool terminal::nullary() const
{
  return std::holds_alternative<D_NULLARY>(data_);
}

}  // namespace ultra

#endif  // include guard
