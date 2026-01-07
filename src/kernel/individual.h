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

#include <fstream>

namespace ultra
{

///
/// A single member of a `population`.
///
/// Each individual contains a genome which represents a possible solution to
/// the task being tackled (i.e. a point in the search space).
///
/// This class is the base class of every type of individual and factorizes out
/// common code / data members.
///
/// \note AKA chromosome.
///
/// \note
/// Thread-safety guarantees are type-specific. See derived class
/// documentation.
///
class individual
{
public:
  using age_t = unsigned;

  [[nodiscard]] hash_t signature() const noexcept;

  [[nodiscard]] age_t age() const noexcept;
  void inc_age(unsigned = 1) noexcept;

  // Serialization.
  [[nodiscard]] bool load(std::istream &, const symbol_set & = {});
  [[nodiscard]] bool save(std::ostream &) const;

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

  age_t age_ {0};
};  // class individual

template<class I> concept Individual = requires(I i)
{
  I();

  i.empty();
  i.signature();
};

namespace out
{

/// Rendering format used to print an individual.
enum print_format_t {list_f,  // default value
                     dump_f, graphviz_f, in_line_f, tree_f,
                     language_f,
                     c_language_f = language_f + symbol::c_format,
                     cpp_language_f = language_f + symbol::cpp_format,
                     python_language_f = language_f + symbol::python_format};

bool long_form_flag(std::ostream &);
print_format_t print_format_flag(std::ostream &);

class print_format
{
public:
  explicit print_format(print_format_t t) : t_(t) {}

  friend std::ostream &operator<<(std::ostream &, print_format);

private:
  print_format_t t_;
};

std::ostream &c_language(std::ostream &);
std::ostream &cpp_language(std::ostream &);
std::ostream &dump(std::ostream &);
std::ostream &graphviz(std::ostream &);
std::ostream &in_line(std::ostream &);
std::ostream &list(std::ostream &);
std::ostream &python_language(std::ostream &);
std::ostream &tree(std::ostream &);
}  // namespace out

}  // namespace ultra

#endif  // include guard
