/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_DATAFRAME_H)
#  error "Don't include this file directly, include the specific .h instead"
#endif

#if !defined(ULTRA_DATAFRAME_TCC)
#define      ULTRA_DATAFRAME_TCC

namespace internal
{

[[nodiscard]] inline value_t lexical_cast(value_t v, domain_t d)
{
  switch (d)
  {
  case d_int:    return lexical_cast<D_INT>(v);
  case d_double: return lexical_cast<D_DOUBLE>(v);
  case d_string: return lexical_cast<D_STRING>(v);
  default:  throw exception::data_format("Invalid domain for lexical_cast");
  }
}

template<class T>
void inplace_trim(T &v)
{
  if constexpr (std::same_as<T, std::string>)
    v = trim(v);

  if constexpr (std::same_as<T, value_t>)
    if (auto *p = std::get_if<D_STRING>(&v))
      *p = trim(*p);
}

}  // namespace internal

///
/// \param[in] r a range containing the example
/// \return      `r` converted to `example` type
///
template<std::ranges::range R>
example dataframe::to_example(const R &r) const
{
  Expects(!std::ranges::empty(r));
  Expects(static_cast<std::size_t>(std::ranges::distance(r)) == columns.size());

  example ret;
  ret.input.reserve(columns.size() - 1);

  for (std::size_t i(0); auto feature : r)
  {
    if (const auto domain(columns[i].domain()); domain != d_void)
    {
      assert(basic_data_type(domain));
      internal::inplace_trim(feature);

      if (i == 0)
      {
        if (task() == task_t::classification)
          ret.output = feature;
        else
          ret.output = internal::lexical_cast(feature, domain);
      }
      else  // input value
        ret.input.push_back(internal::lexical_cast(feature, domain));
    }

    ++i;
  }

  return ret;
}

///
/// \param[in] r            an example in raw format
/// \param[in] output_index index of the output column
/// \param[in] add_instance should we automatically add instances for text
///                         features?
/// \return                 `true` for a correctly converted/imported record
///
template<DataframeRow R>
bool dataframe::read_record(R r, std::optional<std::size_t> output_index,
                            bool add_instance)
{
  Expects(!std::ranges::empty(r));

  const std::size_t row_size(std::ranges::distance(r));

  if (output_index && *output_index >= row_size)
  {
    ultraWARNING << "Malformed example " << size()
                 <<  " skipped (wrong column count)";
    return false;
  }

  const auto logical_size(
    internal::normalised_column_count(row_size, output_index));

  if (logical_size != columns.size())
  {
    ultraWARNING << "Malformed example " << size()
                 <<  " skipped (wrong column count)";
    return false;
  }

  const internal::normalised_row_view normalised(r, output_index);

  push_back(to_example(normalised), add_instance);
  return true;
}

///
/// Populate the dataframe from an in-memory range.
///
/// \param[in] container the input data
/// \param[in] p         parsing and typing parameters. Only `p.data_typing`
///                      and `p.output_index` are used
/// \return              the number of rows stored in the dataframe (`0` in
///                      case of error)
///
/// This function clears existing data, infers the schema using
/// `columns_info::build` and then loads all rows into the dataframe.
///
template<DataframeMatrix R>
std::size_t dataframe::read(const R &container, params p)
{
  columns.data_typing(p.data_typing);

  clear();

  if (std::ranges::size(container) <= 1)
    return 0;

  auto first(std::ranges::begin(container));

  if (p.output_index == params::index::back)
    p.output_index = std::ranges::size(*first) - 1;

  columns.build(container, p.output_index);

  for (auto it(std::next(first)); it != std::ranges::end(container); ++it)
    read_record(*it, p.output_index, true);

  if (!is_valid())
    throw exception::insufficient_data("Empty / invalid data table");

  return size();
}

template<DataframeMatrix R> std::size_t dataframe::read(const R &container)
{
  return read(container, {});
}

///
/// Construct a dataframe from a matrix-like range.
///
/// \tparam R a `DataframeMatrix` type
///
/// \param[in] t the data source
/// \param[in] p additional, optional, parameters (see `params` structure)
///
/// \remark
/// The first row is interpreted as column headers unless overridden via
/// parameters.
///
template<DataframeMatrix R> dataframe::dataframe(const R &t, params p)
{
  read(t, p);
  Ensures(is_valid());
}

///
/// Construct a dataframe from a matrix-like range.
///
/// \tparam R a `DataframeMatrix` type
///
/// \param[in] t the data source
///
/// \remark
/// The first row of the table must contain headers.
///
template<DataframeMatrix R> dataframe::dataframe(const R &t)
  : dataframe(t, {})
{
}

#endif  // include guard
