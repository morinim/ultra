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

#include "kernel/random.h"
#include "kernel/terminal.h"

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

/*
/// \see https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
class add : public function
{
public:
  explicit add(const cvect &c) : function("ADD", c[0], {c[0], c[0]})
  {
    Expects(c.size() == 1);
  }

  bool associative() const final { return true; }

  value_t eval(symbol_params &args) const final
  {
    const auto v0(integer::cast(args[0]));
    const auto v1(integer::cast(args[1]));

    if (v0 > 0 && v1 > 0 && (v0 > std::numeric_limits<base_t>::max() - v1))
      return std::numeric_limits<base_t>::max();
    if (v0 < 0 && v1 < 0 && (v0 < std::numeric_limits<base_t>::min() - v1))
      return std::numeric_limits<base_t>::min();

    return v0 + v1;
  }
};

/// \see https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
class div : public function
{
public:
  explicit div(const cvect &c) : function("DIV", c[0], {c[0], c[0]})
  { Expects(c.size() == 1); }

  value_t eval(symbol_params &args) const final
  {
    const auto v0(integer::cast(args[0]));
    const auto v1(integer::cast(args[1]));

    if (v1 == 0 || (v0 == std::numeric_limits<base_t>::min() && (v1 == -1)))
      return v0;

    return v0 / v1;
  }
};

class ife : public function
{
public:
  explicit ife(const cvect &c)
    : function("IFE", c[1], {c[0], c[0], c[1], c[1]})
  { Expects(c.size() == 2); }

  value_t eval(symbol_params &args) const final
  {
    const auto v0(integer::cast(args[0]));
    const auto v1(integer::cast(args[1]));

    if (v0 == v1)
      return args[2];

    return args[3];
  }

  double penalty_nvi(core_interpreter *ci) const final
  {
    return comparison_function_penalty(ci);
  }
};

class ifl : public function
{
public:
  explicit ifl(const cvect &c)
    : function("IFL", c[1], {c[0], c[0], c[1], c[1]})
  { Expects(c.size() == 2); }

  value_t eval(symbol_params &args) const final
  {
    const auto v0(integer::cast(args[0]));
    const auto v1(integer::cast(args[1]));

    if (v0 < v1)
      return args[2];

    return args[3];
  }

  double penalty_nvi(core_interpreter *ci) const final
  {
    return comparison_function_penalty(ci);
  }
};

class ifz : public function
{
public:
  explicit ifz(const cvect &c) : function("IFZ", c[0], {c[0], c[0], c[0]})
  { Expects(c.size() == 1); }

  value_t eval(symbol_params &args) const final
  {
    const auto v0(integer::cast(args[0]));

    if (v0 == 0)
      return args[1];

    return args[2];
  }

  double penalty_nvi(core_interpreter *ci) const final
  {
    return comparison_function_penalty(ci);
  }
};

/// \see https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
class mod : public function
{
public:
  explicit mod(const cvect &c) : function("MOD", c[0], {c[0], c[0]})
  { Expects(c.size() == 1); }

  value_t eval(symbol_params &args) const final
  {
    const auto v0(integer::cast(args[0]));
    const auto v1(integer::cast(args[1]));

    if (v1 == 0 || (v0 == std::numeric_limits<base_t>::min() && (v1 == -1)))
      return v1;

    return v0 % v1;
  }
};

/// \see https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
class mul : public function
{
public:
  explicit mul(const cvect &c) : function("MUL", c[0], {c[0], c[0]})
  { Expects(c.size() == 1); }

  bool associative() const final { return true; }

  value_t eval(symbol_params &args) const final
  {
    static_assert(sizeof(std::intmax_t) >= 2 * sizeof(base_t),
                  "Unable to detect overflow after multiplication");

    const std::intmax_t v0(integer::cast(args[0]));
    const std::intmax_t v1(integer::cast(args[1]));

    const auto tmp(v0 * v1);
    if (tmp > std::numeric_limits<base_t>::max())
      return std::numeric_limits<base_t>::max();
    if (tmp < std::numeric_limits<base_t>::min())
      return std::numeric_limits<base_t>::min();

    return static_cast<base_t>(tmp);

#ifdef DUMMY
    // On systems where the above relationship does not hold, the following
    // compliant solution may be used to ensure signed overflow does not
    // occur.
    if (v0 > 0)
      if (v1 > 0)
      {
        assert(v0 > 0 && v1 > 0);
        if (v0 > std::numeric_limits<base_t>::max() / v1)
        return std::numeric_limits<base_t>::max();
      }
      else  // v0 is positive, v1 is non-positive
      {
        assert(v0 > 0 && v1 <= 0);
        if (v1 < std::numeric_limits<base_t>::min() / v0)
          return std::numeric_limits<base_t>::min();
      }
    else  // v0 is non-positive
      if (v1 > 0)
      {
        assert(v0 <= 0 && v1 > 0);
        if (v0 < std::numeric_limits<base_t>::min() / v1)
          return std::numeric_limits<base_t>::min();
      }
      else  // v0 is non-positive, v1 is non-positive
      {
        assert(v0 <= 0 && v1 <= 0);
        if (v0 != 0 && v1 < std::numeric_limits<base_t>::max() / v0)
          return std::numeric_limits<base_t>::max();
      }

    return v0 * v1;
#endif
  }
};

/// \see https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
class shl : public function
{
public:
  explicit shl(const cvect &c) : function("SHL", c[0], {c[0], c[0]})
  { Expects(c.size() == 1); }

  value_t eval(symbol_params &args) const final
  {
    const auto v0(integer::cast(args[0]));
    const auto v1(integer::cast(args[1]));

    if (v0 < 0 || v1 < 0 ||
        v1 >= static_cast<base_t>(sizeof(base_t) * CHAR_BIT) ||
        v0 > std::numeric_limits<base_t>::max() >> v1)
      return v0;

    return v0 << v1;
  }
};

/// \see https://wiki.sei.cmu.edu/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
class sub : public function
{
public:
  explicit sub(const cvect &c) : function("SUB", c[0], {c[0], c[0]})
  { Expects(c.size() == 1); }

  value_t eval(symbol_params &args) const final
  {
    const auto v0(integer::cast(args[0]));
    const auto v1(integer::cast(args[1]));

    if (v0 < 0 && v1 > 0 && (v0 < std::numeric_limits<base_t>::min() + v1))
      return std::numeric_limits<base_t>::min();
    if (v0 > 0 && v1 < 0 && (v0 > std::numeric_limits<base_t>::max() + v1))
      return std::numeric_limits<base_t>::max();

    return v0 - v1;
  }
};
*/

}  // namespace ultra::integer

#endif  // include guard
