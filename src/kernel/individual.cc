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

#include "kernel/individual.h"

#include <sstream>

namespace ultra
{

///
/// Returns the cached structural signature of the individual.
///
/// \return the signature of this individual
///
/// The signature is a hash representing the logical structure of the
/// individual. It must be maintained by the derived type whenever the
/// individual is mutated.
///
/// Signature maps syntactically distinct (but logically equivalent)
/// individuals to the same value.
///
/// In other words identical individuals at genotypic level have the same
/// signature; different individuals at the genotypic level may be mapped
/// to the same signature since real structure/computation is considered and
/// not the simple storage.
///
/// This is a very interesting property, useful for individual comparison,
/// information retrieval, entropy calculation...
///
/// \note
/// Concurrent calls to `signature()` on the same instance are safe, provided
/// the instance is not mutated concurrently.
///
hash_t individual::signature() const noexcept
{
  return signature_;
}

///
/// A measurement of the age of an individual (mainly used for ALPS).
///
/// \return the individual's age
///
/// This is a measure of how long an individual's family of genotypic
/// material has been in the population. Randomly generated individuals,
/// such as those that are created when search starts, have an age of `0`. Each
/// generation that an individual stays in the population (such as through
/// elitism) its age is increased by `1`.
/// **Individuals that are created through mutation or recombination take the
/// age of their oldest parent**.
///
/// \note
/// This differs from conventional measures of age, in which individuals
/// created through applying some type of variation to an existing
/// individual (e.g. mutation or recombination) start with an age of `0`.
individual::age_t individual::age() const noexcept
{
  return age_;
}

///
/// Increments the individual's age.
///
/// \param[in] delta increment
///
void individual::inc_age(unsigned delta) noexcept
{
  age_ += delta;
}

///
/// \param[in] ss active symbol set
/// \param[in] in input stream
/// \return       `true` if the object has been loaded correctly
///
/// \note
/// If the load operation isn't successful the object isn't modified.
///
bool individual::load(std::istream &in, const symbol_set &ss)
{
  individual::age_t t_age;
  if (!(in >> t_age))
    return false;

  if (!load_impl(in, ss))
    return false;

  age_ = t_age;
  signature_ = hash();

  return true;
}

///
/// \param[out] out output stream
/// \return         `true` if the object has been saved correctly
///
bool individual::save(std::ostream &out) const
{
  out << age() << '\n';
  // We don't save/load signature: it can be easily calculated on the fly.

  return save_impl(out);
}

///
/// Updates the age of this individual if it's smaller than `rhs_age`.
///
/// \param[in] rhs_age the age of an individual
///
void individual::set_if_older_age(individual::age_t rhs_age) noexcept
{
  if (age() < rhs_age)
    age_ = rhs_age;
}

std::string individual::format(out::print_format_t fmt) const
{
  std::ostringstream oss;
  print_impl(oss, fmt);
  return oss.str();
}

}  // namespace ultra
