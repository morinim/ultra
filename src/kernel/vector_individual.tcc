/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_VECTOR_INDIVIDUAL_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_VECTOR_INDIVIDUAL_TCC)
#define      ULTRA_VECTOR_INDIVIDUAL_TCC

///
/// Allocates a genome for initialisation by a derived constructor.
///
/// \param[in] n number of genes
///
/// \warning
/// The derived constructor must initialise the signature.
///
template<class T>
vector_individual<T>::vector_individual(std::size_t n) : genome_(n)
{
  Expects(parameters());
}

///
/// Constructs a random scalar genome and computes its signature.
///
/// \param[in] p current problem
///
/// \note
/// `T` must be `D_INT` or `D_DOUBLE`.
///
template<class T>
vector_individual<T>::vector_individual(const ultra::problem &p)
  : vector_individual(p.sset.categories())
{
  static_assert(std::same_as<T, D_INT> || std::same_as<T, D_DOUBLE>);

  std::ranges::generate(
    genome_,
    [&, n = 0]() mutable
    {
      return std::get<T>(p.sset.roulette_terminal(n++));
    });

  signature_ = hash();
  Ensures(is_valid());
}

///
/// Returns a const iterator to the first gene.
///
template<class T>
typename vector_individual<T>::const_iterator
vector_individual<T>::begin() const noexcept
{
  return genome_.begin();
}

///
/// Returns a const iterator past the last gene.
///
template<class T>
typename vector_individual<T>::const_iterator
vector_individual<T>::end() const noexcept
{
  return genome_.end();
}

///
/// Returns the gene at the specified index.
///
/// \param[in] i index of the gene
///
template<class T>
const typename vector_individual<T>::value_type &
vector_individual<T>::operator[](std::size_t i) const
{
  Expects(i < parameters());
  return genome_[i];
}

///
/// Provides read-only access to the genome vector.
///
template<class T>
vector_individual<T>::operator const std::vector<T> &() const noexcept
{
  return genome_;
}

///
/// Applies an operation to a range of genes and updates the signature.
///
/// \param first first index (inclusive)
/// \param last one past the last index (exclusive)
/// \param f mutating callable returning `void`
/// \pre `first <= last <= parameters()`; no concurrent access
///
template<class T>
template<class F>
requires std::invocable<F &, T &>
         && std::same_as<std::invoke_result_t<F &, T &>, void>
void vector_individual<T>::apply_each(std::size_t first, std::size_t last, F &&f)
{
  Expects(first <= last);
  Expects(last <= parameters());

  for (auto i(first); i < last; ++i)
    std::invoke(std::forward<F>(f), genome_[i]);

  signature_ = hash();
  Ensures(is_valid());
}

///
/// Applies an operation to every gene and updates the signature.
///
/// \param f mutating callable returning `void`
/// \pre no concurrent access
///
template<class T>
template<class F>
requires std::invocable<F &, T &>
         && std::same_as<std::invoke_result_t<F &, T &>, void>
void vector_individual<T>::apply_each(F &&f)
{
  apply_each(0, size(), std::forward<F>(f));
}

///
/// Replaces the genome and updates its signature.
///
/// \param[in] v replacement genes
/// \return      a reference to `*this`
///
template<class T>
vector_individual<T> &
vector_individual<T>::operator=(const std::vector<T> &v)
{
  Expects(empty() || v.size() == size());

  genome_ = v;
  signature_ = hash();

  Ensures(is_valid());
  return *this;
}

///
/// Checks whether the genome is empty.
///
template<class T>
bool vector_individual<T>::empty() const noexcept
{
  return genome_.empty();
}

///
/// Returns the number of parameters, equivalent to `size()`.
///
template<class T>
std::size_t vector_individual<T>::parameters() const noexcept
{
  return size();
}

///
/// Returns the number of genes.
///
template<class T>
std::size_t vector_individual<T>::size() const noexcept
{
  return genome_.size();
}

///
/// Hashes the raw genome bytes with MurmurHash3.
///
/// \return the genome hash, or an empty hash for an empty genome
///
template<class T>
hash_t vector_individual<T>::hash() const
{
  const auto len(genome_.size() * sizeof(genome_[0]));  // length in bytes
  return len ? ultra::hash::hash128(genome_.data(), len) : hash_t();
}

///
/// Compares genomes, ignoring age.
///
/// \param[in] lhs first individual
/// \param[in] rhs second individual
///
/// \related vector_individual
///
template<class T>
bool operator==(const vector_individual<T> &lhs,
                const vector_individual<T> &rhs) noexcept
{
  return std::ranges::equal(lhs, rhs);
}

///
/// Returns the number of parameters in the individual.
///
/// \param[in] ind individual to inspect
///
/// \related vector_individual
///
template<class T>
std::size_t active_slots(const vector_individual<T> &ind) noexcept
{
  return ind.parameters();
}

///
/// \return `true` if the individual passes the internal consistency check
///
template<class T>
bool vector_individual<T>::is_valid() const
{
  const auto expected(hash());
  if (signature() == expected)
    return true;

  ultraERROR("Wrong signature: {} should be {}",
             internal::streamed(signature()),
             internal::streamed(expected));
  return false;
}

///
/// Loads a genome, leaving it unchanged if parsing fails.
///
/// \param[in] in input stream
/// \param[in] ss symbol set used to decode variant genes
/// \return       `true` if all genes were loaded successfully
///
template<class T>
bool vector_individual<T>::load_impl(std::istream &in, const symbol_set &ss)
{
  std::size_t sz;
  if (!(in >> sz))
    return false;

  genome_t v(sz);
  for (auto &g : v)
  {
    if constexpr (std::same_as<T, D_DOUBLE>)
    {
      if (!load_float_from_stream(in, &g))
        return false;
    }
    else if constexpr (std::same_as<T, value_t>)
    {
      if (!ultra::load(in, ss, g))
        return false;
    }
    else if (!(in >> g))
      return false;
  }

  genome_ = std::move(v);
  return true;
}

///
/// Saves the genome using its gene type's encoding.
///
/// \param[out] out output stream
/// \return         `true` if the stream remains valid
///
template<class T>
bool vector_individual<T>::save_impl(std::ostream &out) const
{
  out << parameters() << '\n';
  for (const auto &g : genome_)
  {
    if constexpr (std::same_as<T, D_DOUBLE>)
      save_float_to_stream(out, g);
    else if constexpr (std::same_as<T, value_t>)
      ultra::save(out, g);
    else
      out << g;

    out << '\n';
  }

  return out.good();
}

#endif  // include guard
