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

#include "kernel/individual_format.h"

#include <ostream>

namespace
{

// Index of the print format flag (used as argument of `std::iword`).
static const int print_format_index {std::ios_base::xalloc()};

}  // unnamed namespace

namespace ultra
{

std::ostream &operator<<(std::ostream &os, const ultra::individual &ind)
{
  return os << ind.format(ultra::out::print_format_flag(os));
}

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
