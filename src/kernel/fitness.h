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

#if !defined(ULTRA_FITNESS_H)
#define      ULTRA_FITNESS_H

#include <cmath>
#include <numeric>
#include <vector>

#include "utility/assert.h"
#include "utility/misc.h"

namespace ultra
{

///
/// This is **NOT THE RAW FITNESS**. Raw fitness is stated in the natural
/// terminology of the problem: the better value may be either smaller (as when
/// raw fitness is error) or larger (as when raw fitness is food eaten, benefit
/// achieved...).
///
/// We use a **STANDARDIZED FITNESS**: a greater numerical value is **always**
/// a better value (in many examples the optimal value is `0`, but this isn't
/// strictly necessary).
///
/// If, for a particular problem, a greater value of raw fitness is better,
/// standardized fitness equals the raw fitness for that problem (otherwise
/// standardized fitness must be computed from raw fitness).
///
/// \warning
/// The definition of standardized fitness given here is different from that
/// used in Koza's "Genetic Programming: On the Programming of Computers by
/// Means of Natural Selection". In the book a **LOWER** numerical value is
/// always a better one.
/// The main difference is that Vita attempts to maximize the fitness (while
/// other applications try to minimize it).
/// We chose this convention since it seemed more natural (a greater fitness
/// is a better fitness; achieving a better fitness means to maximize the
/// fitness). The downside is that sometimes we have to manage negative
/// numbers, but for our purposes it's not so bad.
/// Anyway maximization and minimization problems are basically the same: the
/// solution of `max(f(x))` is the same as `-min(-f(x))`. This is usually
/// all you have to remember when dealing with examples/problems expressed
/// in the other notation.
///
template<class F> concept Fitness =
  OrderedArithmeticType<F>
  && (std::ranges::sized_range<F> || (requires { requires F(-1) < F(0); }));

template<class F> concept MultiDimFitness =
  Fitness<F> && std::ranges::sized_range<F>;

///
/// Tag representing size.
///
/// Used to initialize containers in a way that is completely unambiguous.
///
/// \see https://akrzemi1.wordpress.com/2016/06/29/competing-constructors/
///
class with_size
{
public:
  explicit with_size(std::size_t s) : size_(s) {}
  std::size_t operator()() const { return size_; }

private:
  std::size_t size_;
};

///
/// A basic multi-dimensional fitness type.
///
/// Useful for rapid prototyping. Real use cases may require ad-hoc fitness
/// type.
///
class fitnd
{
public:
  // Type alias and iterators.
  using value_type = double;
  using values_t = std::vector<value_type>;
  using iterator = values_t::iterator;
  using const_iterator = values_t::const_iterator;
  using difference_type = values_t::difference_type;

  fitnd() = default;
  fitnd(std::initializer_list<double>);
  fitnd(values_t);
  fitnd(with_size, value_type = std::numeric_limits<value_type>::lowest());

  [[nodiscard]] std::size_t size() const;
  [[nodiscard]] value_type operator[](std::size_t) const;
  [[nodiscard]] value_type &operator[](std::size_t);

  [[nodiscard]] iterator begin();
  [[nodiscard]] const_iterator begin() const;
  [[nodiscard]] const_iterator end() const;

  [[nodiscard]] friend auto operator<=>(const fitnd &, const fitnd &) = default;

  fitnd &operator+=(const fitnd &);
  fitnd &operator-=(const fitnd &);
  fitnd &operator*=(const fitnd &);
  fitnd &operator/=(const fitnd &);

  friend bool load(std::istream &, fitnd *);
  friend fitnd combine(const fitnd &, const fitnd &);

private:
  values_t vect_ {};
};

// ***********************************************************************
// *  Arithmetic operators                                               *
// ***********************************************************************
[[nodiscard]] fitnd operator+(fitnd, const fitnd &);
[[nodiscard]] fitnd operator-(fitnd, const fitnd &);
[[nodiscard]] fitnd operator*(fitnd, const fitnd &);
[[nodiscard]] fitnd operator/(fitnd, const fitnd &);
[[nodiscard]] fitnd operator*(fitnd, fitnd::value_type);
[[nodiscard]] fitnd operator/(fitnd, fitnd::value_type);
[[nodiscard]] fitnd operator-(fitnd);

// ***********************************************************************
// *  Serialization                                                      *
// ***********************************************************************
[[nodiscard]] bool load(std::istream &, fitnd *);

// ***********************************************************************
// *  Other functions                                                    *
// ***********************************************************************
[[nodiscard]] fitnd abs(fitnd);
[[nodiscard]] fitnd sqrt(fitnd);
[[nodiscard]] fitnd combine(const fitnd &, const fitnd &);
std::ostream &operator<<(std::ostream &, const fitnd &);

#include "kernel/fitness.tcc"

}  // namespace ultra

#endif  // include guard
