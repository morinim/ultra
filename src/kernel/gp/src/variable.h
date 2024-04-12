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

#if !defined(ULTRA_SRC_VARIABLE_H)
#define      ULTRA_SRC_VARIABLE_H

#include "kernel/terminal.h"
#include "kernel/gp/src/interpreter.h"

namespace ultra::src
{
///
/// Represents an input argument (feature) for a symbolic regression or
/// classification problem.
///
class variable : public terminal
{
public:
  variable(std::size_t var_id, const std::string &name,
           category_t c = symbol::default_category)
    : terminal(name, c), var_(var_id)
  {}

  [[nodiscard]] value_t eval(const ultra::interpreter &i) const
  {
    Expects(dynamic_cast<const src::interpreter *>(&i));
    return static_cast<const src::interpreter &>(i).fetch_var(var_);
  }

  [[nodiscard]] value_t instance() const final { return this; }

  [[nodiscard]] std::string to_string(format = c_format) const
  { return name(); }

private:
  std::size_t var_;
};

}  // namespace ultra

#endif  // include guard
