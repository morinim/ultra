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
/// Constructs a random population for a given problem.
///
/// \param[in] p the problem definition
///
/// The population size is initialised according to the problem parameters.
/// Individuals are created using `I(p)`.
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
/// Clears the population and creates a new random one.
///
/// \param[in] p the problem definition
///
/// \pre allowed() >= min_allowed_
///
/// \post size() == allowed()
///
/// Individuals are constructed using the provided problem definition.
/// The number of created individuals equals `allowed()`.
///
template<Individual I>
void linear_population<I>::reset(const ultra::problem &p)
{
  Expects(allowed() >= min_allowed_);

  members_.clear();
  std::generate_n(std::back_inserter(members_), allowed(),
                  [&p] { return I(p); });
}

///
/// Access an individual by index.
///
/// \param[in] i index of the individual
/// \return      a reference to the individual at position `i`
///
template<Individual I>
I &linear_population<I>::operator[](std::size_t i)
{
  Expects(i < size());
  return members_[i];
}

///
/// Access an individual by index (const).
///
/// \param[in] i index of the individual
/// \return      a reference to the individual at position `i`
///
template<Individual I>
const I &linear_population<I>::operator[](std::size_t i) const
{
  Expects(i < size());
  return members_[i];
}

///
/// \return number of individuals in this population
///
template<Individual I>
std::size_t linear_population<I>::size() const noexcept
{
  return members_.size();
}

///
/// \return number of individuals in this population
///
/// \remark
/// Thread safe version of size().
///
template<Individual I>
std::size_t linear_population<I>::safe_size() const
{
  std::lock_guard lock(mutex_);
  return members_.size();
}

///
/// \return the maximum allowed age for individuals
///
template<Individual I>
unsigned linear_population<I>::max_age() const noexcept
{
  return max_age_;
}

///
/// Sets the reference maximum age.
///
/// \param[in] m the new maximum age
///
template<Individual I>
void linear_population<I>::max_age(unsigned m) noexcept
{
  max_age_ = m;
}

///
/// \return the mutex protecting the population
///
/// This mutex must be used by clients to synchronise concurrent access to the
/// population.
///
template<Individual I>
std::shared_mutex &linear_population<I>::mutex() const
{
  return mutex_;
}

///
/// \return the maximum number of allowed individuals
///
/// \note
/// `size() <= allowed()`
///
template<Individual I>
std::size_t linear_population<I>::allowed() const noexcept
{
  return allowed_;
}

///
/// Sets the maximum number of allowed individuals.
///
/// \param[in] n the new maximum population size
///
/// If the population size exceeds the new limit, surplus individuals are
/// removed from the end of the sequence.
///
/// The value is clamped so that it's never less than the minimum allowed
/// population size.
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
/// \return `true` if the population is empty
///
template<Individual I>
bool linear_population<I>::empty() const noexcept
{
  return members_.empty();
}

///
/// Removes all individuals from the population.
///
/// After this call `size() == 0`.
///
template<Individual I>
void linear_population<I>::clear() noexcept
{
  members_.clear();
}

///
/// Adds an individual to the population.
///
/// \param[in] i the individual to add
///
/// The individual is added only if the population size is strictly less than
/// the allowed maximum.
///
template<Individual I>
void linear_population<I>::push_back(const I &i)
{
  if (size() < allowed())
    members_.push_back(i);
}

///
/// Removes the last individual from the population.
///
/// \pre empty() == false
///
template<Individual I>
void linear_population<I>::pop_back()
{
  Expects(!empty());
  members_.pop_back();
}

///
/// \return a const iterator to the first individual
///
template<Individual I>
typename linear_population<I>::const_iterator
linear_population<I>::begin() const noexcept
{
  return members_.begin();
}

///
/// \return an iterator to the first individual
///
template<Individual I>
typename linear_population<I>::iterator linear_population<I>::begin() noexcept
{
  return members_.begin();
}

///
/// \return a const iterator past the last individual.
///
template<Individual I>
typename linear_population<I>::const_iterator
linear_population<I>::end() const noexcept
{
  return members_.end();
}

///
/// \return an iterator past the last individual
///
template<Individual I>
typename linear_population<I>::iterator linear_population<I>::end() noexcept
{
  return members_.end();
}

///
/// Increments the age of all individuals.
///
template<Individual I>
void linear_population<I>::inc_age()
{
  std::ranges::for_each(*this, [](auto &i) { i.inc_age(); });
}

///
/// Loads the population from a stream.
///
/// \param[in] in input stream
/// \param[in] ss symbol_set used to rebuild individuals
/// \return       `true` on success, `false` otherwise
///
/// \note On failure, the population remains unchanged.
///
template<Individual I>
bool linear_population<I>::load(std::istream &in, const symbol_set &ss)
{
  unsigned tmp_max_age;
  if (!(in >> tmp_max_age))
    return false;

  std::size_t tmp_min_allowed;
  if (!(in >> tmp_min_allowed))
    return false;

  std::size_t tmp_allowed;
  if (!(in >> tmp_allowed))
    return false;

  if (tmp_allowed < tmp_min_allowed)
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
  members_ = std::move(tmp_members);
  min_allowed_ = tmp_min_allowed;
  allowed(tmp_allowed);

  Ensures(is_valid());
  return true;
}

///
/// Saves the population to a stream.
///
/// \param[out] out output stream
/// \return         `true` on success, `false` otherwise
///
template<Individual I>
bool linear_population<I>::save(std::ostream &out) const
{
  out << max_age() << ' ' << min_allowed_ << ' ' << allowed() << ' ' << size()
      << '\n';

  for (const auto &prg : *this)
    if (!prg.save(out))
      return false;

  return out.good();
}

///
/// \return a numerical unique identifier of this population
///
/// \note
/// The ID is unique within the current application instance.
///
template<Individual I>
population_uid linear_population<I>::uid() const noexcept
{
  return uid_;
}

///
/// Checks the internal consistency of the population.
///
/// \return `true` if the population is internally consistent
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
