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

#if !defined(ULTRA_PRINT_INTERNAL_H)
#define      ULTRA_PRINT_INTERNAL_H

#include "kernel/individual.h"
#include "utility/log.h"

namespace ultra::internal
{

///
/// Prints the genes of the individual.
///
/// \param[out] s   output stream
/// \param[in]  ind data to be printed
///
template<Individual I>
void print_linear_in_line(std::ostream &s, const I &ind)
{
  if (ind.empty())
    return;

  s << *ind.begin();
  for (auto it(std::next(ind.begin())); it != ind.end(); ++it)
    s << ' ' << *it;
}

///
/// Inserts into the output stream the graph representation of the individual.
///
/// \param[out] s   output stream
/// \param[in]  ind data to be printed
///
/// \note
/// The format used to describe the graph is the dot language
/// (https://www.graphviz.org/).
///
template<Individual I>
void print_linear_graphviz(std::ostream &s, const I &ind)
{
  s << "graph {\n";

  std::size_t i(0);
  for (const auto &g : ind)
  {
    s << "  g" << i << " [label=" << g << ", shape=circle];\n";
    ++i;
  }

  for (std::size_t j(1); j < i; ++j)
    s << "  g" << (j - 1) << " -- g" << j << ";\n";

  s << '}';
}

template<class Individual>
void print_linear(std::ostream &s, const Individual &ind,
                  out::print_format_t format, std::string_view type_name)
{
  switch (format)
  {
  case out::graphviz_f:
    print_linear_graphviz(s, ind);
    return;

  case out::in_line_f:
    print_linear_in_line(s, ind);
    return;

  default:
    ultraWARNING << "Unsupported print format for " << type_name;
    print_linear_in_line(s, ind);
  }
}

}  // namespace ultra::internal

#endif  // include guard
