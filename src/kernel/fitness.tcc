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
///   corresponding component of `rhs`;
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
  Expects(std::ranges::size(lhs) == std::ranges::size(rhs));

  bool one_better(false);

  auto it_lhs(std::ranges::begin(lhs));
  auto it_rhs(std::ranges::begin(rhs));
  const auto end_lhs(std::ranges::end(lhs));

  for (; it_lhs != end_lhs; ++it_lhs, ++it_rhs)
    if (*it_rhs < *it_lhs)
      one_better = true;
    else if (*it_lhs < *it_rhs)
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
  if constexpr (std::floating_point<F>)
    return std::fabs(f1 - f2);
  else
  {
    // Compute the absolute difference in the corresponding unsigned type.
    // This avoids signed overflow, including the lowest()/max() case.
    // Conversion from signed to unsigned is well-defined modulo 2^N and the
    // subtraction is then performed in the unsigned domain.
    using U = std::make_unsigned_t<F>;

    return static_cast<double>(f1 >= f2 ? U(f1) - U(f2) : U(f2) - U(f1));
  }
}

template<MultiDimFitness F>
[[nodiscard]] double distance(const F &f1, const F &f2)
{
  Expects(std::ranges::size(f1) == std::ranges::size(f2));

  return std::transform_reduce(
    std::ranges::begin(f1), std::ranges::end(f1), std::ranges::begin(f2), 0.0,
    std::plus{}, [](auto a, auto b) { return distance(a, b); });
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
requires std::floating_point<std::ranges::range_value_t<F>>
[[nodiscard]] bool save(std::ostream &out, const F &f)
{
  out << std::ranges::size(f);

  for (auto v : f)
  {
    out << ' ';
    save_float_to_stream(out, v);
  }

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
