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

#if !defined(ULTRA_HGA_INDIVIDUAL_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_HGA_INDIVIDUAL_TCC)
#define      ULTRA_HGA_INDIVIDUAL_TCC

///
/// Proxy granting controlled write access to an individual.
///
/// `modify_proxy` is a capability object created exclusively by
/// `individual::modify()`. It provides temporary, scoped permission to mutate
/// the internal genome of an individual.
///
/// The proxy must not escape the scope of the call to `modify()`. All class
/// invariants of individual are guaranteed to hold again when
/// `individual::modify()` returns.
///
/// Users must not assume any invariant holds while operating on the proxy,
/// except those explicitly documented.
///
class individual::modify_proxy
{
public:
  /// Access the owning individual in read-only mode.
  ///
  /// \return a const reference to the owning individual
  ///
  /// This allows inspection of the individual while mutation is in progress,
  /// without granting additional write access.
  [[nodiscard]] const individual& self() const noexcept { return ind_; }

  [[nodiscard]] auto &genome() noexcept { return ind_.genome_; }

  /// Mutable access to a genome element.
  ///
  /// \param[in] i index of the gene to access
  /// \return      a mutable reference to the requested gene
  ///
  /// \pre i < size()
  ///
  /// Provides write access to the gene at position `i`. This function is only
  /// callable within the scope of `individual::modify()`.
  [[nodiscard]] value_type &operator[](std::size_t i) noexcept
  {
    Expects(i < size());
    return ind_.genome_[i];
  }

  /// Number of genes in the genome.
  ///
  /// \return the size of the genome
  [[nodiscard]] std::size_t size() const noexcept
  {
    return self().size();
  }

private:
  friend class individual;

  /// Construct a modification proxy for an individual.
  ///
  /// This constructor is private to prevent unauthorised creation. Only
  /// `individual::modify()` may instantiate this type.
  explicit modify_proxy(individual& ind) noexcept : ind_(ind) {}

  individual &ind_;
};

///
/// Perform a controlled modification of the individual.
///
/// \tparam F callable type accepting a modify_proxy
///
/// \param f callable invoked with a modification proxy
///
/// \post the individual is valid and its signature is up to date
///
/// Executes the callable `f` with a temporary modification proxy granting
/// exclusive write access to the genome.
/// During execution of `f`, class invariants of individual may be temporarily
/// violated. All invariants are restored when this function returns.
/// This is the only public entry point that permits arbitrary mutation of the
/// individual.
///
template<class F>
  requires
    std::invocable<F &, individual::modify_proxy &>
    && std::same_as<std::invoke_result_t<F &, individual::modify_proxy&>, void>
void individual::modify(F &&f)
{
  modify_proxy m(*this);
  f(m);

  signature_ = hash();
  Ensures(is_valid());
}

#endif  // include guard
