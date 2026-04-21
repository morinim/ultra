/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_DECISION_VECTOR_H)
#define      ULTRA_DECISION_VECTOR_H

#include <concepts>
#include <cstddef>
#include <utility>
#include <vector>

namespace ultra
{

/// Original scalar category used when writing back an optimised value.
enum class dv_param_kind : unsigned char { real, integer };

///
/// A flat optimiser-facing view of tunable scalar parameters.
///
/// A `decision_vector<Coord>` separates two concerns:
/// - `values` stores the numerical parameters to be optimised;
/// - `coords` stores the metadata needed to map each optimised value back to
///   the originating object.
///
/// The template parameter `Coord` is domain-specific. It identifies where a
/// parameter comes from and where an updated value must be written back.
///
template<class Coord>
struct decision_vector
{
  /// Metadata describing one optimisable parameter.
  ///
  /// `coord` identifies the parameter within the source object.
  /// `kind` specifies how the optimised scalar must be written back.
  struct coordinate
  {
    Coord coord;
    dv_param_kind kind;
  };

  decision_vector() = default;
  decision_vector(std::vector<double> values, std::vector<coordinate> coords)
    : values(std::move(values)), coords(std::move(coords))
  {}

  std::vector<double> values {};
  std::vector<coordinate> coords {};

  [[nodiscard]] bool empty() const noexcept { return values.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return values.size(); }

  [[nodiscard]] bool is_valid() const noexcept
  {
    return values.size() == coords.size();
  }
};

/// Checks whether a type models a concrete decision-vector representation.
template<class DV>
concept DecisionVector = requires(DV dv, const DV cdv)
{
  typename DV::coordinate;

  { dv.values } -> std::same_as<std::vector<double> &>;
  { cdv.values } -> std::same_as<const std::vector<double> &>;

  { dv.coords } -> std::same_as<std::vector<typename DV::coordinate> &>;
  { cdv.coords } -> std::same_as<const std::vector<typename DV::coordinate> &>;

  { cdv.empty() } -> std::convertible_to<bool>;
  { cdv.size() } -> std::convertible_to<std::size_t>;
};

///
/// Checks whether a type supports decision vector extraction.
///
template<class T>
concept DecisionVectorExtractable = requires(const T &t)
{
  { extract_decision_vector(t) } -> DecisionVector;
};

/// Concrete decision-vector type associated with `I`.
template<class I> using decision_vector_t =
  std::remove_cvref_t<
    decltype(extract_decision_vector(std::declval<const I &>()))>;

}  // namespace ultra

#endif  // include guard
