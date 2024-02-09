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
/// \param[in] p           current problem
/// \param[in] init_layers builds `p.env.population.layers` layers
///
template<Individual I>
layered_population<I>::layered_population(const ultra::problem &p,
                                          bool init_layers)
  : prob_(&p)
{
  if (init_layers)
  {
    for (std::size_t l(0); l < p.env.population.init_layers; ++l)
      init(layers_.insert(layers_.end(), layer_t()));
//emplace_back
    assert(layers() == p.env.population.init_layers);
  }

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
  return *std::next(layers_.begin(), l);
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
  return *std::next(layers_.begin(), l);
}

///
/// \return const reference to the first layer of the population
///
template<Individual I>
const typename layered_population<I>::layer_t &
layered_population<I>::front() const
{
  Expects(layers());
  return layers_.front();
}

///
/// \return reference to the first layer of the population
///
template<Individual I>
typename layered_population<I>::layer_t &layered_population<I>::front()
{
  Expects(layers());
  return layers_.front();
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

template<Individual I>
basic_range<typename layered_population<I>::layer_const_iter>
layered_population<I>::range_of_layers() const
{
  return {layers_.begin(), layers_.end()};
}

template<Individual I>
basic_range<typename layered_population<I>::layer_iter>
layered_population<I>::range_of_layers()
{
  return {layers_.begin(), layers_.end()};
}

///
/// Resets a layer of the population.
///
/// \param[in] l a layer of the population
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
/// Resets a layer of the population.
///
/// \param[in] l an iterator referring to a layer of the population
///
/// \warning
/// If layer `l` is nonexistent the method doesn't work!
///
template<Individual I>
void layered_population<I>::init(layer_iter l)
{
  if (l == layers_.end())
    return;

  assert(std::ranges::find_if(layers_,
                              [&l](const auto &elem)
                              {
                                return std::addressof(elem)
                                       == std::addressof(*l);
                              }) != layers_.end());

  l->clear();
  l->allowed(problem().env.population.individuals);

  std::generate_n(std::back_inserter(*l), l->allowed(),
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

  init(layers_.insert(layers_.begin(), layer_t()));

#if !defined(NDEBUG)
  Ensures(layers() == nl + 1);
#endif
}

///
/// Erases the specified element from the container.
///
/// \param[in] l reference to the element to remove
/// \return    `true` if an element has been removed; `false` otherwise
///
template<Individual I>
bool layered_population<I>::erase(layer_t &l)
{
  for (auto it(layers_.begin()); it != layers_.end(); ++it)
    if (std::addressof(l) == std::addressof(*it))
    {
      layers_.erase(it);
      return true;
    }

  return false;
}

///
/// Erases the specified element from the container.
///
/// \param[in] pos iterator to the element to remove
/// \return        iterator following the last removed element. If `pos` refers
///                to the last element, then the `end()` iterator is returned
///
/// References and iterators to the erased element are invalidated. Other
/// references and iterators are not affected.
///
template<Individual I>
typename layered_population<I>::layer_iter
layered_population<I>::erase(layer_iter pos)
{
  Expects(iterator_of(pos, range_of_layers()));
  return layers_.erase(pos);
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

  layered_population<I> lp(*prob_, false);

  for (std::size_t l(0); l < n_layers; ++l)
  {
    if (layer_t tmp; tmp.load(in, prob_->sset))
      lp.layers_.push_back(std::move(tmp));
    else
      return false;
  }

  *this = lp;
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
