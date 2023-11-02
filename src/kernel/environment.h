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

#if !defined(ULTRA_ENVIRONMENT_H)
#define      ULTRA_ENVIRONMENT_H

#include "kernel/symbol.h"

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

struct environment
{
  struct slp_parameters
  {
    /// The number of genes (maximum length of an evolved program in the
    /// population).
    ///
    /// Code length have to be chosen before population is created and cannot
    /// be changed afterwards.
    ///
    /// \note
    /// A length of `0` means undefined (auto-tune).
    std::size_t code_length {0};
  } slp;

  environment &init();
  [[nodiscard]] bool is_valid(bool) const;
};  // environment

}  // namespace ultra

#endif  // include guard
