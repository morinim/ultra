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

#if !defined(ULTRA_VALIDATION_STRATEGY_H)
#define      ULTRA_VALIDATION_STRATEGY_H

namespace ultra
{

///
/// Interface for specific training / cross validation techniques (e.g.
/// holdout validation, dynamic subsect selection...).
///
class validation_strategy
{
public:
  virtual ~validation_strategy() = default;

  /// Prepares the data structures / environment needed for training.
  ///
  /// \note
  /// Called at the beginning of the evolution (one time per run).
  virtual void training_setup(unsigned /* run */) = 0;

  /// Changes the training environment during evolution.
  ///
  /// \return `true` if the training environment has changed
  ///
  /// By default does nothing, signalling that nothing is changed.
  ///
  /// \note
  /// Called at the beginning of every generation (multiple times per run).
  ///
  virtual bool shake(unsigned /* generation */) = 0;

  /// Prepares the data structures / environment needed for validation
  ///
  /// \note Called at the end of the evolution (one time per run).
  virtual void validation_setup(unsigned /* run */) = 0;
};

///
/// A "null object" implementation of validation_strategy.
///
/// Implements the interface of a `validation_strategy` with empty body
/// methods (it's very predictable and has no side effects: it does nothing).
///
/// \see https://en.wikipedia.org/wiki/Null_Object_pattern
///
class as_is_validation final : public validation_strategy
{
public:
  void training_setup(unsigned) override {}
  bool shake(unsigned) override { return false; }
  void validation_setup(unsigned) override {}
};

}  // namespace ultra

#endif  // include guard
