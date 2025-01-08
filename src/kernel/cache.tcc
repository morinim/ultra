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
/// Fast ceiling integer division.
///
/// \param[in] x dividend
/// \param[in] y divisor
/// \return      ceiling of the quotient
///
inline std::size_t div_ceil(std::size_t x, std::size_t y)
{
  // Most common architecture's divide instruction also includes remainder in
  // its result so this really needs only one division and would be very fast.
  return x/y + (x%y != 0);

  // `(x + y - 1) / y` is an alternative (but could overflow).
}

///
/// Creates a new, not empty, hash table.
///
/// \param[in] n `2^n` is the number of elements of the table
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE>
cache<F, LOCK_GROUP_SIZE>::cache(bitwidth n)
  : table_(1ull << n), locks_(div_ceil(table_.size(), LOCK_GROUP_SIZE)),
    k_mask((1ull << n) - 1)
{
  Expects(n);
  Ensures(is_valid());
}

///
/// Resize the cache.
///
/// \param[in] n `2^n` is the new number of elements of the table
///
/// \warning
/// - This is a destructive operation: content of the cache will be lost.
/// - Not concurrency-safe.
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE>
void cache<F, LOCK_GROUP_SIZE>::resize(bitwidth n)
{
  Expects(n);

  const std::size_t nelem(1ull << n);

  table_ = decltype(table_)(nelem);
  locks_ = decltype(locks_)(div_ceil(nelem, LOCK_GROUP_SIZE));
  k_mask = nelem - 1;

  Ensures(is_valid());
}

///
/// \param[in] h the signature of an individual
/// \return      an index in the hash table
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE>
std::size_t cache<F, LOCK_GROUP_SIZE>::index(const hash_t &h) const noexcept
{
  Expects(k_mask);
  return h.data[0] & k_mask;
}

///
/// \param[in] idx index in the hash table
/// \return        index of the mutex that protects the slot interval
///                containing `idx`
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE>
std::size_t cache<F, LOCK_GROUP_SIZE>::lock_index(std::size_t idx) const noexcept
{
  return idx / LOCK_GROUP_SIZE;
}

///
/// Clears the content and the statistical informations of the table.
///
///
/// \warning
/// Not concurrency-safe.
///
/// \note
/// Allocated size isn't changed.
///
/// \warning
/// Not concurrency-safe.
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE>
void cache<F, LOCK_GROUP_SIZE>::clear()
{
  ++seal_;
}

///
/// Clears the cached information for a specific individual.
///
/// \param[in] h individual's signature whose informations we have to clear
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE>
void cache<F, LOCK_GROUP_SIZE>::clear(const hash_t &h)
{
  const auto idx(index(h));
  auto &mutex(locks_[lock_index(idx)]);

  std::lock_guard lock(mutex);

  // Invalidates the slot since the first valid value for seal is `1`.
  table_[index(h)].seal = 0;
}

///
/// Looks for the fitness of an individual in the transposition table.
///
/// \param[in] h individual's signature to look for
/// \return      the fitness of the individual. If the individuals isn't
///              present returns an empty `std::optional`
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE>
std::optional<F> cache<F, LOCK_GROUP_SIZE>::find(const hash_t &h) const
{
  const auto idx(index(h));
  auto &mutex(locks_[lock_index(idx)]);

    std::shared_lock lock(mutex);

    if (const slot &s(table_[index(h)]); s.seal == seal_ && s.hash == h)
      return s.fitness;

  return std::nullopt;
}

///
/// Stores fitness information in the transposition table.
///
/// \param[in] h       a (possibly) new individual's signature to be stored in
///                    the table
/// \param[in] fitness the fitness of the individual
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE>
void cache<F, LOCK_GROUP_SIZE>::insert(const hash_t &h, const F &fitness)
{
  const auto idx(index(h));
  auto &mutex(locks_[lock_index(idx)]);

  std::lock_guard lock(mutex);

  table_[idx] = {h, fitness, seal_};
}

///
/// \param[in] in input stream
/// \return       `true` if the object is correctly loaded
///
/// \note
/// If the load operation isn't successful the current object isn't changed.
///
/// \warning
/// Not concurrency-safe.
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE>
bool cache<F, LOCK_GROUP_SIZE>::load(std::istream &in)
{
  decltype(seal_) t_seal;
  if (!(in >> t_seal))
    return false;

  std::size_t n;
  if (!(in >> n))
    return false;

  while (n)
  {
    slot s;
    s.seal = t_seal;

    if (!s.hash.load(in))
      return false;
    if (!ultra::load(in, &s.fitness))
      return false;

    table_[index(s.hash)] = s;

    --n;
  }

  seal_ = t_seal;

  return true;
}

///
/// \param[out] out output stream
/// \return         `true` if the object was saved correctly
///
/// \warning
/// Not concurrency-safe.
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE>
bool cache<F, LOCK_GROUP_SIZE>::save(std::ostream &out) const
{
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
/// \return number of bits used for hash table initialization
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE>
bitwidth cache<F, LOCK_GROUP_SIZE>::bits() const
{
  return std::bit_width(k_mask);
}

///
/// \return `true` if the object passes the internal consistency check
///
/// \warning
/// Not concurrency-safe.
///
template<Fitness F, std::size_t LOCK_GROUP_SIZE>
bool cache<F, LOCK_GROUP_SIZE>::is_valid() const
{
  if (seal_ == 0)
    return false;

  if (table_.empty())
    return k_mask == 0;

  return std::has_single_bit(table_.size()) && table_.size() - 1 == k_mask;
}

#endif  // include guard
