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

#include "kernel/distribution.h"
#include "kernel/exceptions.h"
#include "kernel/gp/src/interpreter.h"
#include "kernel/gp/src/predictor.h"
#include "kernel/gp/team.h"
#include "utility/matrix.h"

#include <numbers>
#include <optional>
#include <span>

namespace ultra::src
{

#include "kernel/gp/src/oracle_internal.tcc"

// Forward declarations.
class basic_oracle;

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

  [[nodiscard]] virtual std::string name(const value_t &) const = 0;
  [[nodiscard]] virtual classification_result tag(
    const std::vector<value_t> &) const = 0;

private:
  // ---- Serialization ----
  [[nodiscard]] virtual std::string serialize_id() const = 0;
  virtual bool save(std::ostream &) const = 0;

  template<Individual P> friend std::unique_ptr<basic_oracle>
  serialize::oracle::load(std::istream &, const symbol_set &);
  friend bool serialize::save(std::ostream &, const basic_oracle &);
};

// ***********************************************************************
// * Symbolic regression                                                 *
// ***********************************************************************

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
class basic_reg_oracle : public basic_oracle,
                         protected internal::reg_oracle_storage<P, S>
{
public:
  explicit basic_reg_oracle(const P &);
  basic_reg_oracle(std::istream &, const symbol_set &);

  [[nodiscard]] value_t operator()(const std::vector<value_t> &) const final;

  [[nodiscard]] std::string name(const value_t &) const final;

  [[nodiscard]] bool is_valid() const final;

  // ---- Serialization ----
  static const std::string SERIALIZE_ID;
  bool save(std::ostream &) const final;

private:
  // Not useful for regression tasks and moved to private section.
  [[noreturn]] classification_result tag(
    const std::vector<value_t> &) const final;

  [[nodiscard]] std::string serialize_id() const noexcept final
  { return SERIALIZE_ID; }
};

///
/// Adapts a regression predictor to the `basic_oracle` interface.
///
/// \tparam O predictor type
///
/// This adaptor allows any user-defined regression predictor to be used as a
/// `basic_oracle`. The wrapped predictor must satisfy `RegressionPredictor`,
/// i.e. be callable with an input example and return a `value_t`.
///
/// \note
/// Calling `tag()` on this adaptor is invalid and results in an exception.
///
/// \note
/// The adaptor is intentionally non-serializable.
///
template<RegressionPredictor O>
class reg_oracle_adaptor final : public basic_oracle
{
public:
  explicit reg_oracle_adaptor(O);

  [[nodiscard]] value_t operator()(const std::vector<value_t> &) const override;

  /// This adaptor does not impose additional validity constraints beyond
  /// those of the underlying predictor, therefore it always returns `true`.
  [[nodiscard]] bool is_valid() const override { return true; }

  [[nodiscard]] std::string name(const value_t &v) const override;

private:
  [[noreturn]] classification_result tag(
    const std::vector<value_t> &) const override;
  [[nodiscard]] std::string serialize_id() const override;
  bool save(std::ostream &) const override;

  O oracle_;
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

///
/// The basic interface of a classification oracle.
///
/// This class:
/// - extends the interface of class basic_oracle to handle typical
///   requirements of classification tasks;
/// - factorizes out some code from specific classification schemes;
/// - optionally stores class names.
///
class basic_class_oracle : public basic_oracle
{
public:
  [[nodiscard]] value_t operator()(const std::vector<value_t> &) const final;

  [[nodiscard]] std::string name(const value_t &) const final;

protected:
  explicit basic_class_oracle(std::vector<std::string> = {});

  [[nodiscard]] static std::vector<std::string> class_names(const dataframe &);

  /// \return number of stored class names
  [[nodiscard]] std::size_t names_size() const noexcept { return names_.size(); }

  bool load_names(std::istream &);
  bool save_names(std::ostream &) const;

private:
  std::vector<std::string> names_ {};
};

///
/// Oracle for the Gaussian Distribution Classification.
///
/// \tparam I individual
/// \tparam S stores the individual inside vs keep a reference only
///
/// \see
/// ultra::src::gaussian_evaluator for further details.
///
template<Individual I, bool S>
class basic_gaussian_oracle : public basic_class_oracle
{
public:
  basic_gaussian_oracle(const I &, const dataframe &,
                        std::vector<std::string> = {});
  basic_gaussian_oracle(const I &, const dataframe &,
                        std::vector<classification_result> &,
                        std::vector<std::string> = {});
  basic_gaussian_oracle(std::istream &, const symbol_set &);

  [[nodiscard]] classification_result tag(
    const std::vector<value_t> &) const final;

  [[nodiscard]] bool is_valid() const final;

  // ---- Serialization ----
  static const std::string SERIALIZE_ID;
  bool save(std::ostream &) const final;

private:
  basic_gaussian_oracle(const I &, const dataframe &,
                        std::vector<classification_result> *,
                        std::vector<std::string>);

  // ---- Private support methods ----
  [[nodiscard]] static std::optional<double> normalise_output(const value_t &);
  [[nodiscard]] classification_result calculate_missing_fallback() const;
  [[nodiscard]] classification_result predict(double) const;
  [[nodiscard]] classification_result predict(std::optional<double> out) const
  {
    return out ? predict(*out) : missing_fallback_;
  }

  void fill_vector(const dataframe &, std::vector<classification_result> *);

  [[nodiscard]] std::string serialize_id() const noexcept final
  { return SERIALIZE_ID; }

  // ---- Private data members ----
  basic_reg_oracle<I, S> oracle_;

  // `gauss_dist[i]` contains the gaussian distribution of the i-th class of
  // the classification problem.
  std::vector<distribution<double>> gauss_dist_ {};

  classification_result missing_fallback_ {};
};

///
/// Oracle for Binary Classification.
///
/// \tparam I individual
/// \tparam S stores the individual inside vs keep a reference only
///
/// This class transforms individuals into oracles that can be used for
/// binary classification tasks.
///
template<Individual I, bool S>
class basic_binary_oracle : public basic_class_oracle
{
public:
  basic_binary_oracle(const I &, const dataframe &,
                      std::vector<std::string> = {});
  basic_binary_oracle(std::istream &, const symbol_set &);

  [[nodiscard]] classification_result tag(
    const std::vector<value_t> &) const final;

  [[nodiscard]] bool is_valid() const final;

  // ---- Serialization ----
  static const std::string SERIALIZE_ID;
  bool save(std::ostream &) const final;

private:
  [[nodiscard]] std::string serialize_id() const noexcept final
  { return SERIALIZE_ID; }

  basic_reg_oracle<I, S> oracle_;
};

///
/// Adapts a classification predictor to the `basic_oracle` interface.
///
/// \tparam O predictor type
///
/// This adaptor allows any user-defined classification predictor to be used
/// wherever a `basic_oracle` is required, without inheritance or dynamic
/// polymorphism.
///
/// The wrapped predictor must satisfy `ClassificationPredictor`; the numeric output
/// is derived from `tag()` by returning the predicted class label.
///
/// \note
/// The adaptor is intentionally non-serializable.
///
template<ClassificationPredictor O>
class class_oracle_adaptor final : public basic_class_oracle
{
public:
  explicit class_oracle_adaptor(O, std::vector<std::string> = {});

  /// This adaptor does not impose additional validity constraints beyond
  /// those of the underlying predictor, therefore it always returns `true`.
  [[nodiscard]] bool is_valid() const override { return true; }

  [[nodiscard]] classification_result tag(
    const std::vector<value_t> &) const override;

private:
  [[nodiscard]] std::string serialize_id() const override;
  bool save(std::ostream &) const override;

  O oracle_;
};

// ***********************************************************************
// * Extensions to support teams                                          *
// ***********************************************************************

///
/// An helper class for extending classification schemes to teams.
///
/// \tparam O member oracle type
/// \tparam C composition method for team's member responses
///
template<ClassificationPredictor O,
         team_composition C = team_composition::standard>
class team_class_oracle : public basic_class_oracle
{
public:
  template<Individual I>
  team_class_oracle(const gp::team<I> &, const dataframe &,
                    std::vector<std::string> = {});
  template<Individual I>
  team_class_oracle(const gp::team<I> &, const dataframe &,
                    std::vector<classification_result> &,
                    std::vector<std::string> = {});
  team_class_oracle(std::istream &, const symbol_set &);

  [[nodiscard]] classification_result tag(
    const std::vector<value_t> &) const final;

  [[nodiscard]] bool is_valid() const final;

  static const std::string SERIALIZE_ID;

private:
  [[nodiscard]] classification_result majority_vote(
    std::span<const unsigned>) const;

  bool save(std::ostream &) const final;
  [[nodiscard]] std::string serialize_id() const noexcept final;

  // The components of the team never store the names of the classes. If we
  // need the names, the master class will memorize them.
  std::vector<O> team_;

  src::class_t classes_ {};
};

///
/// Gaussian Distribution Classification specialization for teams.
///
/// \tparam I type of individual
/// \tparam S stores the individual inside vs keep a reference only
///
template<Individual I, bool S>
class basic_gaussian_oracle<gp::team<I>, S>
  : public team_class_oracle<basic_gaussian_oracle<I, S>>
{
public:
  using basic_gaussian_oracle::team_class_oracle::team_class_oracle;
};

///
/// Binary Classification specialization for teams.
///
/// \tparam I type of individual
/// \tparam S stores the individual inside vs keep a reference only
///
template<Individual I, bool S>
class basic_binary_oracle<gp::team<I>, S>
  : public team_class_oracle<basic_binary_oracle<I, S>>
{
public:
  using basic_binary_oracle::team_class_oracle::team_class_oracle;
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
class gaussian_oracle : public basic_gaussian_oracle<P, true>
{
public:
  gaussian_oracle(const P &ind, const dataframe &d)
    : basic_gaussian_oracle<P, true>(ind, d, basic_class_oracle::class_names(d))
  {}

  gaussian_oracle(std::istream &in, const symbol_set &ss)
    : basic_gaussian_oracle<P, true>(in, ss)
  {}
};
template<Individual P> gaussian_oracle(const P &, const dataframe &)
  -> gaussian_oracle<P>;

template<Individual P>
class binary_oracle : public basic_binary_oracle<P, true>
{
public:
  binary_oracle(const P &ind, const dataframe &d)
    : basic_binary_oracle<P, true>(ind, d, basic_class_oracle::class_names(d))
  {}

  binary_oracle(std::istream &in, const symbol_set &ss)
    : basic_binary_oracle<P, true>(in, ss)
  {}
};
template<Individual P> binary_oracle(const P &, const dataframe &)
  -> binary_oracle<P>;

#include "kernel/gp/src/oracle.tcc"

}  // namespace ultra::src

#endif  // include guard
