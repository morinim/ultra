/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_GP_INDIVIDUAL_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_GP_INDIVIDUAL_EXON_VIEW_TCC)
#define      ULTRA_GP_INDIVIDUAL_EXON_VIEW_TCC

#include "individual_iterator.tcc"

///
/// A view over the active genes (exons) of an individual.
///
/// This view exposes only the genes that are reachable from the individual's
/// output locus, following argument dependencies.
/// The view models a `std::ranges::input_range`:
/// - iteration is single-pass;
/// - copying iterators does not guarantee independent traversal;
/// - no size is known in advance.
///
/// \remark The underlying individual must outlive the view.
///
class exon_view : public std::ranges::view_interface<exon_view>
{
public:
  /// Iterator type iterating over mutable active genes.
  using iterator = internal::basic_exon_iterator<false>;

  /// Sentinel marking the end of the exon traversal.
  using sentinel = internal::exon_sentinel;

  /// Constructs a view over the active genes of an individual.
  /// \param[in] ind The individual whose exons will be iterated.
  explicit exon_view(gp::individual &ind) noexcept
    : ind_(std::addressof(ind))
  {}

  /// \return an input iterator positioned at the first exon
  [[nodiscard]] iterator begin() const { return iterator(*ind_); }

  /// \return a sentinel representing the end of the exon range
  [[nodiscard]] sentinel end() const noexcept { return {}; }

private:
  gp::individual *ind_;
};  // class exon_view

///
/// A const view over the active genes (exons) of an individual.
///
/// This is the const-qualified counterpart of \ref exon_view.
/// The view provides read-only access to active genes.
///
class const_exon_view : public std::ranges::view_interface<const_exon_view>
{
public:
  using iterator = internal::basic_exon_iterator<true>;
  using sentinel = internal::exon_sentinel;

  explicit const_exon_view(const gp::individual &ind) noexcept
    : ind_(std::addressof(ind))
  {}

  [[nodiscard]] iterator begin() const { return iterator(*ind_); }
  [[nodiscard]] sentinel end() const noexcept { return {}; }

private:
  const gp::individual *ind_;
};  // class const_exon_view


// Just to document documents the contract for future maintainers
static_assert(std::ranges::input_range<const_exon_view>);
static_assert(!std::ranges::forward_range<const_exon_view>);

#endif  // include guard
