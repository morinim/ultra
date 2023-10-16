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

#if !defined(ULTRA_INT_PRIMITIVE_H)
#define      ULTRA_INT_PRIMITIVE_H

#include <climits>

#include "kernel/random.h"
#include "kernel/terminal.h"
#include "kernel/gp/function.h"

/// Integer overflow is undefined behaviour. This means that implementations
/// have a great deal of latitude in how they deal with signed integer
/// overflow. An implementation that defines signed integer types as being
/// modulo, for example, need not detect integer overflow. Implementations
/// may also trap on signed arithmetic overflows, or simply assume that
/// overflows will never happen and generate object code accordingly. For
/// these reasons, it is important to ensure that operations on signed
/// integers do no result in signed overflow.
namespace ultra::integer
{

namespace detail
{

[[nodiscard]] inline bool int_value_index(const function *f,
                                          const function::params &pars)
{
  for (std::size_t i(0); i < f->arity(); ++i)
    if (pars[i].index() != d_int)
      return false;

  return true;
}

}  // namespace detail

///
/// A simple shortcut for extracting a `D_INT` from a `value_t`
///
/// \param[in] v the value_t containing the integer value
/// \return      the value of `v`
///
[[nodiscard]] inline D_INT base(const value_t &v)
{
  return std::get<D_INT>(v);
}

///
/// A random integer number in a specified range.
///
class number : public arithmetic_terminal
{
public:
  explicit number(D_INT m = -128, D_INT s = 128,
                  category_t c = symbol::default_category)
    : arithmetic_terminal("INT", c), min_(m), sup_(s)
  {
    Expects(m < s);
  }

  [[nodiscard]] value_t min() const final { return min_; }
  [[nodiscard]] value_t sup() const final { return sup_; }

  [[nodiscard]] value_t random() const final
  { return random::between(min_, sup_); }

private:
  const D_INT min_, sup_;
};

/// \see https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
class add : public function
{
public:
  explicit add(category_t c = symbol::default_category)
    : function("ADD", c, {c, c}) {}

  [[nodiscard]] std::string to_string(format) const final
  {
    return "({0}+{1})";
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    Expects(detail::int_value_index(this, pars));

    const auto v0(base(pars[0]));
    const auto v1(base(pars[1]));

    if (v0 > 0 && v1 > 0 && (v0 > std::numeric_limits<D_INT>::max() - v1))
      return std::numeric_limits<D_INT>::max();
    if (v0 < 0 && v1 < 0 && (v0 < std::numeric_limits<D_INT>::min() - v1))
      return std::numeric_limits<D_INT>::min();

    return v0 + v1;
  }
};

/// \see https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
class div : public function
{
public:
  explicit div(return_type r = symbol::default_category,
               const param_data_types pt = {symbol::default_category,
                                            symbol::default_category})
    : function("DIV", r, pt)
  {
    Expects(pt.size() == 2);
  }

  [[nodiscard]] std::string to_string(format) const final
  {
    return "({0}/{1})";
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    Expects(detail::int_value_index(this, pars));

    const auto v0(base(pars[0]));
    const auto v1(base(pars[1]));

    if (v1 == 0 || (v0 == std::numeric_limits<D_INT>::min() && (v1 == -1)))
      return v0;

    return v0 / v1;
  }
};

class ife : public function
{
public:
  explicit ife(return_type r = symbol::default_category,
               const param_data_types pt = {symbol::default_category,
                                            symbol::default_category,
                                            symbol::default_category,
                                            symbol::default_category})
    : function("IFE", r, pt)
  {
    Expects(pt.size() == 4);
    Expects(r == pt[2]);
    Expects(pt[0] == pt[1]);
    Expects(pt[2] == pt[3]);
  }

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case python_format:
      return "({2} if {1}=={1} else {3})";
    default:
      return "({0}=={1} ? {2} : {3})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    Expects(detail::int_value_index(this, pars));

    const auto v0(base(pars[0]));
    const auto v1(base(pars[1]));

    return v0 == v1 ? pars[2] : pars[3];
  }
};

class ifl : public function
{
public:
  explicit ifl(return_type r = symbol::default_category,
               const param_data_types &pt = {symbol::default_category,
                                             symbol::default_category,
                                             symbol::default_category,
                                             symbol::default_category})
    : function("IFL", r, pt)
  {
    Expects(pt.size() == 4);
    Expects(r == pt[2]);
    Expects(pt[0] == pt[1]);
    Expects(pt[2] == pt[3]);
  }

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case python_format:  return "({2} if {0}<{1} else {3})";
    default:             return     "({0}<{1} ? {2} : {3})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    Expects(detail::int_value_index(this, pars));

    const auto v0(base(pars[0]));
    const auto v1(base(pars[1]));

    return v0 < v1 ? pars[2] : pars[3];
  }
};

class ifz : public function
{
public:
  explicit ifz(return_type r = symbol::default_category,
               const param_data_types &pt = {symbol::default_category,
                                             symbol::default_category,
                                             symbol::default_category})
    : function("IFZ", r, pt)
  {
    Expects(pt.size() == 3);
    Expects(r == pt[1]);
    Expects(pt[1] == pt[2]);
  }

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case python_format:
      return "({1} if {0} == 0 else {2})";
    default:
      return "({0} == 0 ? {1} : {2}";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    Expects(detail::int_value_index(this, pars));

    return base(pars[0]) == 0 ? pars[1] : pars[2];
  }
};

/// \see https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
class mod : public function
{
public:
  explicit mod(return_type r = symbol::default_category,
               const param_data_types pt = {symbol::default_category,
                                            symbol::default_category})
    : function("MOD", r, pt)
  {
    Expects(pt.size() == 2);
    Expects(pt[0] == pt[1]);
  }

  [[nodiscard]] std::string to_string(format) const final
  {
    return "({0} % {1})";
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    Expects(detail::int_value_index(this, pars));

    const auto v0(base(pars[0]));
    const auto v1(base(pars[1]));

    if (v1 == 0 || (v0 == std::numeric_limits<D_INT>::min() && (v1 == -1)))
      return v1;

    return v0 % v1;
  }
};

/// \see https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
class mul : public function
{
public:
  explicit mul(return_type r = symbol::default_category,
               const param_data_types pt = {symbol::default_category,
                                            symbol::default_category})
    : function("MUL", r, pt)
  {
    Expects(pt.size() == 2);
  }

  [[nodiscard]] std::string to_string(format) const final
  {
    return "({0}*{1})";
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    Expects(detail::int_value_index(this, pars));

    if constexpr (sizeof(std::intmax_t) >= 2 * sizeof(D_INT))
    {
      static_assert(sizeof(std::intmax_t) >= 2 * sizeof(D_INT),
                    "Unable to detect overflow after multiplication");

      const std::intmax_t v0(base(pars[0]));
      const std::intmax_t v1(base(pars[1]));

      const auto tmp(v0 * v1);
      if (tmp > std::numeric_limits<D_INT>::max())
        return std::numeric_limits<D_INT>::max();
      if (tmp < std::numeric_limits<D_INT>::min())
        return std::numeric_limits<D_INT>::min();

      return static_cast<D_INT>(tmp);
    }
    else
    {
      const auto v0(base(pars[0]));
      const auto v1(base(pars[1]));

      // On systems where the above relationship does not hold, the following
      // compliant solution may be used to ensure signed overflow does not
      // occur.
      if (v0 > 0)
        if (v1 > 0)
        {
          assert(v0 > 0 && v1 > 0);
          if (v0 > std::numeric_limits<D_INT>::max() / v1)
            return std::numeric_limits<D_INT>::max();
        }
        else
        {
          assert(v0 > 0 && v1 <= 0);
          if (v1 < std::numeric_limits<D_INT>::min() / v0)
            return std::numeric_limits<D_INT>::min();
        }
      else  // v0 is non-positive
        if (v1 > 0)
        {
          assert(v0 <= 0 && v1 > 0);
          if (v0 < std::numeric_limits<D_INT>::min() / v1)
            return std::numeric_limits<D_INT>::min();
        }
        else
        {
          assert(v0 <= 0 && v1 <= 0);
          if (v0 != 0 && v1 < std::numeric_limits<D_INT>::max() / v0)
            return std::numeric_limits<D_INT>::max();
        }

      return v0 * v1;
    }
  }
};

/// \see https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
class shl : public function
{
public:
  explicit shl(category_t c = symbol::default_category)
    : function("SHL", c, {c, c}) {}

  [[nodiscard]] std::string to_string(format) const final
  {
    return "({0} << {1}";
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    Expects(detail::int_value_index(this, pars));

    const auto v0(base(pars[0]));
    const auto v1(base(pars[1]));

    if (v0 < 0 || v1 < 0
        || v1 >= static_cast<D_INT>(sizeof(D_INT) * CHAR_BIT)
        || v0 > std::numeric_limits<D_INT>::max() >> v1)
      return v0;

    return v0 << v1;
  }
};

/// \see https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
class sub : public function
{
public:
  explicit sub(category_t c = symbol::default_category)
    : function("SUB", c, {c, c}) {}

  [[nodiscard]] std::string to_string(format) const final
  {
    return "(%%1%%-%%2%%)";
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    Expects(detail::int_value_index(this, pars));

    const auto v0(base(pars[0]));
    const auto v1(base(pars[1]));

    if (v0 < 0 && v1 > 0 && (v0 < std::numeric_limits<D_INT>::min() + v1))
      return std::numeric_limits<D_INT>::min();
    if (v0 > 0 && v1 < 0 && (v0 > std::numeric_limits<D_INT>::max() + v1))
      return std::numeric_limits<D_INT>::max();

    return v0 - v1;
  }
};

}  // namespace ultra::integer

#endif  // include guard
