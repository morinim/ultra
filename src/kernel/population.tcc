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

#if !defined(ULTRA_POPULATION_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_POPULATION_TCC)
#define      ULTRA_POPULATION_TCC

///
/// Creates a random population.
///
/// \param[in] p current problem
///
template<Individual I>
population<I>::population(const ultra::problem &p)
  : prob_(&p), layers_{p.env.population.layers}
{
  const auto n(p.env.population.individuals);
  for (std::size_t l(0); l < layers(); ++l)
  {
    layers_[l].members.reserve(n);
    layers_[l].allowed = n;
    init_layer(l);
  }

  Ensures(layers() == p.env.population.layers);
  Ensures(is_valid());
}

///
/// Resets layer `l` of the population.
///
/// \param[in] l a layer of the population
///
/// \warning If layer `l` is nonexistent/empty the method doesn't work!
///
template<Individual I>
void population<I>::init_layer(std::size_t l)
{
  Expects(l < layers());

  layer &ll(layers_[l]);

  ll.members.clear();

  std::generate_n(std::back_inserter(ll.members), ll.allowed,
                  [this] {return I(problem()); });
}

///
/// \return number of active layers
///
/// \note
/// The number of active layers is a dynamic value (almost monotonically
/// increasing with the generation number).
///
template<Individual I>
std::size_t population<I>::layers() const
{
  return layers_.size();
}

///
/// \return the number of individuals in the population
///
template<Individual I>
std::size_t population<I>::individuals() const
{
  using ret_t = decltype(individuals());

  return std::accumulate(layers_.begin(), layers_.end(), ret_t(0),
                         [](auto accumulator, const auto &l)
                         {
                           return accumulator
                                  + static_cast<ret_t>(l.members.size());
                         });
}

///
/// \param[in] l a layer
/// \return      the number of individuals in layer `l`
///
template<Individual I>
std::size_t population<I>::individuals(std::size_t l) const
{
  Expects(l < layers());
  return layers_[l].members.size();
}

///
/// \return a constant reference to the active problem
///
template<Individual I>
const ultra::problem &population<I>::problem() const
{
  Expects(prob_);
  return *prob_;
}

///
/// \param[in] l a layer
/// \return      the number of individuals allowed in layer `l`
///
/// \note
/// `for each l individuals(l) < allowed(l)`
///
template<Individual I>
std::size_t population<I>::allowed(std::size_t l) const
{
  Expects(l < layers());
  return layers_[l].allowed;
}

///
/// \param[in] c coordinates of an individual
/// \return      a reference to the individual at coordinates `c`
///
template<Individual I>
I &population<I>::operator[](const coord &c)
{
  Expects(c.layer < layers());
  Expects(c.index < individuals(c.layer));
  return layers_[c.layer].members[c.index];
}

///
/// \param[in] c coordinates of an individual
/// \return      a constant reference to the individual at coordinates `c`
///
template<Individual I>
const I &population<I>::operator[](const coord &c) const
{
  Expects(c.layer < layers());
  Expects(c.index < individuals(c.layer));
  return layers_[c.layer].members[c.index];
}

///
/// Adds individual `i` to layer `l`.
///
/// \param[in] l index of a layer
/// \param[in] i an individual
///
template<Individual I>
void population<I>::add_to_layer(std::size_t l, const I &i)
{
  Expects(l < layers());

  if (individuals(l) < allowed(l))
    layers_[l].members.push_back(i);
}

///
/// Removes the last individual of layer `l`.
///
/// \param[in] l index of a layer
///
template<Individual I>
void population<I>::pop_from_layer(std::size_t l)
{
  Expects(l < layers());
  layers_[l].members.pop_back();
}

///
/// Adds a new layer to the population.
///
/// The new layer is inserted as the lower layer and randomly initialized.
///
template<Individual I>
void population<I>::add_layer()
{
#if !defined(NDEBUG)
  const auto nl(layers());
#endif

  layers_.insert(layers_.begin(), layer());
  layers_[0].allowed = problem().env.population.individuals;
  layers_[0].members.reserve(layers_[0].allowed);

  init_layer(0);

#if !defined(NDEBUG)
  Ensures(layers() == nl + 1);
#endif
}

template<Individual I>
void population<I>::remove_layer(std::size_t l)
{
  Expects(l < layers());

  layers_.erase(std::next(layers_.begin(), l));
}

///
/// \return a `const_iterator` pointing to the first layer of the population
///
/// \warning Pointer to the first LAYER *NOT* to the first PROGRAM.
///
template<Individual I>
typename population<I>::const_iterator population<I>::begin() const
{
  return const_iterator(*this, true);
}

///
/// \return an iterator pointing one element past the last individual of the
///         population
///
template<Individual I>
typename population<I>::const_iterator population<I>::end() const
{
  return const_iterator(*this, false);
}

///
/// \return `true` if the object passes the internal consistency check
///
template<Individual I>
bool population<I>::is_valid() const
{
  for (const auto &i : *this)
    if (!i.is_valid())
      return false;

  const auto n(layers());
  for (std::size_t l(0); l < n; ++l)
  {
    if (allowed(l) < individuals(l))
      return false;

    if (layers_[l].members.capacity() < allowed(l))
      return false;
  }

  if (!prob_)
  {
    ultraERROR << "Undefined `problem`";
    return false;
  }

  return true;
}

#endif  // include guard
