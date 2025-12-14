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

#if !defined(ULTRA_FITNESS_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_FITNESS_TCC)
#define      ULTRA_FITNESS_TCC

///
/// Builds a fitness from a list of values.
///
inline constexpr fitnd::fitnd(std::initializer_list<double> l) : vect_(l)
{
}

template<Fitness F>
[[nodiscard]] constexpr F lowest() noexcept
{
  return std::numeric_limits<F>::lowest();
}

template<std::integral F>
[[nodiscard]] bool isfinite(F) noexcept
{
  return true;
}

template<std::floating_point F>
[[nodiscard]] bool isfinite(F f)
{
  return std::isfinite(f);
}

///
/// \param[in] f fitness to check
/// \return      `true` if every component of the fitness is finite
///
template<MultiDimFitness F>
[[nodiscard]] bool isfinite(const F &f)
{
  return std::ranges::all_of(f, [](auto v) { return isfinite(v); });
}

///
/// Pareto dominance comparison.
///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
/// \return        `true` if `lhs` is a Pareto improvement of `rhs`
///
/// `lhs` dominates `rhs` (is a Pareto improvement) if:
/// - each component of `lhs` is not strictly worst (less) than the
///   correspondig component of `rhs`;
/// - there is at least one component in which `lhs` is better than `rhs`.
///
/// \note
/// An interesting property is that if a vector `x` does not dominate a
/// vector `y`, this does not imply that `y` dominates `x` (they can be both
/// non-dominated).
///
template<Fitness F>
requires std::is_arithmetic_v<F>
[[nodiscard]] bool dominating(const F &lhs, const F &rhs)
{
  return rhs < lhs;
}

template<MultiDimFitness F>
[[nodiscard]] bool dominating(const F &lhs, const F &rhs)
{
  bool one_better(lhs.size() && !rhs.size());

  const auto n(std::min(lhs.size(), rhs.size()));
  for (std::size_t i(0); i < n; ++i)
    if (rhs[i] < lhs[i])
      one_better = true;
    else if (lhs[i] < rhs[i])
      return false;

  return one_better;
}

template<MultiDimFitness F>
[[nodiscard]] bool almost_equal(const F &lhs, const F &rhs)
{
  return std::ranges::equal(
    lhs, rhs,
    [](auto v1, auto v2) { return almost_equal(v1, v2); });
}

///
/// Taxicab distance between two vectors.
///
/// \param[in] f1 first fitness value
/// \param[in] f2 second fitness value
/// \return       the distance between `f1` and `f2`
///
/// The taxicab distance between two vectors in an n-dimensional real vector
/// space with fixed Cartesian coordinate system, is the sum of the lengths of
/// the projections of the line segment between the points onto the coordinate
/// axes.
///
template<Fitness F>
requires std::is_arithmetic_v<F>
[[nodiscard]] double distance(F f1, F f2)
{
  return std::fabs(f1 - f2);
}

template<MultiDimFitness F>
[[nodiscard]] double distance(const F &f1, const F &f2)
{
  Expects(f1.size() == f2.size());

  return std::transform_reduce(
    f1.begin(), f1.end(), f2.begin(), 0.0,
    std::plus{}, [](auto a, auto b) { return std::fabs(a - b); });
}

///
/// \param[out] out output stream
/// \param[in]  f   fitness to be saved
/// \return         `true` if object has been saved correctly
///
template<Fitness F>
requires std::floating_point<F>
[[nodiscard]] bool save(std::ostream &out, F f)
{
  save_float_to_stream(out, f);
  out << '\n';

  return out.good();
}

template<std::integral F>
[[nodiscard]] bool save(std::ostream &out, F f)
{
  out << f << '\n';

  return out.good();
}

template<MultiDimFitness F>
[[nodiscard]] bool save(std::ostream &out, const F &f)
{
  if (auto it(f.begin()); it != f.end())
  {
    save_float_to_stream(out, *it);

    while (++it != f.end())
    {
      out << ' ';
      save_float_to_stream(out, *it);
    }
  }

  out << '\n';

  return out.good();
}

///
/// \param[in]  in input stream
/// \param[out] f  load the fitness here
/// \return        `true` if the object has been loaded correctly
///
/// \note
/// If the load operation isn't successful `f` isn't changed.
///
template<std::floating_point F>
bool load(std::istream &in, F *f)
{
  return load_float_from_stream(in, f);
}

template<std::integral F>
bool load(std::istream &in, F *f)
{
  return in >> *f;
}

#endif  // include guard
