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

#if !defined(ULTRA_STRING_PRIMITIVE_H)
#define      ULTRA_STRING_PRIMITIVE_H

#include <cstdlib>
#include <string>

#include "kernel/terminal.h"
#include "kernel/gp/function.h"

namespace ultra::str
{

class literal : public terminal
{
public:
  explicit literal(const std::string &s,
                   category_t c = symbol::default_category)
    : terminal(s, c)
  {
    Expects(!s.empty());
  }

  [[nodiscard]] value_t instance() const final
  {
    return name();
  }
};

///
/// String comparison for equality.
///
class ife : public function
{
public:
  explicit ife(return_type r, const param_data_types &pt)
    : function("SIFE", r, pt)
  {
    Expects(pt.size() == 2);
    Expects(r != pt[0]);
    Expects(pt[0] == pt[1]);
  }

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case python_format:  return "({2} if {0} == {1} else {3})";
    default:             return "{0}=={1}";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto v0(pars[0]);
    if (!has_value(v0))  return v0;

    const auto v1(pars[1]);
    if (!has_value(v1))  return v1;

    return v0 == v1 ? pars[2] : pars[3];
  }
};

}  // namespace ultra::str

#endif  // include guard
