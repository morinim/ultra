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

#if !defined(ULTRA_DATAFRAME_H)
#define      ULTRA_DATAFRAME_H

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "kernel/distribution.h"
#include "kernel/problem.h"
#include "kernel/gp/src/columns_info.h"
#include "utility/pocket_csv.h"

namespace tinyxml2 { class XMLDocument; }

namespace ultra::src
{

/// The type used as class ID in classification tasks.
using class_t = std::size_t;

///
/// Stores a single element (row) of the dataset.
///
/// The `struct` consists of an input vector (`input`) and an answer value
/// (`output`). Depending on the kind of problem, `output` stores:
/// - a numeric value (symbolic regression problem);
/// - a categorical value (classification problem).
///
struct example
{
  /// The thing about which we want to make a prediction (aka instance). The
  /// elements of the vector are features.
  std::vector<value_t> input {};
  /// The answer for the prediction task either the answer produced by the
  /// machine learning system, or the right answer supplied in the training
  /// data.
  value_t output {};

  [[nodiscard]] bool operator==(const example &) const noexcept = default;
};

[[nodiscard]] class_t label(const src::example &);

///
/// A 2-dimensional labeled data structure with columns of potentially
/// different types.
///
/// You can think of it like a spreadsheet or SQL table.
///
/// Dataframe:
/// - is modelled on the corresponding *pandas* object;
/// - is a forward iterable collection of "monomorphic" examples (all samples
///   have the same type and arity);
/// - accepts different kinds of input (CSV and XRFF files).
///
/// \see https://github.com/morinim/ultra/wiki/dataframe
///
class dataframe
{
public:
  // ---- Structures ----
  struct params;

  // ---- Aliases ----
  using examples_t = std::vector<src::example>;
  using value_type = examples_t::value_type;

  // ---- Constructors ----
  dataframe() = default;
  explicit dataframe(std::istream &);
  dataframe(std::istream &, const params &);
  explicit dataframe(const std::filesystem::path &);
  dataframe(const std::filesystem::path &, const params &);

  // ---- Iterators ----
  using iterator = examples_t::iterator;
  using const_iterator = examples_t::const_iterator;
  using difference_type = examples_t::difference_type;

  [[nodiscard]] iterator begin();
  [[nodiscard]] const_iterator begin() const;
  [[nodiscard]] iterator end();
  [[nodiscard]] const_iterator end() const;

  [[nodiscard]] value_type front() const;
  [[nodiscard]] value_type &front();

  // ---- Modifiers ----
  void clear();
  iterator erase(iterator, iterator);

  // ---- Convenience ----
  std::size_t read(const std::filesystem::path &);
  std::size_t read(const std::filesystem::path &, const params &);
  std::size_t read_csv(std::istream &);
  std::size_t read_csv(std::istream &, params);
  std::size_t read_xrff(std::istream &);
  std::size_t read_xrff(std::istream &, const params &);
  [[nodiscard]] bool operator!() const noexcept;

  void push_back(const src::example &);
  template<class InputIt> iterator insert(const_iterator, InputIt, InputIt);

  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] class_t classes() const noexcept;
  [[nodiscard]] unsigned variables() const;

  [[nodiscard]] std::string class_name(class_t) const noexcept;

  [[nodiscard]] bool is_valid() const;

  src::columns_info columns {};

private:
  // Raw input record.
  // The ETL chain is:
  // > FILE -> record_t -> example --(push_back)--> dataframe
  using record_t = std::vector<std::string>;

  bool read_record(const record_t &, bool);
  [[nodiscard]] src::example to_example(const record_t &, bool);

  [[nodiscard]] class_t encode(const std::string &);

  std::size_t read_csv(const std::filesystem::path &, const params &);
  std::size_t read_xrff(const std::filesystem::path &, const params &);
  std::size_t read_xrff(tinyxml2::XMLDocument &, const params &);

  // Integer are simpler to manage than textual data, so, when appropriate,
  // input strings are converted into integers by this map and the `encode`
  // static function.
  std::map<std::string, class_t> classes_map_ {};

  // Available data.
  examples_t dataset_ {};
};

[[nodiscard]] domain_t from_weka(const std::string &);

///
/// Get the output value for a given example.
///
/// \tparam    T the result is casted to type `T`
/// \param[in] e an example
/// \return      the output value for the given example
///
///
template<class T>
[[nodiscard]] T label_as(const src::example &e)
{
  return lexical_cast<T>(e.output);
}

struct dataframe::params
{
  enum index : std::size_t {front = 0,
                            back = std::numeric_limits<std::size_t>::max() };

  /// See `typing` for details.
  typing data_typing {typing::weak};

  /// \remark
  /// Used only when reading CSV files.
  pocket_csv::dialect dialect {};

  /// A filter and transform function (returns `true` for records that should
  /// be loaded and, possibly, transforms its input parameter).
  pocket_csv::parser::filter_hook_t filter {nullptr};

  /// Index of the column containing the output value (label).
  /// \remark
  /// Used only when reading CSV files.
  std::optional<std::size_t> output_index {0};

  params &header() noexcept
  { dialect.has_header = pocket_csv::dialect::HAS_HEADER; return *this; }
  params &no_header() noexcept
  { dialect.has_header = pocket_csv::dialect::NO_HEADER; return *this; }

  params &output(std::size_t o) { output_index = o; return *this; }
  params &no_output() noexcept { output_index = std::nullopt; return *this; }

  params &strong_data_typing() noexcept
  { data_typing = typing::strong; return *this; }
  params &weak_data_typing() noexcept
  { data_typing = typing::weak; return *this; }
};

std::ostream &operator<<(std::ostream &, const dataframe &);

///
/// Inserts elements from range `[first, last[` before `pos`.
///
/// \param[in] pos   iterator before which the content will be inserted (may be
///                  the `end()` iterator)
/// \param[in] first first iterator of the range of elements to insert, cannot
///                  be iterator into container for which `insert` is called
/// \param[in] last  last iterator of the range of elements to insert, cannot
///                  be iterator into container for which `insert` is called
/// \return          iterator pointing to the first element inserted, or `pos`
///                  if `first == last`
///
/// \remark
/// If, after the operation, the new `size()` is greater than old `capacity()`
/// areallocation takes place, in which case all iterators (including the
/// `end()` iterator) and all references to the elements are invalidated;
/// otherwise, only the iterators and references before the insertion point
/// remain valid.
///
template<class InputIt>
dataframe::iterator dataframe::insert(dataframe::const_iterator pos,
                                      InputIt first, InputIt last)
{
  return dataset_.insert(pos, first, last);
}

}  // namespace ultra::src

#endif  // include guard
