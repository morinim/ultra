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

namespace
{

// Index of the print format flag (used as argument of `std::iword`).
static const int print_format_index {std::ios_base::xalloc()};

}  // unnamed namespace

namespace ultra
{

///
/// A measurement of the age of an individual (mainly used for ALPS).
///
/// \return the individual's age
///
/// This is a measure of how long an individual's family of genotypic
/// material has been in the population. Randomly generated individuals,
/// such as those that are created when the search algorithm are started,
/// start with an age of `0`. Each generation that an individual stays in the
/// population (such as through elitism) its age is increased by `1`.
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

// **********************
// * PRINTING SUBSYSTEM *
// **********************
namespace out
{
///
/// \param[in] o an output stream
/// \return      the current value of the print format flag for the `o` stream
///
print_format_t print_format_flag(std::ostream &o)
{
  return static_cast<print_format_t>(o.iword(print_format_index));
}

std::ostream &operator<<(std::ostream &o, print_format pf)
{
  o.iword(print_format_index) = pf.t_;
  return o;
}

///
/// Used to print the content of an individual in c-language format.
///
/// \remark
/// Sticky manipulator.
///
std::ostream &c_language(std::ostream &o)
{
  o.iword(print_format_index) = c_language_f;
  return o;
}

///
/// Used to print the content of an individual in cpp-language format.
///
/// \remark
/// Sticky manipulator.
///
std::ostream &cpp_language(std::ostream &o)
{
  o.iword(print_format_index) = cpp_language_f;
  return o;
}

///
/// Used to print the content of an individual in python-language format.
///
/// \remark
/// Sticky manipulator.
///
std::ostream &python_language(std::ostream &o)
{
  o.iword(print_format_index) = python_language_f;
  return o;
}

///
/// Used to print the complete content of an individual.
///
/// \note
/// Mostly used during debugging.
///
/// \remark
/// Sticky manipulator.
///
std::ostream &dump(std::ostream &o)
{
  o.iword(print_format_index) = dump_f;
  return o;
}

///
/// Used to print a graph, in dot language, representing the individual.
///
/// \see
/// https://www.graphviz.org/
///
/// \remark
/// Sticky manipulator.
///
std::ostream &graphviz(std::ostream &o)
{
  o.iword(print_format_index) = graphviz_f;
  return o;
}

///
/// Used to print the individual on a single line.
///
/// Not at all human readable, but a compact representation for import/export.
///
/// \remark
/// Sticky manipulator.
///
std::ostream &in_line(std::ostream &o)
{
  o.iword(print_format_index) = in_line_f;
  return o;
}

///
/// Used to print a human readable representation of the individual.
///
/// Do you remember C=64's `LIST`? :-)
///
///     10 PRINT "HOME"
///     20 PRINT "SWEET"
///     30 GOTO 10
///
/// \remark
/// Sticky manipulator.
///
std::ostream &list(std::ostream &o)
{
  o.iword(print_format_index) = list_f;
  return o;
}

///
/// Used to print the individual as a tree structure.
///
/// \remark
/// Sticky manipulator.
///
std::ostream &tree(std::ostream &o)
{
  o.iword(print_format_index) = tree_f;
  return o;
}

}  // namespace out

}  // namespace ultra
