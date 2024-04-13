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

namespace ultra::src
{

/// Data/simulations are categorised in three sets:
/// - *training* used directly for learning;
/// - *validation* for controlling overfitting and measuring the performance
///   of an individual;
/// - *test* for a forecast of how well an individual will do in the real
///   world.
/// The `ultra::search` class asks the `problem` class to setup the requested
/// simulation/dataset via the `select` function.
enum class dataset_t {training = 0, validation, test};

///
/// Provides a GP-specific interface to the generic `problem` class.
///
/// The class is a facade that provides a simpler interface to represent
/// symbolic regression / classification tasks.
///
class problem : public ultra::problem
{
public:
  // --- Constructors ---
  /// New empty instance of src_problem.
  ///
  /// \warning
  /// User **must** initialize:
  /// - the training dataset;
  /// - the entire symbol set (functions and terminals)
  /// before starting the evolution.
  problem() = default;

  explicit problem(const std::filesystem::path &, typing = typing::weak);
  explicit problem(std::istream &, typing = typing::weak);

  struct default_symbols_t {};
  static constexpr default_symbols_t default_symbols {};

  problem(const std::filesystem::path &, const default_symbols_t &,
          typing = typing::weak);
  problem(const std::filesystem::path &, const std::filesystem::path &,
          typing = typing::weak);
  // --------------------

  [[nodiscard]] bool operator!() const;
  std::size_t setup_symbols();
  std::size_t setup_symbols(const std::filesystem::path &);
  void setup_terminals();

  [[nodiscard]] const dataframe &data(
    dataset_t = dataset_t::training) const noexcept;
  [[nodiscard]] dataframe &data(dataset_t = dataset_t::training) noexcept;

  /// Just a shorthand for checking number of classes.
  [[nodiscard]] bool classification() const noexcept { return classes() > 1; }

  [[nodiscard]] std::size_t categories() const noexcept;
  [[nodiscard]] std::size_t classes() const noexcept;
  [[nodiscard]] std::size_t variables() const noexcept;

  [[nodiscard]] bool is_valid() const override;

private:
  // Private support methods.
  [[nodiscard]] bool compatible(const function::param_data_types &,
                                const std::vector<std::string> &) const;
  std::size_t setup_symbols_impl();
  std::size_t setup_symbols_impl(const std::filesystem::path &);

  // Private data members.
  dataframe training_ {};
  dataframe validation_ {};
};

}  // namespace ultra::src

#endif  // include guard
