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
const typename population<I>::layer_t &population<I>::layer(std::size_t l) const
{
  Expects(l < layers());
  return layers_[l];
}

///
/// \param[in] l layer index
/// \return      reference to a layer of the population
///
template<Individual I>
typename population<I>::layer_t &population<I>::layer(std::size_t l)
{
  Expects(l < layers());
  return layers_[l];
}

///
/// \return const reference to the first layer of the population
///
template<Individual I>
const typename population<I>::layer_t &population<I>::front() const
{
  Expects(layers());
  return layer(0);
}

///
/// \return reference to the first layer of the population
///
template<Individual I>
typename population<I>::layer_t &population<I>::front()
{
  Expects(layers());
  return layer(0);
}

///
/// \return const reference to the last layer of the population
///
template<Individual I>
const typename population<I>::layer_t &population<I>::back() const
{
  Expects(layers());
  return layer(layers() - 1);
}

///
/// \return reference to the last layer of the population
///
template<Individual I>
typename population<I>::layer_t &population<I>::back()
{
  Expects(layers());
  return layer(layers() - 1);
}

///
/// \param[in] i index of an individual
/// \return      a reference to the individual at index `i`
///
template<Individual I>
I &population<I>::layer_t::operator[](std::size_t i)
{
  Expects(i < size());
  return members_[i];
}

///
/// \param[in] i index of an individual
/// \return      a constant reference to the individual at index `i`
///
template<Individual I>
const I &population<I>::layer_t::operator[](std::size_t i) const
{
  Expects(i < size());
  return members_[i];
}

///
/// \return number of individuals in this layer
///
template<Individual I>
std::size_t population<I>::layer_t::size() const
{
  return members_.size();
}

///
/// \return reference max age for current layer
///
template<Individual I>
unsigned population<I>::layer_t::max_age() const
{
  return max_age_;
}

template<Individual I>
std::shared_mutex &population<I>::layer_t::mutex() const
{
  return *pmutex_;
}

///
/// \param[in] m set the reference max age for the current layer
///
template<Individual I>
void population<I>::layer_t::max_age(unsigned m)
{
  max_age_ = m;
}

///
/// \return number of individuals allowed in this layer
///
/// \note
/// `size() < allowed()`
///
template<Individual I>
std::size_t population<I>::layer_t::allowed() const
{
  return allowed_;
}

///
/// \param[in] n number of individuals allowed in this layer
///
template<Individual I>
void population<I>::layer_t::allowed(std::size_t n)
{
  members_.reserve(n);
  allowed_ = n;
}

///
/// \return `true` if the layer is empty
///
/// \remark
/// This function does not modify the layer in any way. To clear the content of
/// a layer, see `layer_t::clear`.
///
template<Individual I>
bool population<I>::layer_t::empty() const
{
  return members_.empty();
}

///
/// Removes all elements from the layer.
///
template<Individual I>
void population<I>::layer_t::clear()
{
  members_.clear();
}

///
/// Adds individual `i` to this layer.
///
/// \param[in] i an individual
///
template<Individual I>
void population<I>::layer_t::push_back(const I &i)
{
  if (size() < allowed())
    members_.push_back(i);
}

///
/// Removes the last individual of this layer.
///
template<Individual I>
void population<I>::layer_t::pop_back()
{
  members_.pop_back();
}

///
/// \return a const iterator to the beginning of the given range
///
template<Individual I>
typename population<I>::layer_t::const_iterator
population<I>::layer_t::begin() const
{
  return members_.begin();
}

///
/// \return an iterator to the beginning of the given range
///
template<Individual I>
typename population<I>::layer_t::iterator population<I>::layer_t::begin()
{
  return members_.begin();
}

///
/// \return a const iterator to the end of the given range
///
template<Individual I>
typename population<I>::layer_t::const_iterator
population<I>::layer_t::end() const
{
  return members_.end();
}

///
/// Resets layer `l` of the population.
///
/// \param[in] l a layer of the population
///
/// \warning If layer `l` is nonexistent/empty the method doesn't work!
///
template<Individual I>
void population<I>::init(layer_t &l)
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
std::size_t population<I>::layers() const
{
  return layers_.size();
}

///
/// \return the number of individuals in the population
///
template<Individual I>
std::size_t population<I>::size() const
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
const ultra::problem &population<I>::problem() const
{
  Expects(prob_);
  return *prob_;
}

///
/// \param[in] c coordinates of an individual
/// \return      a reference to the individual at coordinates `c`
///
template<Individual I>
I &population<I>::operator[](const coord &c)
{
  Expects(c.layer < layers());
  Expects(c.index < layer(c.layer).size());
  return layers_[c.layer][c.index];
}

///
/// \param[in] c coordinates of an individual
/// \return      a constant reference to the individual at coordinates `c`
///
template<Individual I>
const I &population<I>::operator[](const coord &c) const
{
  Expects(c.layer < layers());
  Expects(c.index < layer(c.layer).size());
  return layers_[c.layer][c.index];
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

  layers_.insert(layers_.begin(), layer_t());
  init(layer(0));

#if !defined(NDEBUG)
  Ensures(layers() == nl + 1);
#endif
}

template<Individual I>
void population<I>::remove(layer_t &l)
{
  layers_.erase(std::next(layers_.begin(), get_index(l, layers_)));
}

///
/// \return a `const_iterator` pointing to the first individual of the
///         population
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
/// \return a `iterator` pointing to the first individual of the population
///
template<Individual I>
typename population<I>::iterator population<I>::begin()
{
  return iterator(*this, true);
}

///
/// \return an iterator pointing one element past the last individual of the
///         population
///
template<Individual I>
typename population<I>::iterator population<I>::end()
{
  return iterator(*this, false);
}

///
/// Increments the age of each individual of the population
///
template<Individual I>
void population<I>::inc_age()
{
  std::ranges::for_each(*this, [](auto &i) { i.inc_age(); });
}

namespace random
{

template<Population P>
std::size_t coord(const P &l)
{
  return sup(l.size());
}

///
/// \return the coordinates of a random individual of the population
///
/// \related
/// population
///
template<Individual I>
typename population<I>::coord coord(const population<I> &p)
{
  const auto n_layers(p.layers());

  if (n_layers == 1)
    return {0, random::coord(p.layer(0))};

  // With multiple layers we cannot be sure that every layer has the same
  // number of individuals. So the simple (and fast) solution:
  //
  //     const auto l(random::sup(n_layers));
  //     return p[{l, random::sup(p.layer(l).size())}];
  //
  // isn't appropriate.

  std::vector<std::size_t> s(n_layers);
  for (std::size_t i(0); i < n_layers; ++i)
    s[i] = p.layer(i).size();

  std::discrete_distribution<std::size_t> dd(s.begin(), s.end());
  const auto l(dd(random::engine));
  return {l, random::coord(p.layer(l))};
}

template<Population P>
std::size_t coord(const P &l, std::size_t i, std::size_t mate_zone)
{
  return random::ring(i, mate_zone, l.size());
}

///
/// \param[in] p         a population
/// \param[in] target    coordinates of a reference individual
/// \param[in] mate_zone restricts the selection of individuals to a maximum
///                      radius
/// \return              the coordinates of a random individual in the
///                      `mate_zone` of `target`
///
/// \related
/// population
///
template<Individual I>
typename population<I>::coord coord(const population<I> &p,
                                    typename population<I>::coord target,
                                    std::size_t mate_zone)
{
  return {target.layer,
          random::coord(p.layer(target.layer), target.index, mate_zone)};
}

///
/// \param[in] p a population
/// \return      a random individual of the population
///
template<PopulationWithMutex P>
typename P::value_type individual(const P &p)
{
  std::shared_lock lock(p.mutex());
  return p[random::coord(p)];
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
bool population<I>::load(std::istream &in)
{
  std::size_t n_layers;
  if (!(in >> n_layers) || !n_layers)
    return false;

  population p(*prob_);
  p.layers_.reserve(n_layers);

  for (std::size_t l(0); l < n_layers; ++l)
  {
    std::size_t allowed;
    if (!(in >> allowed))
      return false;

    p.layer(l).allowed(allowed);

    std::size_t n_elem;
    if (!(in >> n_elem))
      return false;

    if (allowed < n_elem)
      return false;

    for (std::size_t i(0); i < n_elem; ++i)
      if (!p[{l, i}].load(in, prob_->sset))
        return false;
  }

  *this = p;
  return true;
}

///
/// \param[out] out output stream
/// \return         `true` if population has been correctly saved
///
template<Individual I>
bool population<I>::save(std::ostream &out) const
{
  const auto n(layers());

  out << n << '\n';

  for (const auto &l : layers_)
  {
    out << l.allowed() << ' ' << l.size() << '\n';

    for (const auto &prg : l)
      if (!prg.save(out))
        return false;
  }

  return out.good();
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

  for (const auto &l : layers_)
    if (l.allowed() < l.size())
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
population<I> make_debug_population(const ultra::problem &prob)
{
  population<I> pop(prob);

  unsigned inc(0);
  for (auto &prg : pop)
    prg.inc_age(inc++);

  return pop;
}

#endif  // include guard
