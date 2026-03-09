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

#if !defined(ULTRA_ALPS_H)
#define      ULTRA_ALPS_H

#include "kernel/population.h"

#include <array>
#include <cstddef>
#include <functional>
#include <span>

namespace ultra
{

///
/// Lightweight view over one or two ALPS layers.
///
/// \tparam Layer layer type. Can be `const` to model read-only access.
///
/// This class stores references to a _primary_ layer and, optionally, a
/// _secondary_ layer. It's designed to avoid dynamic allocation in helper
/// functions that, by policy, return either one or two eligible layers.
///
/// The object is iterable. When no secondary layer is present, `size()` is `1`
/// and the iteration covers only the primary layer.
///
/// \warning
/// `alps_layer_pair` does not own the layers. The referenced layers must
/// outlive the `alps_layer_pair` instance.
///
/// \note
/// When constructed with a single layer, the internal storage keeps two copies
/// of the primary reference, but `size()`/iteration expose only one element.
///
template<class Layer>
class alps_layer_pair
{
public:
  /// Builds a pair with a single eligible layer (primary only).
  ///
  /// \param[in] l primary layer
  ///
  /// After construction `size() == 1` and `has_secondary() == false`.
  alps_layer_pair(Layer &l) noexcept
    : layers_{std::ref(l), std::ref(l)}, has_secondary_(false) {}

  /// Builds a pair with two eligible layers (primary and secondary).
  ///
  /// \param[in] p primary layer
  /// \param[in] s secondary layer
  ///
  /// After construction `size() == 2` and `has_secondary() == true`.
  alps_layer_pair(Layer &p, Layer &s) noexcept
    : layers_{std::ref(p), std::ref(s)}, has_secondary_(true) {}

  /// \return number of exposed layers (1 or 2)
  [[nodiscard]] constexpr std::size_t size() const noexcept
  {
    return 1 + static_cast<std::size_t>(has_secondary_);
  }

  /// \return `true` if a secondary layer is present
  [[nodiscard]] constexpr bool has_secondary() const noexcept
  {
    return has_secondary_;
  }

  /// \return iterator to the first exposed layer reference
  [[nodiscard]] constexpr auto begin() const noexcept
  {
    return layers_.begin();
  }

  /// \return iterator one past the last exposed layer reference
  [[nodiscard]] constexpr auto end() const noexcept
  {
    return layers_.begin() + static_cast<std::ptrdiff_t>(size());
  }

  /// \return the primary layer
  [[nodiscard]] constexpr Layer &primary() const noexcept
  {
    return layers_[0].get();
  }

  /// \return the secondary layer
  ///
  /// \pre `has_secondary() == true`
  [[nodiscard]] constexpr Layer &secondary() const noexcept
  {
    Expects(has_secondary());
    return layers_[1].get();
  }

  /// Picks a layer at random.
  ///
  /// \param[in] p_pri probability of selecting the primary layer
  /// \return          either `primary()` (with probability `p_pri`) or
  ///                  `secondary()` (with probability `1 - p_pri`)
  ///
  /// \pre `0.0 < p_pri && p_pri <= 1.0`.
  ///
  /// \note
  /// If no secondary layer is present, this function always returns
  /// `primary()`, regardless of `p_pri`.
  [[nodiscard]] Layer &random(double p_pri) const
  {
    Expects(in_0_1(p_pri));
    return !has_secondary_ || random::boolean(p_pri) ? primary() : secondary();
  }

private:
  std::array<std::reference_wrapper<Layer>, 2> layers_;
  bool has_secondary_ {false};
};

template<class Layer>
alps_layer_pair(Layer &) -> alps_layer_pair<Layer>;

template<class Layer>
alps_layer_pair(Layer &, Layer &) -> alps_layer_pair<Layer>;

template<class Layer>
alps_layer_pair(const Layer &) -> alps_layer_pair<const Layer>;

template<class Layer>
alps_layer_pair(const Layer &, const Layer &) -> alps_layer_pair<const Layer>;

namespace alps
{

///
/// Parameters for the Age-Layered Population Structure (ALPS) paradigm.
///
/// ALPS is a meta heuristic for overcoming premature convergence by
/// running multiple instances of a search algorithm in parallel, with each
/// instance in its own age layer and having its own population.
///
struct parameters
{
  [[nodiscard]] unsigned max_age(std::size_t) const;
  [[nodiscard]] unsigned max_age(std::size_t, std::size_t) const;

  /// The maximum ages for age layers is monotonically increasing and
  /// different methods can be used for setting these values. Since there is
  /// generally little need to segregate individuals which are within a few
  /// "generations" of each other, these values are then multiplied by an
  /// `age_gap` parameter. In addition, this allows individuals in the first
  /// age-layer some time to be optimized before them, or their offspring, are
  /// pushed to the next age layer.
  /// For instance, with 6 age layers, a linear aging-scheme and an age gap of
  /// 20, the maximum ages for the layers are: 20, 40, 60, 80, 100, 120.
  ///
  /// Also, the `age_gap` parameter sets the frequency of how often the first
  /// layer is restarted.
  ///
  /// \note A value of 0 means undefined (auto-tune).
  unsigned age_gap {20};

  /// Maximum number of layers an ALPS layered population can grow to.
  std::size_t max_layers {8};

  /// The probability that a parent will be extracted from the main layer.
  ///
  /// \note
  /// A negative value means auto-tune.
  double p_main_layer {0.75};
};

template<LayeredPopulation P>
void set_age(P &pop)
{
  const auto layers(pop.layers());
  if (!layers)
    return;

  const auto &params(pop.problem().params);

  for (std::size_t l(0); l < layers; ++l)
    pop.layer(l).max_age(params.alps.max_age(l, layers));
}

///
/// Determines the set of layers whose individuals may be replaced by offspring
/// generated from the specified layer.
///
/// \param[in] pop layered population
/// \param[in] l   iterator to the layer producing the offspring
/// \return        an `alps_layer_pair` containing the layers eligible for
///                replacement
///
/// \pre `pop` contains at least one layer
/// \pre `l`   is a valid iterator into `pop.range_of_layers()`
///
/// In the ALPS paradigm, replacement is restricted in order to preserve age
/// stratification. Individuals are typically replaced either within the same
/// layer or, in some cases, in the oldest layer.
///
/// The replacement policy implemented here is:
/// - if `l` refers to the last (oldest) layer, only that layer is eligible;
/// - otherwise, both the current layer and the last layer are eligible.
///
/// \note
/// The returned object stores references to layers owned by `pop`; it doesn't
/// extend their lifetime.
///
template<LayeredPopulation P>
alps_layer_pair<typename P::layer_t>
replacement_layers(P &pop, typename P::layer_iter l)
{
  Expects(pop.layers());
  Expects(iterator_of(l, pop.range_of_layers()));

  if (l == std::prev(pop.range_of_layers().end()))
    return {std::ref(*l)};

  return {std::ref(*l), std::ref(pop.back())};
}

///
/// Determines the set of layers from which parents may be selected when
/// generating offspring for the specified layer.
///
/// \param[in] pop layered population
/// \param[in] l   iterator to the layer for which parents are being selected
/// \return        an `alps_layer_pair` containing the layers eligible for
///                parent selection
///
/// \pre `pop` contains at least one layer
/// \pre `l` is a valid iterator into `pop.range_of_layers()`
///
/// In ALPS, parent selection is typically restricted to the same age layer or
/// younger ones, preventing older individuals from influencing younger layers.
///
/// The selection policy implemented here is:
/// - if `l` refers to the first (youngest) layer, only that layer is used;
/// - otherwise, both the current layer and the immediately younger layer are
///   used.
///
/// \note
/// The returned object stores references to layers owned by `pop`; it doesn't
/// extend their lifetime.
///
template<LayeredPopulation P>
alps_layer_pair<const typename P::layer_t>
selection_layers(const P &pop, typename P::layer_const_iter l)
{
  Expects(pop.layers());
  Expects(iterator_of(l, pop.range_of_layers()));

  if (l == pop.range_of_layers().begin())
    return {std::cref(*l)};

  return {std::cref(*l), std::cref(*std::prev(l))};
}

}  // namespace alps
}  // namespace ultra

#endif  // include guard
