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

#if !defined(ULTRA_INDIVIDUAL_H)
#define      ULTRA_INDIVIDUAL_H

#include "kernel/hash_t.h"
#include "kernel/symbol_set.h"

#include <iosfwd>
#include <string>

namespace ultra
{

namespace out
{

/// Rendering format used to print an individual.
enum print_format_t {list_f,  // default value
                     dump_f, graphviz_f, in_line_f, tree_f,
                     language_f,
                     c_language_f = language_f + symbol::c_format,
                     cpp_language_f = language_f + symbol::cpp_format,
                     python_language_f = language_f + symbol::python_format};

}  // namespace out

///
/// A single member of a `population`.
///
/// Each individual contains a genome which represents a possible solution to
/// the task being tackled (i.e. a point in the search space).
///
/// This class is the base class of every type of individual and factorizes out
/// common code / data members.
///
/// \note AKA chromosome
///
/// \note
/// Thread-safety guarantees are type-specific. See derived class
/// documentation.
///
class individual
{
public:
  // ---- Member types ----
  using age_t = unsigned;

  // ---- Age management ----
  [[nodiscard]] age_t age() const noexcept;
  void inc_age(unsigned = 1) noexcept;

  // ---- Serialization ----
  [[nodiscard]] bool load(std::istream &, const symbol_set & = {});
  [[nodiscard]] bool save(std::ostream &) const;

  // ---- Misc ----
  [[nodiscard]] hash_t signature() const noexcept;

  [[nodiscard]] std::string format(out::print_format_t) const;

protected:
  ~individual() = default;

  void set_if_older_age(age_t) noexcept;

  // Note that syntactically distinct (but logically equivalent) individuals
  // have the same signature. This is a very interesting  property, useful
  // for individual comparison, information retrieval, entropy calculation...
  hash_t signature_ {};

private:
  [[nodiscard]] virtual bool load_impl(std::istream &, const symbol_set &) = 0;
  [[nodiscard]] virtual bool save_impl(std::ostream &) const = 0;
  [[nodiscard]] virtual hash_t hash() const = 0;

  // Writes the representation selected by `format` to the output stream.
  virtual void print_impl(std::ostream &, out::print_format_t) const = 0;

  age_t age_ {0};
};  // class individual

template<class I> concept Individual =
  std::default_initializable<I>
  && requires(const I &ci, I &i)
  {
    { ci.empty() } -> std::convertible_to<bool>;
    { ci.signature() } -> std::same_as<hash_t>;
    { ci.age() } -> std::convertible_to<individual::age_t>;
    { i.inc_age() } -> std::same_as<void>;
  };

}  // namespace ultra

#endif  // include guard
