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

#include "kernel/distribution.h"
#include "kernel/exceptions.h"
#include "kernel/problem.h"
#include "kernel/gp/src/columns_info.h"
#include "utility/pocket_csv.h"

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace tinyxml2 { class XMLDocument; }

namespace ultra::src
{

/// The type used as class ID in classification tasks.
using class_t = std::size_t;

/// A raw observation or data entry, typically read from an input source.
///
/// The ETL chain is:
/// INPUT SOURCE   -> raw record -> processed example --(push_back)-> dataframe
/// `raw_data`/file   `record_t`    `example`
using record_t = std::vector<value_t>;

using raw_data = std::vector<record_t>;

///
/// Stores a single processed element (row) of the dataset.
///
/// The `example` struct consists of an input vector (`input`) and an
/// outoput value (`output`). Depending on the task, `output` holds:
/// - a numeric value (e.g. in symbolic regression);
/// - a categorical label (e.g. in classification).
///
struct example
{
  /// The instance we want to make a prediction about. Each element in the
  /// vector represents a feature.
  std::vector<value_t> input {};
  /// The expected output for the prediction task: either the predicted
  /// value or the correct label from training data.
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
  template<RangeOfSizedRanges R> dataframe(const R &);
  template<RangeOfSizedRanges R> dataframe(const R &, params);

  // ---- Iterators ----
  using iterator = examples_t::iterator;
  using const_iterator = examples_t::const_iterator;
  using difference_type = examples_t::difference_type;

  [[nodiscard]] iterator begin();
  [[nodiscard]] const_iterator begin() const;
  [[nodiscard]] iterator end();
  [[nodiscard]] const_iterator end() const;

  // ---- Element access ----
  [[nodiscard]] value_type front() const;
  [[nodiscard]] value_type &front();

  // ---- Modifiers ----
  void clear();
  iterator erase(iterator, iterator);
  void push_back(const src::example &);
  template<std::input_iterator InputIt> iterator insert(const_iterator,
                                                        InputIt, InputIt);
  void swap(dataframe &);

  // ---- Convenience ----
  std::size_t read(const std::filesystem::path &);
  std::size_t read(const std::filesystem::path &, const params &);
  std::size_t read(std::istream &);
  std::size_t read(std::istream &, params);
  template<RangeOfSizedRanges R> std::size_t read(const R &);
  template<RangeOfSizedRanges R> std::size_t read(const R &, params);
  [[nodiscard]] bool operator!() const noexcept;

  bool clone_schema(const dataframe &);

  // ---- Capacity ----
  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] bool empty() const noexcept;

  [[nodiscard]] class_t classes() const noexcept;
  [[nodiscard]] unsigned variables() const;

  // ---- Misc ----
  [[nodiscard]] std::string class_name(class_t) const noexcept;

  [[nodiscard]] bool is_valid() const;

  // ---- Public data members ----
  src::columns_info columns {};

private:
  template<std::ranges::range R> bool read_record(
    R, std::optional<std::size_t>, bool);

  template<std::ranges::range R>
  [[nodiscard]] example to_example(const R &, bool);

  [[nodiscard]] class_t encode(const value_t &);

  std::size_t read_csv(const std::filesystem::path &, const params &);
  std::size_t read_csv(std::istream &);
  std::size_t read_csv(std::istream &, params);

  std::size_t read_xrff(const std::filesystem::path &, const params &);
  std::size_t read_xrff(std::istream &, params);
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
template<std::input_iterator InputIt>
dataframe::iterator dataframe::insert(dataframe::const_iterator pos,
                                      InputIt first, InputIt last)
{
  return dataset_.insert(pos, first, last);
}

#include "kernel/gp/src/dataframe.tcc"

}  // namespace ultra::src

#endif  // include guard
