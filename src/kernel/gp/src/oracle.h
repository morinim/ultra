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
#include "kernel/gp/src/calculate_metrics.h"
#include "kernel/gp/src/dataframe.h"
#include "kernel/gp/src/interpreter.h"
#include "kernel/gp/team.h"

namespace ultra
{

namespace src
{

#include "kernel/gp/src/oracle_internal.tcc"

// Forward declarations.
class basic_oracle;
class model_metric;

namespace serialize
{

namespace oracle
{

template<Individual P = gp::individual>
[[nodiscard]] std::unique_ptr<basic_oracle>
load(std::istream &, const symbol_set &);

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

  [[nodiscard]] virtual value_t operator()(
    const std::vector<value_t> &) const = 0;

  [[nodiscard]] virtual bool is_valid() const = 0;

  [[nodiscard]] virtual double measure(const model_metric &,
                                       const dataframe &) const = 0;
  [[nodiscard]] virtual std::string name(const value_t &) const = 0;
  [[nodiscard]] virtual classification_result tag(
    const std::vector<value_t> &) const = 0;

private:
  // *** Serialization ***
  [[nodiscard]] virtual std::string serialize_id() const = 0;
  virtual bool save(std::ostream &) const = 0;

  template<Individual P> friend std::unique_ptr<basic_oracle>
  serialize::oracle::load(std::istream &, const symbol_set &);
  friend bool serialize::save(std::ostream &, const basic_oracle &);
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
template<Individual P, bool S>
class basic_reg_oracle : public core_reg_oracle,
                         protected internal::reg_oracle_storage<P, S>
{
public:
  explicit basic_reg_oracle(const P &);
  basic_reg_oracle(std::istream &, const symbol_set &);

  [[nodiscard]] value_t operator()(const std::vector<value_t> &) const final;

  [[nodiscard]] std::string name(const value_t &) const final;

  [[nodiscard]] double measure(const model_metric &,
                               const dataframe &) const final;

  [[nodiscard]] bool is_valid() const final;

  // *** Serialization ***
  static const std::string SERIALIZE_ID;
  bool save(std::ostream &) const final;

private:
  // Not useful for regression tasks and moved to private section.
  [[nodiscard]] classification_result tag(
    const std::vector<value_t> &) const final;

  [[nodiscard]] std::string serialize_id() const final { return SERIALIZE_ID; }
};

// ***********************************************************************
// * Classification                                                      *
// ***********************************************************************

///
/// For classification problems there are two major possibilities to combine
/// the outputs of multiple predictors: either the raw output values or the
/// classification decisions can be aggregated (in the latter case the team
/// members act as full pre-classificators themselves). We decided for the
/// latter and combined classification decisions (thanks to the confidence
/// parameter we don't have a reduction in the information content that each
/// individual can contribute to the common team decision).
///
enum class team_composition
{
  mv,  // majority voting
  wta, // winner takes all

  standard = wta
};

/// The model_metric class choose the appropriate method considering this type.
/// \see
/// core_reg_oracle
class core_class_oracle : public basic_oracle {};

///
/// The basic interface of a classification oracle.
///
/// \tparam N stores the name of the classes vs doesn't store the names
///
/// This class:
/// - extends the interface of class basic_oracle to handle typical
///   requirements of classification tasks;
/// - factorizes out some code from specific classification schemes;
/// - optionally stores class names.
///
template<bool N>
class basic_class_oracle : public core_class_oracle,
                           protected internal::class_names<N>
{
public:
  explicit basic_class_oracle(const dataframe &);

  [[nodiscard]] value_t operator()(const std::vector<value_t> &) const final;

  [[nodiscard]] std::string name(const value_t &) const final;

  [[nodiscard]] double measure(const model_metric &,
                               const dataframe &) const final;

protected:
  basic_class_oracle() = default;
};

///
/// Oracle for the Gaussian Distribution Classification.
///
/// \tparam I individual
/// \tparam S stores the individual inside vs keep a reference only
/// \tparam N stores the name of the classes vs doesn't store the names
///
/// \see
/// ultra::src::::gaussian_evaluator for further details.
///
template<class I, bool S, bool N>
class basic_gaussian_oracle : public basic_class_oracle<N>
{
public:
  basic_gaussian_oracle(const I &, dataframe &);
  basic_gaussian_oracle(std::istream &, const symbol_set &);

  [[nodiscard]] classification_result tag(
    const std::vector<value_t> &) const final;

  [[nodiscard]] bool is_valid() const final;

  // *** Serialization ***
  static const std::string SERIALIZE_ID;
  bool save(std::ostream &) const final;

private:
  // *** Private support methods ***
  void fill_vector(dataframe &);
  bool load_(std::istream &, const symbol_set &, std::true_type);
  bool load_(std::istream &, const symbol_set &, std::false_type);

  [[nodiscard]] std::string serialize_id() const final { return SERIALIZE_ID; }

  // *** Private data members ***
  basic_reg_oracle<I, S> oracle_;

  // `gauss_dist[i]` contains the gaussian distribution of the i-th class of
  // the classification problem.
  std::vector<distribution<double>> gauss_dist_ {};
};

// ***********************************************************************
// * Extensions to support teams                                          *
// ***********************************************************************

///
/// An helper class for extending classification schemes to teams.
///
/// \tparam I type of individual
/// \tparam S stores the individual inside vs keep a reference only
/// \tparam N stores the name of the classes vs doesn't store the names
/// \tparam L the basic classificator that must be extended
/// \tparam C composition method for team's member responses
///
template<Individual I, bool S, bool N,
         template<class, bool, bool> class L,
         team_composition C = team_composition::standard>
class team_class_oracle : public basic_class_oracle<N>
{
public:
  template<class... Args> team_class_oracle(const gp::team<I> &, dataframe &,
                                            Args &&...);
  team_class_oracle(std::istream &, const symbol_set &);

  [[nodiscard]] classification_result tag(
    const std::vector<value_t> &) const final;

  [[nodiscard]] bool is_valid() const final;

  static const std::string SERIALIZE_ID;

private:
  bool save(std::ostream &) const final;
  [[nodiscard]] std::string serialize_id() const final;

  // The components of the team never store the names of the classes. If we
  // need the names, the master class will memorize them.
  std::vector<L<I, S, false>> team_;

  src::class_t classes_ {};
};

///
/// Gaussian Distribution Classification specialization for teams.
///
/// \tparam I type of individual
/// \tparam S stores the individual inside vs keep a reference only
/// \tparam N stores the name of the classes vs doesn't store the names
///
template<class I, bool S, bool N>
class basic_gaussian_oracle<gp::team<I>, S, N>
  : public team_class_oracle<I, S, N, basic_gaussian_oracle>
{
public:
  using basic_gaussian_oracle::team_class_oracle::team_class_oracle;
};

// ***********************************************************************
// *  Template aliases to simplify the syntax and help the user          *
// ***********************************************************************
template<Individual P>
class reg_oracle : public basic_reg_oracle<P, true>
{
public:
  using reg_oracle::basic_reg_oracle::basic_reg_oracle;
};
template<Individual P> reg_oracle(const P &) -> reg_oracle<P>;

template<Individual P>
class gaussian_oracle : public basic_gaussian_oracle<P, true, true>
{
public:
  using gaussian_oracle::basic_gaussian_oracle::basic_gaussian_oracle;
};
template<Individual P> gaussian_oracle(const P &, dataframe &) -> gaussian_oracle<P>;

template<class P> gaussian_oracle(const P &, dataframe &) -> gaussian_oracle<P>;

#include "kernel/gp/src/oracle.tcc"
}  // namespace src

}  // namespace ultra

#endif  // include guard
