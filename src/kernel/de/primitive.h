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

#if !defined(ULTRA_DE_PRIMITIVE_H)
#define      ULTRA_DE_PRIMITIVE_H

#include <string>

#include "kernel/random.h"
#include "kernel/terminal.h"

namespace ultra::de
{

///
/// A real number within a range.
///
/// While many genetic algorithms use integers to approximate continuous
/// parameters, the choice limits the resolution with which an optimum can
/// be located. Floating point not only uses computer resources
/// efficiently, it also makes input and output transparent for the user.
/// Parameters can be input, manipulated and output as ordinary
/// floating-point numbers without ever being reformatted as genes with a
/// different binary representation.
///
class real : public terminal
{
public:
  ///
  /// \param[in] i a half open interval
  /// \param[in] c an optional category
  ///
  explicit real(const interval<double> &i = {-1000.0, 1000.0},
                symbol::category_t c = symbol::undefined_category)
    : terminal("REAL", c), interval_(i)
  {
  }

  [[nodiscard]] constexpr double min() const noexcept { return interval_.min; }
  [[nodiscard]] constexpr double sup() const noexcept { return interval_.sup; }

  [[nodiscard]] value_t instance() const final
  { return random::element(interval_); }

private:
  const interval<double> interval_;
};

}  // namespace ultra::de

#endif  // include guard
