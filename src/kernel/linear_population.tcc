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

#if !defined(ULTRA_LINEAR_POPULATION_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_LINEAR_POPULATION_TCC)
#define      ULTRA_LINEAR_POPULATION_TCC

///
/// Creates a random population.
///
/// \param[in] p current problem
///
template<Individual I>
linear_population<I>::linear_population(const ultra::problem &p)
  : allowed_(std::max(p.params.population.individuals,
                      p.params.population.min_individuals)),
    min_allowed_(p.params.population.min_individuals)
{
  reset(p);
}

///
/// Clear the current population and creates a new random one.
///
/// \param[in] p current problem
///
template<Individual I>
void linear_population<I>::reset(const ultra::problem &p)
{
  members_.clear();
  std::generate_n(std::back_inserter(members_), allowed(),
                  [&p] { return I(p); });
}

///
/// \param[in] i index of an individual
/// \return      a reference to the individual at index `i`
///
template<Individual I>
I &linear_population<I>::operator[](std::size_t i)
{
  Expects(i < size());
  return members_[i];
}

///
/// \param[in] i index of an individual
/// \return      a constant reference to the individual at index `i`
///
template<Individual I>
const I &linear_population<I>::operator[](std::size_t i) const
{
  Expects(i < size());
  return members_[i];
}

///
/// \return number of individuals in this layer
///
template<Individual I>
std::size_t linear_population<I>::size() const noexcept
{
  return members_.size();
}

///
/// \return number of individuals in this layer
///
/// \remark
/// Thread safe version fo size().
///
template<Individual I>
std::size_t linear_population<I>::safe_size() const
{
  std::lock_guard lock(mutex_);
  return members_.size();
}

///
/// \return reference max age for current layer
///
template<Individual I>
unsigned linear_population<I>::max_age() const noexcept
{
  return max_age_;
}

///
/// \param[in] m set the reference max age for the current layer
///
template<Individual I>
void linear_population<I>::max_age(unsigned m) noexcept
{
  max_age_ = m;
}

template<Individual I>
std::shared_mutex &linear_population<I>::mutex() const
{
  return mutex_;
}

///
/// \return number of individuals allowed in this layer
///
/// \note
/// `size() <= allowed()`
///
template<Individual I>
std::size_t linear_population<I>::allowed() const
{
  return allowed_;
}

///
/// Sets the number of individuals allowed in this layer.
///
/// \param[in] n number of allowed individuals
///
/// If the layer contains more individuals than the amount allowed, the surplus
/// is erased.
///
template<Individual I>
void linear_population<I>::allowed(std::size_t n)
{
  // Don't drop under a predefined number of individuals.
  n = std::max(n, min_allowed_);

  if (size() > n)
  {
    const auto delta(size() - n);
    members_.erase(members_.end() - delta, members_.end());

    assert(size() == n);
  }

  allowed_ = n;

  Ensures(is_valid());
}

///
/// \return `true` if the layer is empty
///
/// \remark
/// This function does not modify the layer in any way. To clear the content of
/// a layer, see `layer_t::clear`.
///
template<Individual I>
bool linear_population<I>::empty() const noexcept
{
  return members_.empty();
}

///
/// Removes all elements from the layer.
///
template<Individual I>
void linear_population<I>::clear() noexcept
{
  members_.clear();
}

///
/// Adds individual `i` to this layer.
///
/// \param[in] i an individual
///
template<Individual I>
void linear_population<I>::push_back(const I &i)
{
  if (size() < allowed())
    members_.push_back(i);
}

///
/// Removes the last individual of this layer.
///
template<Individual I>
void linear_population<I>::pop_back()
{
  members_.pop_back();
}

///
/// \return a const iterator to the beginning of the given range
///
template<Individual I>
typename linear_population<I>::const_iterator
linear_population<I>::begin() const
{
  return members_.begin();
}

///
/// \return an iterator to the beginning of the given range
///
template<Individual I>
typename linear_population<I>::iterator linear_population<I>::begin()
{
  return members_.begin();
}

///
/// \return a const iterator to the end of the given range
///
template<Individual I>
typename linear_population<I>::const_iterator linear_population<I>::end() const
{
  return members_.end();
}

///
/// \return a iterator to the end of the given range
///
template<Individual I>
typename linear_population<I>::iterator linear_population<I>::end()
{
  return members_.end();
}

///
/// Increments the age of each individual of the population
///
template<Individual I>
void linear_population<I>::inc_age()
{
  std::ranges::for_each(*this, [](auto &i) { i.inc_age(); });
}

///
/// \param[in] in input stream
/// \param[in] ss symbol_set (for building of the individuals)
/// \return       `true` if population has been correctly loaded
///
/// \note
/// The current population isn't changed if the load operation fails.
///
template<Individual I>
bool linear_population<I>::load(std::istream &in, const symbol_set &ss)
{
  unsigned tmp_max_age;
  if (!(in >> tmp_max_age))
    return false;

  std::size_t tmp_allowed;
  if (!(in >> tmp_allowed))
    return false;

  std::size_t n_elem;
  if (!(in >> n_elem))
    return false;

  if (tmp_allowed < n_elem)
    return false;

  std::vector<I> tmp_members;
  tmp_members.reserve(n_elem);
  for (std::size_t i(0); i < n_elem; ++i)
    if (I ind; !ind.load(in, ss))
      return false;
    else
      tmp_members.push_back(ind);

  max_age(tmp_max_age);
  allowed(tmp_allowed);
  members_ = tmp_members;

  return true;
}

///
/// \param[out] out output stream
/// \return         `true` if population has been correctly saved
///
template<Individual I>
bool linear_population<I>::save(std::ostream &out) const
{
  out << max_age() << ' ' << allowed() << ' ' << size() << '\n';

  for (const auto &prg : *this)
    if (!prg.save(out))
      return false;

  return out.good();
}

///
/// \return a numerical unique ID of this population
///
/// \note
/// The ID is unique considering the current application instance.
///
template<Individual I>
population_uid linear_population<I>::uid() const noexcept
{
  return uid_;
}

///
/// \return `true` if the object passes the internal consistency check
///
template<Individual I>
bool linear_population<I>::is_valid() const
{
  if (!std::ranges::all_of(*this, [](const auto &i) { return i.is_valid(); }))
    return false;

  if (allowed() < min_allowed_)
    return false;

  return size() <= allowed();
}

#endif  // include guard
