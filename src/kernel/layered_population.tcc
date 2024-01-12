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

#if !defined(ULTRA_LAYERED_POPULATION_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_LAYERED_POPULATION_TCC)
#define      ULTRA_LAYERED_POPULATION_TCC

///
/// Creates a random population.
///
/// \param[in] p current problem
///
template<Individual I>
layered_population<I>::layered_population(const ultra::problem &p)
  : prob_(&p), layers_{p.env.population.layers}
{
  for (auto &l : layers_)
    init(l);

  Ensures(layers() == p.env.population.layers);
  Ensures(is_valid());
}

///
/// \param[in] l layer index
/// \return      a const reference to a layer of the population
///
template<Individual I>
const typename layered_population<I>::layer_t &
layered_population<I>::layer(std::size_t l) const
{
  Expects(l < layers());
  return layers_[l];
}

///
/// \param[in] l layer index
/// \return      reference to a layer of the population
///
template<Individual I>
typename layered_population<I>::layer_t &
layered_population<I>::layer(std::size_t l)
{
  Expects(l < layers());
  return layers_[l];
}

///
/// \return const reference to the first layer of the population
///
template<Individual I>
const typename layered_population<I>::layer_t &
layered_population<I>::front() const
{
  Expects(layers());
  return layer(0);
}

///
/// \return reference to the first layer of the population
///
template<Individual I>
typename layered_population<I>::layer_t &layered_population<I>::front()
{
  Expects(layers());
  return layer(0);
}

///
/// \return const reference to the last layer of the population
///
template<Individual I>
const typename layered_population<I>::layer_t &
layered_population<I>::back() const
{
  Expects(layers());
  return layers_.back();
}

///
/// \return reference to the last layer of the population
///
template<Individual I>
typename layered_population<I>::layer_t &layered_population<I>::back()
{
  Expects(layers());
  return layers_.back();
}

///
/// Resets layer `l` of the population.
///
/// \param[in] l a layer of the population
///
/// \warning If layer `l` is nonexistent/empty the method doesn't work!
///
template<Individual I>
void layered_population<I>::init(layer_t &l)
{
  l.clear();
  l.allowed(problem().env.population.individuals);

  std::generate_n(std::back_inserter(l), l.allowed(),
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
std::size_t layered_population<I>::layers() const
{
  return layers_.size();
}

///
/// \return the number of individuals in the population
///
template<Individual I>
std::size_t layered_population<I>::size() const
{
  using ret_t = decltype(size());

  return std::accumulate(layers_.begin(), layers_.end(), ret_t(0),
                         [](auto accumulator, const auto &l)
                         {
                           return accumulator
                                  + static_cast<ret_t>(l.size());
                         });
}

///
/// \return a constant reference to the active problem
///
template<Individual I>
const ultra::problem &layered_population<I>::problem() const
{
  Expects(prob_);
  return *prob_;
}

///
/// Adds a new layer to the population.
///
/// The new layer is inserted as the lower layer and randomly initialized.
///
template<Individual I>
void layered_population<I>::add_layer()
{
#if !defined(NDEBUG)
  const auto nl(layers());
#endif

  layers_.insert(layers_.begin(), layer_t());
  init(layer(0));

#if !defined(NDEBUG)
  Ensures(layers() == nl + 1);
#endif
}

template<Individual I>
void layered_population<I>::remove(layer_t &l)
{
  layers_.erase(std::next(layers_.begin(), get_index(l, layers_)));
}

///
/// \return a `const_iterator` pointing to the first individual of the
///         population
///
template<Individual I>
typename layered_population<I>::const_iterator
layered_population<I>::begin() const
{
  return const_iterator(*this, true);
}

///
/// \return an iterator pointing one element past the last individual of the
///         population
///
template<Individual I>
typename layered_population<I>::const_iterator
layered_population<I>::end() const
{
  return const_iterator(*this, false);
}

///
/// \return a `iterator` pointing to the first individual of the population
///
template<Individual I>
typename layered_population<I>::iterator layered_population<I>::begin()
{
  return iterator(*this, true);
}

///
/// \return an iterator pointing one element past the last individual of the
///         population
///
template<Individual I>
typename layered_population<I>::iterator layered_population<I>::end()
{
  return iterator(*this, false);
}

///
/// Increments the age of each individual of the population
///
template<Individual I>
void layered_population<I>::inc_age()
{
  std::ranges::for_each(layers_, [](auto &l) { l.inc_age(); });
}

namespace random
{

///
/// \return the coordinates of a random individual of the population
///
/// \related
/// population
///
template<LayeredPopulation P>
typename std::size_t layer(const P &p)
{
  const auto n_layers(p.layers());

  // With multiple layers we cannot be sure that every layer has the same
  // number of individuals. So the simple (and fast) solution:
  //
  //     return random::sup(n_layers);
  //
  // isn't appropriate.

  std::vector<std::size_t> s(n_layers);
  for (std::size_t i(0); i < n_layers; ++i)
    s[i] = p.layer(i).size();

  std::discrete_distribution<std::size_t> dd(s.begin(), s.end());
  return dd(random::engine);
}

}  // namespace random

///
/// \param[in] in input stream
/// \return       `true` if population has been correctly loaded
///
/// \note
/// The current population isn't changed if the load operation fails.
///
template<Individual I>
bool layered_population<I>::load(std::istream &in)
{
  std::size_t n_layers;
  if (!(in >> n_layers) || !n_layers)
    return false;

  layered_population p(*prob_);
  p.layers_.reserve(n_layers);

  for (std::size_t l(0); l < n_layers; ++l)
    if (!p.layer(l).load(in, prob_->sset))
      return false;

  *this = p;
  return true;
}

///
/// \param[out] out output stream
/// \return         `true` if population has been correctly saved
///
template<Individual I>
bool layered_population<I>::save(std::ostream &out) const
{
  const auto n(layers());

  out << n << '\n';

  for (const auto &l : layers_)
    if (!l.save(out))
      return false;

  return out.good();
}

///
/// \return `true` if the object passes the internal consistency check
///
template<Individual I>
bool layered_population<I>::is_valid() const
{
  for (const auto &l : layers_)
    if (!l.is_valid())
      return false;

  if (!prob_)
  {
    ultraERROR << "Undefined `problem`";
    return false;
  }

  return true;
}

///
/// Creates a random population where each individual has a different age.
///
/// \param[in] prob current problem
///
/// This function is useful for debug purpose since allows to easily
/// distinguish among members.
///
/// \note
/// The `==` operator doesn't compare the age, explicit check must be performed
/// by the user.
///
template<Individual I>
layered_population<I> make_debug_population(const ultra::problem &prob)
{
  layered_population<I> pop(prob);

  unsigned inc(0);
  for (auto &prg : pop)
    prg.inc_age(inc++);

  return pop;
}

#endif  // include guard