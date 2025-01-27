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

#include <algorithm>

#include "kernel/gp/src/dataframe.h"
#include "kernel/exceptions.h"
#include "kernel/random.h"
#include "utility/log.h"

#include "tinyxml2/tinyxml2.h"

namespace ultra::src
{

namespace
{

// \param[in] s the string to be converted
// \param[in] d what type should `s` be converted in?
// \return      the converted data or an empty value (`std::monostate`) if no
///             conversion can be applied
//
// `convert("123.1", D_DOUBLE) == value_t(123.1f)`
[[nodiscard]] value_t convert(const std::string &s, domain_t d)
{
  switch (d)
  {
  case d_int:    return std::stoi(s);
  case d_double: return std::stod(s);
  case d_string: return            s;
  default:       return           {};
  }
}

}  // namespace

///
/// Gets the `class_t` ID (aka label) for a given example.
///
/// \param[in] e an example
/// \return      the label of the example
///
/// \warning
/// Used only in classification tasks.
///
/// \related example
///
[[nodiscard]] class_t label(const example &e)
{
  Expects(std::holds_alternative<D_INT>(e.output));
  return std::get<D_INT>(e.output);
}

///
/// \param[in] n the name of a weka domain
/// \return      the internal id corresponding to  the weka-domain `n`
///              (`D_VOID` if unknown / not managed)
///
[[nodiscard]] domain_t from_weka(const std::string &n)
{
  static const std::map<std::string, domain_t> map(
  {
    {"integer", d_int},

    // Real and numeric are treated as double precision number (d_double).
    {"numeric", d_double},
    {"real",    d_double},

    // Nominal values are defined by providing a list of possible values.
    {"nominal", d_string},

    // String attributes allow us to create attributes containing arbitrary
    // textual values. This is very useful in text-mining applications.
    {"string",  d_string}

    // {"date", ?}, {"relational", ?}
  });

  const auto &i(map.find(n));
  return i == map.end() ? d_void : i->second;
}

///
/// New dataframe instance containing the learning collection from a stream.
///
/// \param[in] is input stream
/// \param[in] p  additional, optional, parameters (see `params` structure)
///
/// \remark
/// Data from the input stream must be in CSV format.
///
dataframe::dataframe(std::istream &is, const params &p)
{
  Expects(is.good());
  read_csv(is, p);
  Ensures(is_valid());
}
dataframe::dataframe(std::istream &is) : dataframe(is, {}) {}


///
/// New datafame instance containing the learning collection from a file.
///
/// \param[in] fn name of the file containing the learning collection (CSV /
///               XRFF format)
/// \param[in] p  additional, optional, parameters (see `params` structure)
///
dataframe::dataframe(const std::filesystem::path &fn, const params &p)
{
  Expects(!fn.empty());
  read(fn, p);
  Ensures(is_valid());
}
dataframe::dataframe(const std::filesystem::path &fn) : dataframe(fn, {}) {}

///
/// Removes all elements from the container.
///
/// Invalidates any references, pointers or iterators referring to contained
/// examples. Any past-the-end iterators are also invalidated.
///
/// Leaves the associated metadata unchanged.
///
void dataframe::clear()
{
  dataset_.clear();
}

///
/// \return reference to the first element of the active dataset
///
dataframe::iterator dataframe::begin()
{
  return dataset_.begin();
}

///
/// \return a constant reference to the first element of the dataset
///
dataframe::const_iterator dataframe::begin() const
{
  return dataset_.begin();
}

///
/// \return a reference to the sentinel element of the active dataset
///
dataframe::iterator dataframe::end()
{
  return dataset_.end();
}

///
/// \return a constant reference to the sentinel element of the active dataset
///
dataframe::const_iterator dataframe::end() const
{
  return dataset_.end();
}

///
/// Returns a constant reference to the first element in the dataframe.
///
/// \return a constant reference to the first element int the dataframe
///
/// \remark Calling `front` on an empty dataframe is undefined.
///
dataframe::value_type dataframe::front() const
{
  return dataset_.front();
}

///
/// Returns a reference to the first element in the dataframe.
///
/// \return a reference to the first element in the dataframe
///
/// \remark Calling `front` on an empty dataframe is undefined.
///
dataframe::value_type &dataframe::front()
{
  return dataset_.front();
}

///
/// \return the size of the active dataset
///
std::size_t dataframe::size() const noexcept
{
  return dataset_.size();
}

///
/// \return `true` if the dataframe is empty
///
bool dataframe::empty() const noexcept
{
  return dataset_.empty();
}

///
/// \return number of classes of the problem (`== 0` for a symbolic regression
///         problem, `> 1` for a classification problem)
///
class_t dataframe::classes() const noexcept
{
  return static_cast<class_t>(classes_map_.size());
}

///
/// \return input vector dimension
///
/// \note
/// If the dataset is not empty, `variables() + 1 == columns.size()`.
///
unsigned dataframe::variables() const
{
  const auto n(empty() ? 0u : static_cast<unsigned>(begin()->input.size()));

  Ensures(empty() || n + 1 == columns.size());
  return n;
}

///
/// Appends the given element to the end of the active dataset.
///
/// \param[in] e the value of the element to append
///
void dataframe::push_back(const example &e)
{
  dataset_.push_back(e);
}

///
/// \param[in] label name of a class of the learning collection
/// \return          the (numerical) value associated with class `label`
///
class_t dataframe::encode(const std::string &label)
{
  if (!classes_map_.contains(label))
    classes_map_[label] = classes();

  return classes_map_[label];
}

///
/// \param[in] v            the raw example (features encoded as `std::string`s)
/// \param[in] add_instance should we automatically add instances for text
///                         features?
/// \return                 `v` converted to `example` type
///
/// \remark
/// When `add_instance` is `true` the function can have side-effects (changing
/// the set of admissible instances associated with a text-feature).
///
example dataframe::to_example(const record_t &v, bool add_instance)
{
  Expects(v.size());
  Expects(v.size() == columns.size());

  example ret;

  for (std::size_t i(0); i < v.size(); ++i)
    if (const auto domain(columns[i].domain()); domain != d_void)
    {
      const auto feature(trim(v[i]));

      if (i == 0)
      {
        const bool classification(!is_number(v.front()));

        // Strings could be used as label for classes, but integers
        // are simpler and faster to manage (arrays instead of maps).
        if (classification)
          ret.output = static_cast<D_INT>(encode(feature));
        else
          ret.output = convert(feature, domain);
      }
      else  // input value
        ret.input.push_back(convert(feature, domain));

      if (add_instance && domain == d_string)
        columns[i].add_state(feature);
    }

  return ret;
}

///
/// \param[in] r            an example in raw format
/// \param[in] add_instance should we automatically add instances for text
///                         features?
/// \return                 `true` for a correctly converted/imported record
///
bool dataframe::read_record(const record_t &r, bool add_instance)
{
  Expects(r.size());

  if (r.size() != columns.size())  // skip lines with wrong number of columns
  {
    ultraWARNING << "Malformed exampled " << size() <<  " skipped";
    return false;
  }

  push_back(to_example(r, add_instance));
  return true;
}

///
/// \param[in] i the encoded (dataframe::encode()) value of a class
/// \return      the name of the class encoded by `i` (or an empty string if
///              such class cannot be find)
///
std::string dataframe::class_name(class_t i) const noexcept
{
  for (const auto &p : classes_map_)
    if (p.second == i)
      return p.first;

  return {};
}

///
/// Loads a XRFF file from a file into the dataframe.
///
/// \param[in] fn the xrff filename
/// \param[in] p  additional, optional, parameters (see `params` structure)
/// \return       number of lines parsed (`0` in case of errors)
///
/// \exception exception::data_format wrong data format for data file
///
/// \see
/// `dataframe::load_xrff(tinyxml2::XMLDocument &)` for details.
///
std::size_t dataframe::read_xrff(const std::filesystem::path &fn,
                                 const params &p)
{
  tinyxml2::XMLDocument doc;
  if (doc.LoadFile(fn.string().c_str()) != tinyxml2::XML_SUCCESS)
    throw exception::data_format("XRFF data file format error");

  return read_xrff(doc, p);
}

///
/// Loads a XRFF file from a stream into the dataframe.
///
/// \param[in] in the xrff stream
/// \param[in] p  additional, optional, parameters (see `params` structure)
/// \return       number of lines parsed (`0` in case of errors)
///
/// \exception exception::data_format wrong data format for data file
///
/// \see
/// `dataframe::read_xrff(tinyxml2::XMLDocument &)` for details.
///
std::size_t dataframe::read_xrff(std::istream &in, const params &p)
{
  std::ostringstream ss;
  ss << in.rdbuf();

  tinyxml2::XMLDocument doc;
  if (doc.Parse(ss.str().c_str()) != tinyxml2::XML_SUCCESS)
    throw exception::data_format("XRFF data file format error");

  return read_xrff(doc, p);
}
std::size_t dataframe::read_xrff(std::istream &in)
{
  return read_xrff(in, {});
}

///
/// Loads a XRFF document into the active dataset.
///
/// \param[in] doc object containing the xrff file
/// \param[in] p   additional, optional, parameters (see `params` structure)
/// \return        number of lines parsed (`0` in case of errors)
///
/// \exception exception::data_format wrong data format for data file
///
/// An XRFF (eXtensible attribute-Relation File Format) file describes a list
/// of instances sharing a set of attributes.
/// The original format is defined in
/// https://waikato.github.io/weka-wiki/formats_and_processing/xrff/
///
/// \warning
/// To date we don't support compressed and sparse format XRFF files.
///
std::size_t dataframe::read_xrff(tinyxml2::XMLDocument &doc, const params &p)
{
  columns.data_typing(p.data_typing);

  // Iterate over `dataset.header.attributes` selection and store all found
  // attributes in the header vector.
  tinyxml2::XMLHandle handle(&doc);
  auto *attributes(handle.FirstChildElement("dataset")
                         .FirstChildElement("header")
                         .FirstChildElement("attributes").ToElement());
  if (!attributes)
    throw exception::data_format("Missing `attributes` element in XRFF file");

  clear();

  unsigned n_output(0), output_index(0), index(0);

  for (auto *attribute(attributes->FirstChildElement("attribute"));
       attribute;
       attribute = attribute->NextSiblingElement("attribute"), ++index)
  {
    columns_info::column_info a(columns);

    if (const char *s = attribute->Attribute("name"))
      a.name(s);

    // One can define which attribute should act as output value via the
    // `class="yes"` attribute in the attribute specification of the header.
    const bool output(attribute->Attribute("class", "yes"));

    const char *s(attribute->Attribute("type"));
    std::string xml_type(s ? s : "");

    if (output)
    {
      ++n_output;

      output_index = index;

      // We can manage only one output column.
      if (n_output > 1)
        throw exception::data_format("Multiple output columns in XRFF file");

      // For classification problems we use discriminant functions, so the
      // actual output type is always numeric.
      if (xml_type == "nominal" || xml_type == "string")
        xml_type = "numeric";
    }

    a.domain(from_weka(xml_type));

    // Store label1... labelN.
    if (xml_type == "nominal")
      for (auto *l(attribute->FirstChildElement("label"));
           l;
           l = l->NextSiblingElement("label"))
      {
        const std::string label(l->GetText() ? l->GetText() : "");
        a.add_state(label);
      }

    // Output column is always the first one.
    if (output)
      columns.push_front(a);
    else
      columns.push_back(a);
  }

  // XRFF needs information about the columns.
  if (columns.empty())
    throw exception::data_format("Missing column information in XRFF file");

  // If no output column is specified the default XRFF output column is the
  // last one (and it's the first element of the `header_` vector).
  if (n_output == 0)
  {
    columns.push_front(columns.back());
    columns.pop_back();
    output_index = index - 1;
  }

  if (auto *instances = handle.FirstChildElement("dataset")
                        .FirstChildElement("body")
                        .FirstChildElement("instances").ToElement())
  {
    for (auto *i(instances->FirstChildElement("instance"));
         i;
         i = i->NextSiblingElement("instance"))
    {
      record_t record;

      for (auto *v(i->FirstChildElement("value"));
           v;
           v = v->NextSiblingElement("value"))
        record.push_back(v->GetText() ? v->GetText() : "");

      if (p.filter && p.filter(record) == false)
        continue;

      std::rotate(record.begin(),
                  std::next(record.begin(), output_index),
                  std::next(record.begin(), output_index + 1));

      read_record(record, false);
    }
  }
  else
    throw exception::data_format("Missing `instances` element in XRFF file");

  return is_valid() ? size() : static_cast<std::size_t>(0);
}

///
/// Loads a CSV file into the active dataset.
///
/// \param[in] fn the csv filename
/// \param[in] p  additional, optional, parameters (see `params` structure)
/// \return       number of lines parsed (0 in case of errors)
///
/// \exception std::runtime_error cannot read CSV data file
///
/// \see
/// `dataframe::load_csv(const std::string &)` for details.
///
std::size_t dataframe::read_csv(const std::filesystem::path &fn,
                                const params &p)
{
  std::ifstream in(fn);
  if (!in)
    throw std::runtime_error("Cannot read CSV data file");

  return read_csv(in, p);
}

///
/// Loads a CSV file into the active dataset.
///
/// \param[in] from the csv stream
/// \param[in] p    additional, optional, parameters (see `params` structure)
/// \return         number of lines parsed (0 in case of errors)
///
/// \exception exception::insufficient_data empty / undersized data file
///
/// General conventions:
/// - NO HEADER ROW is allowed;
/// - only one example is allowed per line. A single example cannot contain
///   newlines and cannot span multiple lines.
///   Note than CSV standard (e.g.
///   http://en.wikipedia.org/wiki/Comma-separated_values) allows for the
///   newline character `\n` to be part of a csv field if the field is
///   surrounded by quotes;
/// - columns are separated by commas. Commas inside a quoted string aren't
///   column delimiters;
/// - the column containing the labels (numeric or string) for the examples can
///   be specified by the user; if not specified, the the first column is the
///   default. If the label is numeric Ultra assumes a REGRESSION model; if it's
///   a string, a CATEGORIZATION (i.e. classification) model is assumed.
/// - each column must describe the same kind of information;
/// - the column order of features in the table does not weight the results.
///   The first feature is not weighted any more than the last;
/// - as a best practice, remove punctuation (other than apostrophes) from
///   your data. This is because commas, periods and other punctuation
///   rarely add meaning to the training data, but are treated as meaningful
///   elements by the learning engine. For example, "end." is not matched to
///   "end";
/// - TEXT STRINGS:
///   - place double quotes around all text strings;
///   - text matching is case-sensitive: "wine" is different from "Wine.";
///   - if a string contains a double quote, the double quote must be escaped
///     with another double quote, for example:
///     "sentence with a ""double"" quote inside";
/// - NUMERIC VALUES:
///   - both integer and decimal values are supported;
///   - numbers in quotes without whitespace will be treated as numbers, even
///     if they are in quotation marks. Multiple numeric values within
///     quotation marks in the same field will be treated as a string. For
///     example:
///       Numbers: "2", "12", "236"
///       Strings: "2 12", "a 23"
///
/// \note
/// Test set can have an empty output value.
///
std::size_t dataframe::read_csv(std::istream &from, params p)
{
  columns.data_typing(p.data_typing);

  clear();

  if (p.dialect.has_header == pocket_csv::dialect::GUESS_HEADER
      || !p.dialect.delimiter)
  {
    const auto sniff(pocket_csv::sniffer(from));

    if (p.dialect.has_header == pocket_csv::dialect::GUESS_HEADER)
      p.dialect.has_header = sniff.has_header;
    if (!p.dialect.delimiter)
      p.dialect.delimiter = sniff.delimiter;
  }

  std::size_t count(0);
  for (auto record : pocket_csv::parser(from, p.dialect).filter_hook(p.filter))
  {
    if (p.output_index)
    {
      if (p.output_index == params::index::back)
        p.output_index = record.size() - 1;

      assert(p.output_index < record.size());

      if (p.output_index > 0)
        std::rotate(record.begin(),
                    std::next(record.begin(), *p.output_index),
                    std::next(record.begin(), *p.output_index + 1));
    }
    else
      // When the output index is unspecified, all the columns are treated as
      // input columns (this is obtained adding a surrogate, empty output
      // column).
      record.insert(record.begin(), "");

    // Every new record may add further information about the column domain.
    const bool has_header(p.dialect.has_header
                          == pocket_csv::dialect::HAS_HEADER);

    if (count < 10)
      columns.build(record, has_header);
    if (has_header == false || count)
      read_record(record, true);

    ++count;
  }

  if (!is_valid() || !size())
    throw exception::insufficient_data("Empty / undersized CSV data file");

  return size();
}
std::size_t dataframe::read_csv(std::istream &from)
{
  return read_csv(from, {});
}

///
/// Loads the content of a file into the active dataset.
///
/// \param[in] fn name of the file containing the data set (CSV / XRFF format)
/// \param[in] p  additional, optional, parameters (see `params` structure)
/// \return       number of lines parsed
///
/// \exception std::invalid_argument missing dataset file name
///
/// \note
/// Test set can have an empty output value.
///
std::size_t dataframe::read(const std::filesystem::path &fn, const params &p)
{
  if (fn.empty())
    throw std::invalid_argument("Missing dataset filename");

  const auto ext(fn.extension().string());
  const bool xrff(iequals(ext, ".xrff") || iequals(ext, ".xml"));

  return xrff ? read_xrff(fn, p) : read_csv(fn, p);
}
std::size_t dataframe::read(const std::filesystem::path &fn)
{
  return read(fn, {});
}

///
/// \return `true` if the current dataset is empty
///
bool dataframe::operator!() const noexcept
{
  return empty();
}

///
/// Removes specified elements from the dataframe.
///
/// \param[in] first first element of the range
/// \param[in] last  end of the range
/// \return          iterator following the last removed element
///
dataframe::iterator dataframe::erase(iterator first, iterator last)
{
  return dataset_.erase(first, last);
}

///
/// Creates a copy of a given schema in an empty dataframe.
///
/// \param[in] other dataframe we copy schema from
/// \return    `true` if schema has been cloned
///
/// \warning
/// If the current dataframe isn't empty the operation fails.
///
bool dataframe::clone_schema(const dataframe &other)
{
  if (!empty())
    return false;

  columns = other.columns;
  classes_map_ = other.classes_map_;

  return true;
}

///
/// Exchanges the contents and capacity of the container with those of `other`.
///
/// \param[in] other dataframe to exchange the contents with
///
/// \remark
/// All iterators and references remain valid. The `end()` iterator is
/// invalidated.
///
void dataframe::swap(dataframe &other)
{
  auto tmp(columns);
  columns = other.columns;
  other.columns = tmp;

  classes_map_.swap(other.classes_map_);
  dataset_.swap(other.dataset_);
}

///
/// \return `true` if the object passes the internal consistency check
///
bool dataframe::is_valid() const
{
  if (empty())
    return true;

  const auto cl_size(classes());
  // Symbolic regression has 0 classes.
  // Classification requires at least 2 classes.
  if (cl_size == 1)
    return false;

  const auto in_size(front().input.size());

  for (const auto &e : *this)
  {
    if (e.input.size() != in_size)
      return false;

    if (cl_size && label(e) >= cl_size)
      return false;
  }

  return columns.is_valid();
}

///
/// Prints the content of the dataframe on a given stream (markdown format).
///
/// \param[out] os output stream
/// \param[in]  d  dataframe to be printed
/// \return        a reference to the (updated) output stream
///
/// \related dataframe
///
std::ostream &operator<<(std::ostream &os, const dataframe &d)
{
  const auto str_col_info([](const columns_info::column_info &ci)
  {
    const std::string name(ci.name().empty() ? std::string("EMPTY")
                                             : "'" + ci.name() + "'");

    std::string str_domain;
    switch (ci.domain())
    {
    case d_void:     str_domain = "void"; break;
    case d_int:      str_domain = "int"; break;
    case d_double:   str_domain = "double"; break;
    case d_string:   str_domain = "string"; break;
    case d_nullary:  str_domain = "nullary"; break;
    case d_address:  str_domain = "address"; break;
    case d_variable: str_domain = "variable"; break;
    default:         str_domain = "?"; break;
    };

    const auto str_category(std::to_string(ci.category()));

    return name + " " + str_domain + "/" + str_category;
  });

  std::vector<std::size_t> width;
  std::ranges::transform(d.columns, std::back_inserter(width),
                         [str_col_info](const auto &ci)
                         {
                           return str_col_info(ci).length();
                         });

  for (const auto &col : d.columns)
    os << "| " << str_col_info(col) << ' ';
  os << "|\n";

  for (std::size_t i(0); i < d.columns.size(); ++i)
    os << "| " << std::string(width[i], '-') << ' ';
  os << "|\n";

  for (const auto &example : d)
  {
    std::size_t i(0);

    os << "| " << std::setw(width[i++]) << example.output;

    for (const auto &cell : example.input)
      os << " | " << std::setw(width[i++]) << cell;

    os << " |\n";
  }

  return os;
}

}  // namespace ultra
