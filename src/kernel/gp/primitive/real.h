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

#if !defined(ULTRA_REAL_PRIMITIVE_H)
#define      ULTRA_REAL_PRIMITIVE_H

#include <string>

#include "kernel/nullary.h"
#include "kernel/random.h"
#include "kernel/gp/function.h"
#include "kernel/terminal.h"
#include "utility/misc.h"

namespace ultra::real
{

///
/// A simple shortcut for extracting a `D_DOUBLE` from a `value_t`
///
/// \param[in] v the value_t containing the real value
/// \return      the value of `v`
///
[[nodiscard]] inline D_DOUBLE base(const value_t &v)
{
  return std::get<D_DOUBLE>(v);
}

// We assume that errors during floating-point operations aren't terminal
// errors. So we dont't try to prevent domain errors (e.g. square root of a
// negative number) or range error (e.g. `pow(10.0, 1e6)`) checking arguments
// beforehand (domain errors could be prevented by carefully bounds checking
// the arguments before calling functions and taking alternative action if
// the bounds are violated; range errors usually cannot be prevented, as they
// are dependent on the implementation of floating-point numbers, as well as
// the function being applied).
// Instead we detect them and take alternative action (usually returning an
// empty value).
static_assert(std::numeric_limits<D_DOUBLE>::is_iec559,
              "Ultra requires IEC 559/IEEE 754 floating-point types");

///
/// A random floating point number in a specified range.
///
class number : public terminal
{
public:
  explicit number(D_DOUBLE m = -1000.0, D_DOUBLE s = 1000.0,
                  category_t c = symbol::default_category)
    : terminal("REAL", c), min_(m), sup_(s)
  {
    Expects(m < s);
  }

  [[nodiscard]] D_DOUBLE min() const noexcept { return min_; }
  [[nodiscard]] D_DOUBLE sup() const noexcept { return sup_; }

  [[nodiscard]] value_t instance() const final
  { return random::between(min_, sup_); }

private:
  const D_DOUBLE min_, sup_;
};

///
/// This is like real::real but restricted to integer numbers.
///
class integer : public terminal
{
public:
  explicit integer(D_INT m = -128, D_INT s = 128,
                   category_t c = symbol::default_category)
    : terminal("IREAL", c), min_(m), sup_(s)
  {
    Expects(m < s);
  }

  [[nodiscard]] D_DOUBLE min() const noexcept
  {return static_cast<D_DOUBLE>(min_);}
  [[nodiscard]] D_DOUBLE sup() const noexcept
  {return static_cast<D_DOUBLE>(sup_);}

  [[nodiscard]] value_t instance() const final
  { return static_cast<D_DOUBLE>(random::between(min_, sup_)); }

private:
  const D_INT min_, sup_;
};

class literal : public terminal
{
public:
  explicit literal(D_DOUBLE v, category_t c = symbol::default_category)
    : terminal(std::to_string(v), c), val_(v)
  {
  }

  [[nodiscard]] value_t instance() const noexcept final { return val_; }

private:
  const D_DOUBLE val_;
};

///
/// The absolute value of a real number.
///
class abs : public function
{
public:
  explicit abs(category_t c = symbol::default_category)
    : function("FABS", c, {c}) {}

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:     return "std::abs({0})";
    case python_format:  return      "abs({0})";
    default:             return     "fabs({0})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p(pars[0]);
    if (!has_value(p))  return p;

    assert(std::isfinite(base(p)));
    return std::fabs(base(p));
  }
};

///
/// Sum of two real numbers.
///
class add : public function
{
public:
  explicit add(category_t c = symbol::default_category)
    : function("FADD", c, {c, c}) {}

  [[nodiscard]] std::string to_string(format) const final
  {
    return "({0}+{1})";
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto p1(pars[1]);
    if (!has_value(p1))  return p1;

    const auto ret(base(p0) + base(p1));
    if (!std::isfinite(ret))  return {};

    return ret;
  }
};

///
/// Analytic quotient (AQ).
///
/// Analytic quotient (AQ) operator systematically yields lower mean squared
/// errors over a range of regression tasks, due principally to removing the
/// discontinuities or singularities that can often result from using either
/// protected or unprotected division. Further, the AQ operator is
/// differentiable.
///
class aq : public function
{
public:
  explicit aq(return_type r = symbol::default_category,
              const param_data_types &pt = {symbol::default_category,
                                            symbol::default_category})
    : function("AQ", r, pt)
  {
    Expects(pt.size() == 2);
  }

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:     return "({0}/std::sqrt(1.0+std::pow({1},2.0)))";
    default:             return           "({0}/sqrt(1.0+pow({1},2.0)))";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto p1(pars[1]);
    if (!has_value(p1))  return p1;

    const auto x(base(p0)), y(base(p1));
    const auto ret(x / std::sqrt(1.0 + y * y));
    if (!std::isfinite(ret))  return {};

    return ret;
  }
};

///
/// `cos()` of a real number.
///
class cos : public function
{
public:
  explicit cos(category_t c = symbol::default_category)
    : function("FCOS", c, {c}) {}

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:     return "std::cos({0})";
    default:             return      "cos({0})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p(pars[0]);
    if (!has_value(p))  return p;

    assert(!std::isinf(base(p)));
    return std::cos(base(p));
  }
};

///
/// Unprotected division (UPD) between two real numbers.
///
class div : public function
{
public:
  explicit div(return_type r = symbol::default_category,
               const param_data_types &pt = {symbol::default_category,
                                             symbol::default_category})
    : function("FDIV", r, pt)
  {
    Expects(pt.size() == 2);
  }

  [[nodiscard]] std::string to_string(format) const final
  {
    return "({0}/{1})";
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto p1(pars[1]);
    if (!has_value(p1))  return p1;

    const auto ret(base(p0) / base(p1));
    if (!std::isfinite(ret))  return {};

    return ret;
  }
};

///
/// "Greater Than" operator.
///
class gt : public function
{
public:
  explicit gt(return_type r = symbol::default_category,
               const param_data_types &pt = {symbol::default_category,
                                             symbol::default_category})
    : function(">", r, pt)
  {
    Expects(pt.size() == 2);
    Expects(pt[0] == pt[1]);
  }

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:  return "std::isgreater({0},{1})";
    default:          return "({0}>{1})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto p1(pars[1]);
    if (!has_value(p1))  return p1;

    return std::isgreater(base(p0), base(p1));
    // If one or both arguments of `isgreater` are `NaN`, the function returns
    // `false`, but no FE_INVALID exception is raised (note that the
    // expression `v0 > v1` may raise an exception in this case).
  }
};

///
/// Quotient of the division between two real numbers.
///
class idiv : public function
{
public:
  explicit idiv(return_type r = symbol::default_category,
               const param_data_types &pt = {symbol::default_category,
                                             symbol::default_category})
    : function("FIDIV", r, pt)
  {
    Expects(pt.size() == 2);
    Expects(pt[0] == pt[1]);
  }

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:     return "std::floor({0}/{1})";
    case python_format:  return          "({0}//{1})";
    default:             return      "floor({0}/{1})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto p1(pars[1]);
    if (!has_value(p1))  return p1;

    const auto ret(std::floor(base(p0) / base(p1)));
    if (!std::isfinite(ret))  return {};

    return ret;
  }
};

///
/// "If equal" operator.
///
class ife : public function
{
public:
  explicit ife(return_type r = symbol::default_category,
               const param_data_types &pt = {symbol::default_category,
                                             symbol::default_category,
                                             symbol::default_category,
                                             symbol::default_category})
    : function("FIFE", r, pt)
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
    case cpp_format:
      return "(std::fabs({0}-{1})<2*std::numeric_limits<double>::epsilon() ? "
             "{2} : {3})";
    case python_format:
      return "({2} if isclose({0}, {1}) else {3})";
    default:
      return "(fabs({0}-{1}) < 2*DBL_EPSILON ? {2} : {3})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto p1(pars[1]);
    if (!has_value(p1))  return p1;

    if (issmall(base(p0) - base(p1)))
      return pars[2];
    else
      return pars[3];
  }
};

///
/// "If less then" operator.
///
class ifl : public function
{
public:
  explicit ifl(return_type r = symbol::default_category,
               const param_data_types &pt = {symbol::default_category,
                                             symbol::default_category,
                                             symbol::default_category,
                                             symbol::default_category})
    : function("FIFL", r, pt)
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
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto p1(pars[1]);
    if (!has_value(p1))  return p1;

    const auto v0(base(p0)), v1(base(p1));
    if (std::isless(v0, v1))
      return pars[2];
    else
      return pars[3];
  }

};

///
/// "If zero" operator.
///
class ifz : public function
{
public:
  explicit ifz(return_type r = symbol::default_category,
               const param_data_types &pt = {symbol::default_category,
                                             symbol::default_category,
                                             symbol::default_category})
    : function("FIFZ", r, pt)
  {
    Expects(pt.size() == 3);
    Expects(r == pt[1]);
    Expects(pt[1] == pt[2]);
  }

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:
      return "(std::fabs({0})<2*std::numeric_limits<double>::epsilon() ? "
             "{1} : {2})";
    case python_format:
      return "({1} if abs({0}) < 1e-10 else {2})";
    default:
      return "(fabs({0})<2*DBL_EPSILON ? {1} : {2})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    return issmall(base(p0)) ? pars[1] : pars[2];
  }
};

///
/// Length of a string.
///
class length : public function
{
public:
  explicit length(return_type r, const param_data_types &pt)
    : function("FLENGTH", r, pt)
  {
    Expects(pt.size() == 1);
    Expects(r != pt[0]);
  }

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:     return "std::string({0}).length()";
    case python_format:  return                  "len({0})";
    default:             return               "strlen({0})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p(pars[0]);
    if (!has_value(p))  return p;

    return static_cast<D_DOUBLE>(std::get<D_STRING>(p).length());
  }
};

///
/// Natural logarithm of a real number.
///
/// \remark
/// The argument of a trascendental function must be dimensionless and
/// dimensionless is the return type (so same category). For a possible
/// explanation see https://math.stackexchange.com/a/238400
///
class ln : public function
{
public:
  explicit ln(category_t c = symbol::default_category)
    : function("FLN", c, {c}) {}

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:  return "std::log({0})";
    default:          return      "log({0})";
    }
  }

  ///
  /// \param[in] pars input parameters (lazy eval)
  /// \return         the natural logarithm of its argument or an empty value
  ///                 in case of invalid argument / infinite result
  ///
  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto ret(std::log(base(p0)));
    if (!std::isfinite(ret))  return {};

    return ret;
  }
};

///
/// "Less Then" operator.
///
class lt : public function
{
public:
  explicit lt(return_type r = symbol::default_category,
              const param_data_types &pt = {symbol::default_category,
                                            symbol::default_category})
    : function("<", r, pt)
  {
    Expects(pt.size() == 2);
    Expects(pt[0] == pt[1]);
  }

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:  return "std::isless({0},{1})";
    default:          return            "({0}<{1})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto p1(pars[1]);
    if (!has_value(p1))  return p1;

    return std::isless(base(p0), base(p1));
    // If one or both arguments of `isless` are NaN, the function returns
    // false, but no FE_INVALID exception is raised (note that the
    // expression `v0 < v1` may raise an exception in this case).
  }
};

///
/// The larger of two floating point values.
///
class max : public function
{
public:
  explicit max(category_t c = symbol::default_category)
    : function("FMAX", c, {c, c}) {}

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:     return "std::fmax({0},{1})";
    case python_format:  return       "max({0},{1})";
    default:             return      "fmax({0},{1})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto p1(pars[1]);
    if (!has_value(p1))  return p1;

    const auto ret(std::fmax(base(p0), base(p1)));
    if (!std::isfinite(ret))  return {};

    return ret;
  }
};

///
/// Remainder of the division between real numbers.
///
class mod : public function
{
public:
  explicit mod(return_type r = symbol::default_category,
               const param_data_types &pt = {symbol::default_category,
                                             symbol::default_category})
    : function("FMOD", r, pt)
  {
    Expects(pt.size() == 2);
    Expects(pt[0] == pt[1]);
  }

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:     return "std::fmod({0},{1})";
    case python_format:  return        "({0} % {1})";
    default:             return      "fmod({0},{1})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto p1(pars[1]);
    if (!has_value(p1))  return p1;

    const auto ret(std::fmod(base(p0), base(p1)));
    if (!std::isfinite(ret))  return {};

    return ret;
  }
};

///
/// Product of real numbers.
///
class mul : public function
{
public:
  explicit mul(return_type r = symbol::default_category,
               const param_data_types &pt = {symbol::default_category,
                                             symbol::default_category})
    : function("FMUL", r, pt)
  {
    Expects(pt.size() == 2);
    Expects(pt[0] == pt[1]);
  }

  [[nodiscard]] std::string to_string(format) const final
  {
    return "({0}*{1})";
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto p1(pars[1]);
    if (!has_value(p1))  return p1;

    const auto ret(base(p0) * base(p1));
    if (!std::isfinite(ret))  return {};

    return ret;
  }
};

///
/// sin() of a real number.
///
class sin : public function
{
public:
  explicit sin(category_t c = symbol::default_category)
    : function("FSIN", c, {c}) {}

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:     return "std::sin({0})";
    default:             return      "sin({0})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p(pars[0]);
    if (!has_value(p))  return p;

    assert(!std::isinf(base(p)));
    return std::sin(base(p));
  }
};

///
/// Square root of a real number.
///
class sqrt : public function
{
public:
  explicit sqrt(category_t c = symbol::default_category)
    : function("FSQRT", c, {c}) {}

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:     return "std::sqrt({0})";
    default:             return      "sqrt({0})";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p(pars[0]);
    if (!has_value(p))  return p;

    const auto v(base(p));
    if (std::isless(v, 0.0))
      return {};

    return std::sqrt(v);
  }
};

///
/// Subtraction between real numbers.
///
class sub : public function
{
public:
  explicit sub(category_t c = symbol::default_category)
    : function("FSUB", c, {c, c}) {}

  [[nodiscard]] std::string to_string(format) const final
  {
    return "({0}-{1})";
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    const auto p1(pars[1]);
    if (!has_value(p1))  return p1;

    const auto ret(base(p0) - base(p1));
    if (!std::isfinite(ret))  return {};

    return ret;
  }
};

///
/// Sigmoid function.
///
class sigmoid : public function
{
public:
  explicit sigmoid(category_t c = symbol::default_category)
    : function("FSIGMOID", c, {c}) {}

  [[nodiscard]] std::string to_string(format f) const final
  {
    switch (f)
    {
    case cpp_format:     return "1.0 / (1.0 + std::exp(-{0}))";
    case python_format:  return   "1. / (1. + math.exp(-{0}))";
    default:             return          "1 / (1 + exp(-{0}))";
    }
  }

  [[nodiscard]] value_t eval(const params &pars) const final
  {
    const auto p0(pars[0]);
    if (!has_value(p0))  return p0;

    // The sigmoid function can be expressed in one of two equivalent ways:
    //     sigmoid(x) = 1 / (1 + exp(-x)) = exp(x) / (exp(x) + 1)
    // Each version can be used in order to avoid numerical overflow in extreme
    // cases (`x --> +inf` and `x --> -inf` respectively).
    const auto x(base(p0));

    return x >= 0.0 ? 1.0 / (1.0 + std::exp(-x))
                    : std::exp(x) / (1.0 + std::exp(x));
  }
};

}  // namespace ultra::real

#endif  // include guard
