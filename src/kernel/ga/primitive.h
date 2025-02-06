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

#if !defined(ULTRA_GA_PRIMITIVE_H)
#define      ULTRA_GA_PRIMITIVE_H

#include <string>

#include "kernel/random.h"
#include "kernel/terminal.h"

namespace ultra::ga
{

///
/// An integer number within a range.
///
class integer : public terminal
{
public:
  ///
  /// A number (terminal symbol) within a range used for genetics algorithms.
  ///
  /// \param[in] interval a half open interval
  /// \param[in] category an optional category
  ///
  /// This is a base helper class used to build more specific numeric classes.
  /// The general idea follows:
  /// - **the problem can be tackled with a standard, uniform chromosome**
  ///   (every locus contain the same kind of gene). In this case the user
  ///   simply calls the `ga_problem`/`de_problem` constructor specifying the
  ///   length of the chromosome;
  /// - **the problem requires a more complex structure**. The user specifies a
  ///   (possibly) different type for every locus.
  ///
  explicit integer(interval<int> interval = {-1000, 1000},
                   symbol::category_t category = symbol::undefined_category)
    : terminal("INTEGER", category), interval_(interval)
  {
  }

  [[nodiscard]] constexpr int min() const noexcept { return interval_.min; }
  [[nodiscard]] constexpr int sup() const noexcept { return interval_.sup; }

  [[nodiscard]] value_t instance() const final
  { return random::element(interval_); }

private:
  const interval<int> interval_;
};

}  // namespace ultra::ga

#endif  // include guard
