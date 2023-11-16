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

#if !defined(ULTRA_LOCUS_H)
#define      ULTRA_LOCUS_H

#include <iostream>
#include <limits>

namespace ultra
{

///
/// A locus is uniquely identified by an index **and** a category.
///
struct locus
{
  /// Index in the genome.
  using index_t = std::size_t;

  locus() = default;
  constexpr locus(index_t i, size_t c) : index(i), category(c) {}

  index_t index;
  std::size_t category;

  [[nodiscard]] static constexpr locus npos()
  {
    return {std::numeric_limits<index_t>::max(),
            std::numeric_limits<std::size_t>::max()};
  }

  [[nodiscard]] bool operator==(const locus &) const = default;
};

///
/// \param[in] l1 first locus
/// \param[in] l2 second locus
/// \return       `true` if `l1` precedes `l2` in lexicographic order
///               (http://en.wikipedia.org/wiki/Lexicographical_order)
///
[[nodiscard]] inline bool operator<(const locus &l1, const locus &l2)
{
  return l1.index < l2.index ||
         (l1.index == l2.index && l1.category < l2.category);
}

///
/// \param[in] l1 first locus
/// \param[in] l2 second locus
/// \return       `true` if `l2` precedes `l1` in lexicographic order
///               (http://en.wikipedia.org/wiki/Lexicographical_order)
///
[[nodiscard]] inline bool operator>(const locus &l1, const locus &l2)
{
  return l1.index > l2.index ||
         (l1.index == l2.index && l1.category > l2.category);
}

///
/// \param[in] l base locus
/// \param[in] i displacement
/// \return      a new locus obtained from `l` incrementing index component by
///              `i` (and not changing the category component)
///
[[nodiscard]] inline locus operator+(const locus &l, locus::index_t i)
{
  return {l.index + i, l.category};
}

///
/// \param[out] s output stream
/// \param[in]  l locus to print
/// \return       output stream including `l`
///
inline std::ostream &operator<<(std::ostream &s, const locus &l)
{
  return s << '[' << l.index << ',' << l.category << ']';
}

}  // namespace ultra

#endif  // include guard
