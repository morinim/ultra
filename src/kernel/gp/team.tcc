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

#if !defined(ULTRA_TEAM_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_TEAM_TCC)
#define      ULTRA_TEAM_TCC

///
/// Allocates space for a given number of individuals.
///
template<Individual I>
team<I>::team(std::size_t n) : individuals_(n)
{
}

///
/// Creates a team of individuals that will cooperate to solve a task.
///
template<Individual I>
team<I>::team(const problem &p)
{
  Expects(p.params.team.individuals);

  auto n(p.params.team.individuals);
  individuals_.reserve(n);

  for (std::size_t i(0); i < n; ++i)
    individuals_.emplace_back(p);

  Ensures(is_valid());
}

///
/// Builds a team containing the individuals of a given vector.
///
/// \param[in] v a vector of individuals
///
template<Individual I>
team<I>::team(std::vector<I> v) : individuals_(std::move(v))
{
  Ensures(is_valid());
}

///
/// Mutates the individuals in `this` team and returns the number of mutations
/// performed.
///
/// \param[in] prb current problem
/// \return        number of mutations performed
///
/// \note
/// External parameters: `evolution.p_mutation`
///
template<Individual I>
unsigned team<I>::mutation(const problem &prb)
{
  //const auto nm(random::element(individuals_).mutation(prb));
  //if (nm)
  //  signature_.clear();
  //return nm;

  unsigned nm(0);
  for (auto &i : individuals_)
    nm += i.mutation(prb);

  if (nm)
    signature_.clear();

  return nm;
}

///
/// \param[in] lhs first parent
/// \param[in] rhs second parent
/// \return        the result of the crossover (we only generate a single
///                offspring)
///
/// \see
/// individual::crossover for further details.
///
/// \related
/// team
///
template<Individual I>
team<I> crossover(const team<I> &lhs, const team<I> &rhs)
{
  Expects(lhs.size() == rhs.size());

/*
  const auto j(random::sup(lhs.size()));

  auto ret(lhs);
  ret.individuals_[j] = crossover(lhs[j], rhs[j])
  ret.signature_.clear();
*/

  std::vector<I> crossed;
  crossed.reserve(lhs.size());

  std::ranges::transform(
    lhs, rhs, std::back_inserter(crossed),
    [](const auto &i1, const auto &i2) { return crossover(i1, i2); });

  return team(crossed);
}

///
/// \return an iterator pointing to the first individual of the team
///
template<Individual I>
typename team<I>::const_iterator team<I>::begin() const noexcept
{
  return individuals_.begin();
}

///
/// \return an iterator pointing to a end-of-team sentry
///
template<Individual I>
typename team<I>::const_iterator team<I>::end() const noexcept
{
  return individuals_.end();
}

///
/// \param[in] i index of a member of the team
/// \return      the `i`-th member of the team
///
template<Individual I>
const I &team<I>::operator[](std::size_t i) const
{
  Expects(i < size());
  return individuals_[i];
}

///
/// \return `true` if the team is empty, `false` otherwise
///
template<Individual I>
bool team<I>::empty() const noexcept
{
  return individuals_.empty();
}

///
/// \return number of individuals of the team
///
template<Individual I>
std::size_t team<I>::size() const noexcept
{
  return individuals_.size();
}

///
/// Total number of active functions.
///
/// \paramp[in] t team to be analyzed
/// \return       number of active functions in the team
///
/// \see
/// team::size()
///
/// \related
/// team
///
template<Individual I> unsigned active_slots(const team<I> &t)
{
  return std::accumulate(t.begin(), t.end(), 0,
                         [](unsigned n, const I &ind)
                         {
                           return n + active_slots(ind);
                         });
}

///
/// Signature maps syntactically distinct (but logically equivalent)
/// teams to the same value.
///
/// \return the signature of `this` team
///
/// In other words identical teams, at genotypic level, have the same
/// signature; different teams at the genotipic level may be mapped
/// to the same signature since the value of terminals is considered and not
/// the index.
///
/// This is a very interesting  property, useful for comparison, information
/// retrieval, entropy calculation...
///
template<Individual I>
hash_t team<I>::signature() const
{
  if (signature_.empty())
    signature_ = hash();

  return signature_;
}

///
/// \return the signature of `this` team
///
///
template<Individual I>
hash_t team<I>::hash() const
{
  hash_t ret;

  for (const auto &i : individuals_)
    ret.combine(i.signature());

  return ret;
}

///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparision
/// \return        `true` if the two teams are equal (individual by individual)
///
/// \note
/// Age is not checked.
///
/// \related
/// team
///
template<Individual I>
bool operator==(const team<I> &lhs, const team<I> &rhs)
{
  return std::ranges::equal(lhs, rhs);
}

///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparision
/// \return        `true` if the two teams aren't equal
///
/// \related
/// team
///
template<Individual I>
bool operator!=(const team<I> &lhs, const team<I> &rhs)
{
  return !(lhs == rhs);
}

///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparision
/// \return        a numeric measurement of the difference between `x` and
///                `this` (the number of different genes between teams)
///
/// \related
/// team
///
template<Individual I>
unsigned distance(const team<I> &lhs, const team<I> &rhs)
{
  Expects(lhs.size() == rhs.size());

  return std::inner_product(lhs.begin(), lhs.end(), rhs.begin(), 0u,
                            std::plus{},
                            [](const auto &i1, const auto &i2)
                            {
                              return distance(i1, i2);
                            });
}

///
/// \return the age of the team (average age of the team members)
///
template<Individual I>
ultra::individual::age_t team<I>::age() const
{
  const unsigned age_sum(std::accumulate(begin(), end(), 0u,
                                         [](ultra::individual::age_t sum,
                                            const I &i)
                                         {
                                           return sum + i.age();
                                         }));

  return age_sum / size();
}

///
/// Increments the age of every element of the team.
///
/// \param[in] delta variation of age
///
template<Individual I>
void team<I>::inc_age(unsigned delta)
{
  for (auto &i : individuals_)
    i.inc_age(delta);
}

///
/// \return `true` if the team passes the internal consistency check
///
template<Individual I>
bool team<I>::is_valid() const
{
  return signature_.empty() || signature_ == hash();
}

///
/// \param[in] ss active symbol set
/// \param[in] in input stream
/// \return       `true` if team was loaded correctly
///
/// \note
/// If the load operation isn't successful the current team isn't modified.
///
template<Individual I>
bool team<I>::load(std::istream &in, const symbol_set &ss)
{
  unsigned n;
  if (!(in >> n) || !n)
    return false;

  decltype(individuals_) v;
  v.reserve(n);

  for (unsigned j(0); j < n; ++j)
  {
    I i;
    if (!i.load(in, ss))
      return false;
    v.push_back(i);
  }

  individuals_ = v;

  // We don't save/load signature: it can be easily calculated on the fly.
  signature_.clear();

  return true;
}

///
/// \param[out] out output stream
/// \return         `true` if team was saved correctly
///
template<Individual I>
bool team<I>::save(std::ostream &out) const
{
  out << size() << '\n';
  if (!out.good())
    return false;

  if (std::ranges::any_of(*this,
                          [&](const I &i) { return !i.save(out); }))
    return false;

  return out.good();
}

///
/// \param[out] s output stream
/// \param[in]  t team to print
/// \return       output stream including `t`
///
/// \related
/// team
///
template<Individual I>
std::ostream &operator<<(std::ostream &s, const team<I> &t)
{
  const auto format(out::print_format_flag(s));

  for (const auto &i : t)
  {
    if (format == out::in_line_f)
      s << '{';

    s << i;

    if (format == out::in_line_f)
      s << '}';
    else
      s << '\n';
  }

  return s;
}

#endif  // include guard
