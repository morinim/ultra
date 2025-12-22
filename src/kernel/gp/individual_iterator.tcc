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

#if !defined(ULTRA_GP_INDIVIDUAL_EXON_VIEW_TCC)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_GP_INDIVIDUAL_ITERATOR_TCC)
#define      ULTRA_GP_INDIVIDUAL_ITERATOR_TCC

class individual;

namespace internal
{

///
/// Sentinel type marking the end of exon iteration.
///
/// The sentinel compares equal to an exon iterator when no further active
/// loci remain to be explored.
///
/// \remark This sentinel is intentionally stateless.
///
struct exon_sentinel {};

///
/// Input iterator over the active genes (exons) of an individual.
///
/// The iterator performs a dependency-driven traversal starting from the
/// individual's output locus. Each increment explores the arguments of the
/// current gene and discovers additional active loci.
///
/// \tparam is_const if `true`, the iterator provides read-only access
///
template<bool is_const>
class basic_exon_iterator
{
public:
  /// Iterator concept for ranges.
  using iterator_concept = std::input_iterator_tag;
  /// Iterator category for legacy algorithms.
  using iterator_category = std::input_iterator_tag;

  using difference_type = std::ptrdiff_t;
  using value_type = gene;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;

  using ptr= std::conditional_t<is_const, const_pointer, pointer>;
  using ref= std::conditional_t<is_const, const_reference, reference>;
  using ind= std::conditional_t<is_const, const gp::individual, gp::individual>;

  /// Constructs an iterator starting from the output locus.
  ///
  /// \param[in] id the individual being iterated
  explicit basic_exon_iterator(ind &id)
    : loci_({id.start()}), ind_(std::addressof(id)) {}

  /// Advances the iterator to the next active gene.
  ///
  /// Discovers new active loci by inspecting the arguments of the current
  /// gene.
  ///
  /// \return reference to this iterator
  basic_exon_iterator &operator++()
  {
    if (!loci_.empty())
    {
      const auto &g(operator*());

      for (const auto &a : g.args)
        if (a.index() == d_address)
          loci_.insert(g.locus_of_argument(a));

      loci_.erase(loci_.begin());
    }

    return *this;
  }

  /// Advances the iterator and returns the previous state.
  basic_exon_iterator operator++(int)
  {
    basic_exon_iterator tmp(*this);
    operator++();
    return tmp;
  }

  /// Compares the iterator with the end sentinel.
  ///
  /// \return `true` if no further active loci remain.
  [[nodiscard]] bool operator==(exon_sentinel) const noexcept
  {
    return loci_.empty();
  }

  /// Inequality comparison with the sentinel.
  [[nodiscard]] bool operator!=(exon_sentinel s) const noexcept
  {
    return !(*this == s);
  }

  /// Symmetric sentinel comparison.
  friend bool operator==(exon_sentinel s,
                         const basic_exon_iterator &it) noexcept
  {
    return it == s;
  }

  /// Symmetric sentinel comparison.
  friend bool operator!=(exon_sentinel s,
                         const basic_exon_iterator &it) noexcept
  {
    return !(it == s);
  }

  /// Dereferences the iterator.
  ///
  /// \return reference to the current active gene
  [[nodiscard]] ref &operator*() const
  {
    return ind_->genome_(locus());
  }

  /// Member access to the current gene.
  ///
  /// \return pointer to the current locus of the individual
  [[nodiscard]] ptr operator->() const
  {
    return &operator*();
  }

  /// \return the locus of the current gene
  [[nodiscard]] ultra::locus locus() const
  {
    return *loci_.cbegin();
  }

private:
  // For different implementation see `test/speed_gp_individual_iterator.cc`.

  // Set of active loci yet to be explored (ordered descending). Aka frontier,
  // pending or worklist.
  std::set<ultra::locus, std::greater<ultra::locus>> loci_ {};

  // Pointer to the individual being iterated.
  ind *ind_ {nullptr};
};

}  // namespace internal

#endif  // include guard
