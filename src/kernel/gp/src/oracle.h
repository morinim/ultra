/**
 *  \file
 *  \remark This file is part of ORACLE.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_ORACLE_H)
#define      ULTRA_ORACLE_H

#include "kernel/exceptions.h"
#include "kernel/gp/src/dataframe.h"
#include "kernel/gp/src/interpreter.h"
#include "kernel/gp/team.h"

namespace ultra
{

template<class P> concept IndividualOrTeam = Individual<P> || Team<P>;

namespace src
{

#include "kernel/gp/src/oracle_internal.tcc"

// Forward declarations.
class basic_oracle;

namespace serialize
{

namespace oracle
{

template<IndividualOrTeam P> std::unique_ptr<basic_oracle> load(
  std::istream &, const symbol_set &);

}  // namespace oracle

bool save(std::ostream &, const basic_oracle *);
bool save(std::ostream &, const basic_oracle &);
bool save(std::ostream &, const std::unique_ptr<basic_oracle> &);

}  // namespace serialize

///
/// Contains a class ID / confidence level pair.
///
struct classification_result
{
  src::class_t label;   /// class ID
  double    sureness;   /// confidence level
};

///
/// The basic interface of an oracle.
///
/// An oracle predicts the answers to our problem. It's the *incarnation* of
/// the individual/team we've evolved.
///
/// \note
/// The output of `basic_oracle` and `src::interpreter` can be similar or
/// distinct, depending on the task (regression, classification...).
/// E.g. for **regression problems** `basic_oracle` and `src::interpreter`
/// calculate the same number.
/// `basic_oracle` always calculates a meaningful value for the end-user (the
/// class of an example, an approximation...) while `src::interpreter` often
/// outputs an intermediate value that is just a building block for
/// `basic_oracle` (e.g. classification tasks with discriminant functions).
/// The typical use chain is:
/// src::evaluator --[uses]--> basic_oracle --[uses]--> interpreter.
///
/// \note
/// Another interesting function of `basic_oracle` is that it extends the
/// functionalities of interpreter to teams.
///
class basic_oracle
{
public:
  virtual ~basic_oracle() = default;

  [[nodiscard]] virtual value_t operator()(const example &) const = 0;

  [[nodiscard]] virtual bool is_valid() const = 0;

  //[[nodiscard]] virtual double measure(const model_metric &,
  //                                     const dataframe &) const = 0;
  [[nodiscard]] virtual std::string name(const value_t &) const = 0;
  [[nodiscard]] virtual classification_result tag(const example &) const = 0;

private:
  // *** Serialization ***
  [[nodiscard]] virtual std::string serialize_id() const = 0;
  virtual bool save(std::ostream &) const = 0;

  template<IndividualOrTeam P> friend std::unique_ptr<basic_oracle>
  serialize::oracle::load(std::istream &, const symbol_set &);
  friend bool serialize::save(std::ostream &, const basic_oracle *);
};

// ***********************************************************************
// * Symbolic regression                                                 *
// ***********************************************************************

/// The model_metric class choose the appropriate method considering this type.
/// \see
/// core_class_oracle
class core_reg_oracle : public basic_oracle {};

///
/// Oracle function specialized for regression tasks.
///
/// \tparam S stores the individual inside vs keep a reference only.
///           Sometimes we need an autonomous oracle function that stores
///           everything it needs inside (it will survive the death of the
///           individual it's constructed on). Sometimes we prefer space
///           efficiency (typically inside an evaluator)
///
template<IndividualOrTeam P, bool S>
class basic_reg_oracle : public core_reg_oracle,
                         protected internal::reg_oracle_storage<P, S>
{
public:
  explicit basic_reg_oracle(const P &);
  basic_reg_oracle(std::istream &, const symbol_set &);

  [[nodiscard]] value_t operator()(const example &) const final;

  [[nodiscard]] std::string name(const value_t &) const final;

  //[[nodiscard]] double measure(const model_metric &,
  //                             const dataframe &) const final;

  [[nodiscard]] bool is_valid() const final;

  // *** Serialization ***
  static const std::string SERIALIZE_ID;
  bool save(std::ostream &) const final;

private:
  // Not useful for regression tasks and moved to private section.
  [[nodiscard]] classification_result tag(const example &) const final;

  [[nodiscard]] std::string serialize_id() const final { return SERIALIZE_ID; }
};

// ***********************************************************************
// *  Template aliases to simplify the syntax and help the user          *
// ***********************************************************************
template<IndividualOrTeam P>
class reg_oracle : public basic_reg_oracle<P, true>
{
public:
  using reg_oracle::basic_reg_oracle::basic_reg_oracle;
};
template<IndividualOrTeam P> reg_oracle(const P &) -> reg_oracle<P>;


#include "kernel/gp/src/oracle.tcc"
}  // namespace src

}  // namespace ultra

#endif  // include guard
