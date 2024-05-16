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

#include <filesystem>
#include <string>

#include "kernel/problem.h"
#include "kernel/gp/src/dataframe.h"
#include "kernel/gp/src/multi_dataset.h"

namespace ultra::src
{

///
/// Provides a GP-specific interface to the generic `problem` class.
///
/// The class is a facade that provides a simpler interface to represent
/// symbolic regression / classification tasks.
///
class problem : public ultra::problem
{
public:
  // ---- Constructors ----
  /// New empty instance of src_problem.
  ///
  /// \warning
  /// User **must** initialize:
  /// - the training dataset;
  /// - the entire symbol set (functions and terminals)
  /// before starting the evolution.
  problem() = default;

  explicit problem(dataframe);
  explicit problem(const std::filesystem::path &,
                   const dataframe::params & = {});
  explicit problem(std::istream &, const dataframe::params & = {});

  // ---- Information about the problem ----
  /// Just a shorthand for checking number of classes.
  [[nodiscard]] bool classification() const noexcept { return classes() > 1; }

  [[nodiscard]] std::size_t categories() const noexcept;
  [[nodiscard]] std::size_t classes() const noexcept;
  [[nodiscard]] std::size_t variables() const noexcept;

  // ---- Misc ----
  [[nodiscard]] bool operator!() const;
  void setup_symbols();
  void setup_terminals();
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
