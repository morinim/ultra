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

#if !defined(ULTRA_FUNCTION_H)
#define      ULTRA_FUNCTION_H

#include <vector>

#include "kernel/symbol.h"

namespace ultra
{
///
/// A symbol with `arity() > 0`.
///
/// A function labels the internal (non-leaf) points of the parse trees that
/// represent the programs in the population. An example function set might be
/// {+, -, *}.
///
/// \warning
/// Each function should be able to handle gracefully all values it might
/// receive as input (this is called *closure property*). If there is a way to
/// crash the system, the GP system will certainly hit upon hit.
///
class function : public symbol
{
public:
  using param_data_types = std::vector<category_t>;
  using return_type = category_t;

  class params;

  function(const std::string &, return_type, param_data_types);
  function(const std::string &, std::size_t);

  [[nodiscard]] category_t categories(std::size_t) const;
  [[nodiscard]] const param_data_types &categories() const;

  [[nodiscard]] std::size_t arity() const;

  virtual value_t eval(const params &) const = 0;

  [[nodiscard]] virtual std::string to_string(format = c_format) const;

  [[nodiscard]] bool is_valid() const override;

private:
  const param_data_types params_;
};

///
/// An interface for parameter passing to functions.
///
/// Parameters are lazy evaluated so:
/// - store the value of `fetch_arg(i)` (i.e. `operator[](i)`) in a local
///   variable for multiple uses;
/// - call `fetch_arg(i)` only if you need the `i`-th argument.
///
class function::params
{
public:
  /// Fetches a specific input parameter assuming referential transparency.
  /// Referential transparency allows cache based optimization for argument
  /// retrieval. If this kind of optimization isn't required the implementation
  /// can be a simple call to `fetch_opaque_arg`.
  [[nodiscard]] virtual value_t fetch_arg(std::size_t) const = 0;

  /// Fetches a specific input parameter without assuming referential
  /// transparency.
  /// \remark
  /// Sometimes return value is ignored: typically for agent simulation (the
  /// caller is only interested in the side effects of the call).
  virtual value_t fetch_opaque_arg(std::size_t) const = 0;

  /// Equivalent to fetch_arg().
  [[nodiscard]] value_t operator[](std::size_t i) const { return fetch_arg(i); }
};

}  // namespace ultra

#endif  // include guard
