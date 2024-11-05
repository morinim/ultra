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

#include <functional>

#include "kernel/fitness.h"

namespace ultra
{

///
/// Builds a fitness from a list of values.
///
fitnd::fitnd(std::initializer_list<double> l) : vect_(l)
{
}

///
/// Fills the fitness with copies of a given value.
///
/// \param[in] s dimension of the the fitness vector
/// \param[in] v default value
///
/// Both Herb Sutter and Scott Meyers recommend to avoid class designs where
/// a `initializer_list` constructor overload can cause ambiguities to the
/// programmer. We use a tag to avoid such situations.
///
/// The tag also helps to clarify the meaning of the other arguments.
///
fitnd::fitnd(with_size s, value_type v) : vect_(s(), v)
{
  Expects(s());
}

///
/// Builds a fitness from a vector of values.
///
fitnd::fitnd(values_t v) : vect_(std::move(v))
{
}

///
/// \return the size of the fitness vector
///
std::size_t fitnd::size() const
{
  return vect_.size();
}

///
/// \param[in] i index of an element
/// \return      the `i`-th element of the fitness vector
///
fitnd::value_type fitnd::operator[](std::size_t i) const
{
  // This assert could be considered a bit too strict. In general taking the
  // address of one past the last element is allowed, e.g.
  //     std::copy(&f[0], &f[N], &dest);
  // but here the assertion will signal this use case. The workaround is:
  //     std::copy(f.begin(), f.end(), &dest);
  Expects(i < size());
  return vect_[i];
}

///
/// \param[in] i index of an element
/// \return      a reference to the `i`-th element of the fitness vector
///
fitnd::value_type &fitnd::operator[](std::size_t i)
{
  Expects(i < size());
  return vect_[i];
}

///
/// \return returns an iterator to the first element of the container. If the
///         container is empty, the returned iterator will be equal to `end()`
///
fitnd::iterator fitnd::begin()
{
  return std::begin(vect_);
}

///
/// \return returns an iterator to the first element of the container. If the
///         container is empty, the returned iterator will be equal to `end()`
///
fitnd::const_iterator fitnd::begin() const
{
  return vect_.cbegin();
}

///
/// \return returns an iterator to the element following the last element of
///         the container. This element acts as a placeholder; attempting to
//          access it results in undefined behavior
///
fitnd::const_iterator fitnd::end() const
{
  return std::cend(vect_);
}

///
/// Deserialises a `fitnd` value.
///
/// \param[in]  in input stream
/// \param[out] f  load the fitness here
/// \return       `true` if the object has been loaded correctly
///
/// \note
/// If the load operation isn't successful 'f' isn't changed.
///
/// \relates fitnd
///
bool load(std::istream &in, fitnd *f)
{
  std::string line;
  if (!std::getline(in >> std::ws, line))
    return false;

  std::istringstream line_in(line);

  fitnd tmp;
  for (fitnd::value_type elem; load_float_from_stream(line_in, &elem);)
    tmp.vect_.push_back(elem);

  *f = tmp;
  return true;
}

///
/// Standard output operator for `fitnd`.
///
/// \param[out] o output stream to print data to
/// \param[in]  f constant reference to a fitness to insert
/// \return       `o`
///
/// This function is used for displaying values / debugging. For serialisation
/// use the `save` function.
///
/// \relates fitnd
///
std::ostream &operator<<(std::ostream &o, const fitnd &f)
{
  o << '(';

  if (auto it(f.begin()); it != f.end())
  {
    o << *it;

    while (++it != f.end())
      o << ", " << *it;
  }

  return o << ')';
}

///
/// Standard input operator for `fitnd`.
///
/// \param[in]  in a character input stream
/// \param[out] f  a fitness to be extracted
/// \return        `in`
///
/// \warning
/// For deserialisation use the `load` function.
///
/// \relates fitnd
///
std::istream &operator>>(std::istream &in, fitnd &f)
{
  in >> std::ws;

  std::string values;

  if (in.peek() == '(')
  {
    in.get();  // discards the open parenthesis

    std::getline(in >> std::ws, values, ')');
    std::ranges::replace(values, ',', ' ');

  }
  else
    in >> values;

  fitnd tmp;
  if (std::istringstream ss(values); load(ss, &tmp))
    f = tmp;

  return in;
}

///
/// \param[in] f a fitness
/// \return      the sum of `this` and `f`
///
fitnd &fitnd::operator+=(const fitnd &f)
{
  std::ranges::transform(*this, f, begin(), std::plus{});

  return *this;
}

///
/// \param[in] lhs first addend
/// \param[in] rhs second addend
/// \return        the sum of `lhs` and `rhs`
///
/// \relates fitnd
///
fitnd operator+(fitnd lhs, const fitnd &rhs)
{
  // operator+ shouldn't be a member function otherwise it won't work as
  // naturally as user may expect (i.e. asymmetry in implicit conversion from
  // other types).
  // Implementing `+` in terms of `+=` makes the code simpler and guarantees
  // consistent semantics as the two functions are less likely to diverge
  // during maintenance.
  return lhs += rhs;
}

///
/// \param[in] f a fitness
/// \return      the difference of `this` and `f`
///
fitnd &fitnd::operator-=(const fitnd &f)
{
  std::ranges::transform(*this, f, begin(), std::minus{});

  return *this;
}

///
/// \param[in] lhs minuend
/// \param[in] rhs subtrahend
/// \return        difference between `lhs` and `rhs`
///
/// \relates fitnd
///
fitnd operator-(fitnd lhs, const fitnd &rhs)
{
  return lhs -= rhs;
}

///
/// \param[in] f a fitness
/// \return      product of `this` and `f`
///
fitnd &fitnd::operator*=(const fitnd &f)
{
  std::ranges::transform(*this, f, begin(), std::multiplies{});

  return *this;
}

///
/// \param[in] lhs first factor
/// \param[in] rhs second factor
/// \return        the product of `lhs` and `rhs`
///
/// \relates fitnd
///
fitnd operator*(fitnd lhs, const fitnd &rhs)
{
  return lhs *= rhs;
}

///
/// \param[in] f a fitness
/// \return      division of `this` and `f`
///
fitnd &fitnd::operator/=(const fitnd &f)
{
  std::ranges::transform(*this, f, begin(), std::divides{});

  return *this;
}

///
/// \param[in] lhs first factor
/// \param[in] rhs second factor
/// \return        the division of `lhs` and `rhs`
///
/// \relates fitnd
///
fitnd operator/(fitnd lhs, const fitnd &rhs)
{
  return lhs /= rhs;
}

///
/// \param[in] f a fitness value
/// \param[in] v a scalar
/// \return      a new vector obtained dividing each component of `f` by the
///              scalar value `v`
///
/// \relates fitnd
///
fitnd operator/(fitnd f, fitnd::value_type v)
{
  std::ranges::transform(f, f.begin(), [v](auto fi) { return fi / v; });
  return f;
}

///
/// \param[in] f a fitness value
/// \param[in] v a scalar
/// \return      a new vector obtained multiplying each component of `f` by the
///              scalar value `v`
///
/// \relates fitnd
///
fitnd operator*(fitnd f, fitnd::value_type v)
{
  std::ranges::transform(f, f.begin(), [v](auto fi) { return fi * v; });
  return f;
}

fitnd operator-(fitnd f)
{
  std::ranges::transform(f, f.begin(), std::negate{});
  return f;
}

///
/// \param[in] f a fitness value
/// \return      a new vector obtained taking the absolute value of each
///              component of `f`
///
fitnd abs(fitnd f)
{
  std::ranges::transform(f, f.begin(), [](auto v) { return std::abs(v); });
  return f;
}

///
/// \param[in] f a fitness
/// \return      a new vector obtained taking the square root of each component
///              of `f`
///
fitnd sqrt(fitnd f)
{
  std::ranges::transform(f, f.begin(), [](auto v) { return std::sqrt(v); });
  return f;
}

///
/// \param[in] f1 first fitness value
/// \param[in] f2 second fitness value
/// \return       the fitness vector obtained joining `f1` and `f2`
///
/// \relates fitnd
///
fitnd combine(const fitnd &f1, const fitnd &f2)
{
  fitnd ret;
  ret.vect_.reserve(f1.size() + f2.size());

  ret.vect_.insert(std::end(ret), std::begin(f1), std::end(f1));
  ret.vect_.insert(std::end(ret), std::begin(f2), std::end(f2));

  return ret;
}

}  // namespace ultra
