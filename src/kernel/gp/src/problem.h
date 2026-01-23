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

#if !defined(ULTRA_SRC_PROBLEM_H)
#define      ULTRA_SRC_PROBLEM_H

#include "kernel/problem.h"
#include "kernel/gp/src/dataframe.h"
#include "kernel/gp/src/multi_dataset.h"

#include <filesystem>
#include <string>

namespace ultra
{

///
/// Bitmask flags for configuring symbol initialisation stages.
///
/// Can be combined using bitwise operators.
///
enum class symbol_init : unsigned
{
  disabled   =  0,  /// No initialisation
  variables  =  1,  /// Initialises input variables
  attributes =  2,  /// Initialises attributes
  ephemerals =  4,  /// Initialises ephemeral values
  functions  =  8,  /// Initialises the function set
  all        = 15,  /// Initialises everything
};

/// Enable bitmask operations for symbol_init.
template<> struct is_bitmask_enum<symbol_init> : std::true_type {};

///
/// By default, the only terminals automatically initialised are variables and
/// attributes.
/// This is sensible because users can often deduce more appropriate ranges for
/// ephemerals.
///
constexpr symbol_init def_terminal_init =
  symbol_init::variables | symbol_init::attributes;

}  // namespace ultra

namespace ultra::src
{

///
/// A specialization of the generic `problem` class for symbolic regression and
/// classification problems.
///
class problem : public ultra::problem
{
public:
  // ---- Constructors ----
  /// New empty instance of src_problem.
  ///
  /// \warning
  /// The user **must** initialise:
  /// - the training dataset;
  /// - the complete symbol set (functions and terminals) before starting the
  ///   evolution.
  problem() = default;

  explicit problem(dataframe, symbol_init = def_terminal_init);
  explicit problem(const std::filesystem::path &,
                   const dataframe::params & = {});
  explicit problem(std::istream &, const dataframe::params & = {});

  // ---- Information about the problem ----
  /// Just a shorthand for checking number of classes.
  [[nodiscard]] bool classification() const noexcept { return classes() > 1; }

  [[nodiscard]] std::size_t categories() const noexcept;
  [[nodiscard]] std::size_t classes() const noexcept;
  [[nodiscard]] std::size_t variables() const noexcept;

  [[nodiscard]] bool ready() const;

  // ---- Misc ----
  void setup_symbols(symbol_init = symbol_init::all);
  void setup_terminals(symbol_init = def_terminal_init);
  [[nodiscard]] bool is_valid() const override;

  // ---- Public data members ----
  multi_dataset<dataframe> data;

private:
  // Private support methods.
  [[nodiscard]] bool compatible(const function::param_data_types &,
                                const std::vector<std::string> &) const;
};

}  // namespace ultra::src

#endif  // include guard
