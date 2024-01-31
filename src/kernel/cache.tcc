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

#if !defined(ULTRA_CACHE_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_CACHE_TCC)
#define      ULTRA_CACHE_TCC

///
/// Creates a new hash table.
///
/// \param[in] bits `2^bits` is the number of elements of the table
///
template<Fitness F>
cache<F>::cache(unsigned bits) : k_mask((1ull << bits) - 1),
                                 table_(1ull << bits)
{
  Expects(bits);
  Ensures(is_valid());
}

///
/// \param[in] h the signature of an individual
/// \return      an index in the hash table
///
template<Fitness F>
inline std::size_t cache<F>::index(const hash_t &h) const
{
  return h.data[0] & k_mask;
}

///
/// Clears the content and the statistical informations of the table.
///
/// \note Allocated size isn't changed.
///
template<Fitness F>
void cache<F>::clear()
{
  std::lock_guard lock(mutex_);
  ++seal_;
}

///
/// Clears the cached information for a specific individual.
///
/// \param[in] h individual's signature whose informations we have to clear
///
template<Fitness F>
void cache<F>::clear(const hash_t &h)
{
  std::lock_guard lock(mutex_);

  // Invalidates the slot since the first valid value for seal is `1`.
  table_[index(h)].seal = 0;
}

///
/// Looks for the fitness of an individual in the transposition table.
///
/// \param[in] h individual's signature to look for
/// \return      the fitness of the individual. If the individuals isn't
///              present returns `nullptr`
///
template<Fitness F>
const F *cache<F>::find(const hash_t &h) const
{
  std::shared_lock lock(mutex_);

  const slot &s(table_[index(h)]);
  return s.seal == seal_ && s.hash == h ? &s.fitness : nullptr;
}

///
/// Stores fitness information in the transposition table.
///
/// \param[in] h       a (possibly) new individual's signature to be stored in
///                    the table
/// \param[in] fitness the fitness of the individual
///
template<Fitness F>
void cache<F>::insert(const hash_t &h, const F &fitness)
{
  std::lock_guard lock(mutex_);

  const slot s{h, fitness, seal_};
  table_[index(s.hash)] = s;
}

///
/// \param[in] in input stream
/// \return       `true` if the object is correctly loaded
///
/// \note
/// If the load operation isn't successful the current object isn't changed.
///
template<Fitness F>
bool cache<F>::load(std::istream &in)
{
  std::lock_guard lock(mutex_);

  decltype(seal_) t_seal;
  if (!(in >> t_seal))
    return false;

  std::size_t n;
  if (!(in >> n))
    return false;

  for (std::size_t i(0); i < n; ++i)
  {
    slot s;
    s.seal = t_seal;

    if (!s.hash.load(in))
      return false;
    if (!ultra::load(in, &s.fitness))
      return false;

    table_[index(s.hash)] = s;
  }

  seal_ = t_seal;

  return true;
}

///
/// \param[out] out output stream
/// \return         `true` if the object was saved correctly
///
template<Fitness F>
bool cache<F>::save(std::ostream &out) const
{
  std::shared_lock lock(mutex_);

  out << seal_ << ' ' << '\n';

  std::size_t num(0);
  for (const auto &s : table_)
    if (s.seal == seal_)
      ++num;
  out << num << '\n';

  for (const auto &s : table_)
    if (s.seal == seal_)
    {
      if (!s.hash.save(out))
        return false;
      if (!ultra::save(out, s.fitness))
        return false;
    }

  return out.good();
}

///
/// \return `true` if the object passes the internal consistency check
///
template<Fitness F>
bool cache<F>::is_valid() const
{
  return seal_ > 0;
}

#endif  // include guard
