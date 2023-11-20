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
    layers_[l].allowed(n);
    init(layers_[l]);
  }

  Ensures(layers() == p.env.population.layers);
  Ensures(is_valid());
}

template<Individual I>
const typename population<I>::layer_t &population<I>::layer(std::size_t l) const
{
  Expects(l < layers());
  return layers_[l];
}

template<Individual I>
typename population<I>::layer_t &population<I>::layer(std::size_t l)
{
  Expects(l < layers());
  return layers_[l];
}

///
/// \return number of individuals in this layer
///
template<Individual I>
std::size_t population<I>::layer_t::size() const
{
  return members.size();
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
/// \return `true` if the layer is empty
///
/// \remark
/// This function does not modify the layer in any way. To clear the content of
/// a layer, see `layer_t::clear`.
///
template<Individual I>
bool population<I>::layer_t::empty() const
{
  return members.empty();
}

///
/// Removes all elements from the layer.
///
template<Individual I>
void population<I>::layer_t::clear()
{
  members.clear();
}

///
/// \param[in] n number of individuals allowed in this layer
///
template<Individual I>
void population<I>::layer_t::allowed(std::size_t n)
{
  allowed_ = n;
  Ensures(size() <= allowed());
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
    members.push_back(i);
}

///
/// Removes the last individual of this layer.
///
template<Individual I>
void population<I>::layer_t::pop_back()
{
  members.pop_back();
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

  std::generate_n(std::back_inserter(l.members), l.allowed(),
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
  Expects(c.index < layer(c.layer).size());
  return layers_[c.layer].members[c.index];
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
  layer(0).allowed(problem().env.population.individuals);
  layers_[0].members.reserve(layer(0).allowed());

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

    p.layers_[l].allowed(allowed);

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

    for (const auto &prg : l.members)
      prg.save(out);
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

  const auto n(layers());
  for (std::size_t l(0); l < n; ++l)
  {
    if (layer(l).allowed() < layer(l).size())
      return false;

    if (layers_[l].members.capacity() < layer(l).allowed())
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
