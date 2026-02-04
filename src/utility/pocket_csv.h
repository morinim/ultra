/**
 *  \file
 *  \remark This file is part of POCKET_CSV.
 *
 *  \copyright Copyright (C) 2022 Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(POCKET_CSV_PARSER_H)
#define      POCKET_CSV_PARSER_H

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <vector>

namespace pocket_csv
{

namespace internal
{

inline void rewind(std::istream &is)
{
  is.clear();
  is.seekg(0, std::ios::beg);
}

}  // namespace internal

///
/// Information about the CSV dialect.
///
/// *CSV is a textbook example of how not to design a textual file format*.
/// The Art of Unix Programming, Raymond (2003).
///
struct dialect
{
  /// A one-character string used to separate fields. When `0` triggers the
  /// sniffer.
  char delimiter {0};
  /// When `true` skips leading and trailing spaces adjacent to commas.
  bool trim_ws {false};
  /// If `HAS_HEADER` assumes a header row is present; if `GUESS_HEADER`,
  /// sniffs.
  enum header_e {GUESS_HEADER = -1, NO_HEADER = 0, HAS_HEADER = 1} has_header
  {GUESS_HEADER};
  /// Controls whether quotes should be keep by the reader.
  /// - `KEEP_QUOTES`. Always keep the quotes;
  /// - `REMOVE_QUOTES`. Never keep quotes.
  /// It defaults to `REMOVE_QUOTES`.
  enum quoting_e {KEEP_QUOTES, REMOVE_QUOTES} quoting {REMOVE_QUOTES};
};  // class dialect

///
/// Simple parser for CSV files.
///
/// \warning
/// This class does not support multi-line fields.
///
class parser
{
public:
  using record_t = std::vector<std::string>;
  using filter_hook_t = std::function<bool (record_t &)>;

  explicit parser(std::istream &);
  parser(std::istream &, const dialect &);

  [[nodiscard]] const dialect &active_dialect() const noexcept;

  parser &delimiter(char) & noexcept;
  parser delimiter(char) && noexcept;

  parser &quoting(dialect::quoting_e) & noexcept;
  parser quoting(dialect::quoting_e) && noexcept;

  parser &skip_header() & noexcept;
  parser skip_header() && noexcept;

  parser &trim_ws(bool) & noexcept;
  parser trim_ws(bool) && noexcept;

  parser &filter_hook(filter_hook_t) & noexcept;
  parser filter_hook(filter_hook_t) && noexcept;

  class const_iterator;
  [[nodiscard]] const_iterator begin() const;
  [[nodiscard]] const_iterator end() const noexcept;

private:
  // DO NOT move this. `is` must be initialized before other data members.
  std::istream *is_;

  filter_hook_t filter_hook_ {nullptr};
  dialect dialect_;
  bool skip_header_ {false};
};  // class parser

///
/// Input iterator over CSV records.
///
/// This iterator models a **single-pass input iterator** backed by an
/// `std::istream`. Advancing the iterator consumes data from the underlying
/// stream.
///
/// \warning
/// - Copies of this iterator share the same stream state;
/// - comparing two non-end iterators for equality is not meaningful;
/// - the only supported comparison is against the end iterator.
///
class parser::const_iterator
{
public:
  using iterator_category = std::input_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = parser::record_t;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;

  /// Constructs a CSV input iterator.
  ///
  /// \param[in] is pointer to the input stream to read from; if `nullptr`,
  ///               constructs an end iterator
  /// \param[in] f  optional filter function applied to each parsed record;
  ///               records for which the function returns `false` are skipped
  /// \param[in] d  CSV dialect used for parsing records
  ///
  /// \note
  /// When constructed with a non-null stream pointer, the iterator
  /// immediately reads and parses the first available record.
  ///
  /// \warning
  /// This iterator is single-pass: advancing it consumes data from the
  /// underlying stream and copies of the iterator share the same stream
  /// state.
  explicit const_iterator(std::istream *is = nullptr,
                          parser::filter_hook_t f = nullptr,
                          const dialect &d = {})
    : ptr_(is), filter_hook_(f), dialect_(d)
  {
    if (ptr_)
      get_input();
  }

  /// \return an iterator pointing to the next record of the CSV file
  const_iterator &operator++()
  {
    get_input();
    return *this;
  }

  /// \return reference to the current record of the CSV file
  [[nodiscard]] const_reference operator*() const noexcept { return value_; }

  /// \return pointer to the current record of the CSV file
  [[nodiscard]] const_pointer operator->() const noexcept
  { return &operator*(); }

  /// Compares two iterators for inequality.
  ///
  /// \param[in] lhs first iterator
  /// \param[in] rhs second iterator
  /// \return        `true` if the iterators differ
  ///
  /// \note
  /// For this single-pass input iterator, comparison is only meaningful when
  /// testing against the end iterator. Equality of two non-end iterators
  /// does not imply interchangeable or repeatable traversal.
  [[nodiscard]] friend bool operator!=(const const_iterator &lhs,
                                       const const_iterator &rhs) noexcept
  {
    return lhs.ptr_ != rhs.ptr_;
  }

  [[nodiscard]] friend bool operator==(const const_iterator &lhs,
                                       const const_iterator &rhs) noexcept
  {
    return !(lhs.ptr_ != rhs.ptr_);
  }


private:
  value_type parse_line(const std::string &);
  void get_input();

  // Data members MUST BE INITIALIZED IN THE CORRECT ORDER:
  // * `value_` is initialized via the `get_input` member function;
  // * `get_input` works only if `_ptr` is already initialized.
  // So it's important that `value_` is the last value to be initialized.
  std::istream *ptr_;
  parser::filter_hook_t filter_hook_;
  dialect dialect_;
  value_type value_ {};
};  // class parser::const_iterator

namespace internal
{

/// Column classification tag.
///
/// Negative values represent semantic categories inferred for the column:
/// - `number_tag`: numeric column;
/// - `string_tag`: variable-length string column;
/// - `skip_tag`: column to be ignored.
///
/// Non-negative values are used to encode structural information:
/// - `0` (`none_tag`) indicates an unspecified or unknown column type;
/// - a positive value indicates a fixed-length string column, where the value
///   represents the exact string length.
///
/// \note
/// This enum intentionally overloads semantic tags and structural data.
/// In particular, positive values do not denote distinct enum constants but
/// encode the fixed length of string columns.
enum column_tag {none_tag = 0, skip_tag = -1,
                 number_tag = -2, string_tag = -3};

struct char_stat
{
  char_stat(unsigned cf = 0, unsigned w = 0) : char_freq(cf), weight(w) {}

  unsigned char_freq {0};
  unsigned weight {0};
};

///
/// \param[in] s the input string
/// \return      a copy of `s` with spaces removed on both sides of the string
///
/// \see https://stackoverflow.com/a/24425221/3235496
///
[[nodiscard]] inline std::string trim(const std::string &s)
{
  const auto front(std::find_if_not(
                     s.begin(), s.end(),
                     [](unsigned char c) { return std::isspace(c); }));

  // The search is limited in the reverse direction to the last non-space value
  // found in the search in the forward direction.
  const auto back(std::find_if_not(
                    s.rbegin(), std::make_reverse_iterator(front),
                    [](unsigned char c) { return std::isspace(c); }).base());

  return {front, back};
}

///
/// \param[in] s the string to be tested
/// \return      `true` if `s` contains a number
///
[[nodiscard]] inline bool is_number(std::string s)
{
  s = trim(s);

  char *end;
  const double val(std::strtod(s.c_str(), &end));  // if no conversion can be
                                                   // performed, `end` is set
                                                   // to `s.c_str()`
  return end != s.c_str() && *end == '\0' && std::isfinite(val);
}

///
/// Calculates the mode of a sequence of natural numbers.
///
/// \param[in] v a sequence of natural numbers
/// \return    a vector of `char_stat` pairs (the input sequence may have
///            more than one mode)
///
/// \warning
/// Assumes a sorted input vector.
///
[[nodiscard]] inline std::vector<char_stat> mode(const std::vector<unsigned> &v)
{
  assert(std::is_sorted(v.begin(), v.end()));

  if (v.empty())
    return {};

  auto current(v.front());
  unsigned count(1), max_count(1);

  std::vector<char_stat> ret({{current, 1}});

  for (auto i(std::next(v.begin())); i != v.end(); ++i)
  {
    if (*i == current)
      ++count;
    else
    {
      count = 1;
      current = *i;
    }

    if (count > max_count)
    {
      max_count = count;
      ret = {{current, max_count}};
    }
    else if (count == max_count)
      ret.emplace_back(current, max_count);
  }

  return ret;
}

[[nodiscard]] inline column_tag find_column_tag(const std::string &s)
{
  if (const auto ts(internal::trim(s)); ts.empty())
    return none_tag;
  else if (internal::is_number(ts))
    return number_tag;

  // Length is taken from the original field to preserve structural width.
  return static_cast<column_tag>(s.length());
}

[[nodiscard]] inline bool capitalized(std::string s)
{
  s = internal::trim(s);

  return !s.empty() && std::isupper(s.front())
         && std::all_of(std::next(s.begin()), s.end(),
                        [](unsigned char c)
                        {
                          return std::isprint(c)
                                 && (!std::isalpha(c) || std::islower(c));
                        });
}

[[nodiscard]] inline bool lower_case(const std::string &s)
{
  return std::all_of(s.begin(), s.end(),
                     [](unsigned char c)
                     {
                       return !std::isalpha(c) || std::islower(c);
                     });
}

[[nodiscard]] inline bool upper_case(const std::string &s)
{
  return std::all_of(s.begin(), s.end(),
                     [](unsigned char c)
                     {
                       return !std::isalpha(c) || std::isupper(c);
                     });
}

[[nodiscard]] inline dialect::header_e has_header(std::istream &is,
                                                  std::size_t lines,
                                                  char delim)
{
  dialect d;
  d.delimiter = delim;
  d.has_header = dialect::HAS_HEADER;  // assume first row is header (1)
  pocket_csv::parser parser(is, d);

  // Quoting allows to correctly identify a column with header `"1980"` (e.g. a
  // specific year. Notice the double quotes) and values `2012`, `2000`...
  // (the values observed during 1980).
  parser.quoting(pocket_csv::dialect::KEEP_QUOTES);
  const auto header_it(parser.begin());
  if (header_it == parser.end())
  {
    internal::rewind(is);
    return dialect::NO_HEADER;
  }

  const auto header(*header_it);  // assume first row is header (2)
  parser.quoting(pocket_csv::dialect::REMOVE_QUOTES);

  const auto columns(header.size());
  std::vector<internal::column_tag> column_types(columns, none_tag);

  unsigned checked(0);
  for (auto it(std::next(parser.begin())); it != parser.end(); ++it)
    // Skip rows that have irregular number of columns
    if (const auto row = *it; row.size() == columns)
    {
      for (std::size_t field(0); field < columns; ++field)
      {
        if (column_types[field] == skip_tag)  // inconsistent column
          continue;
        if (trim(row[field]).empty())         // missing values
          continue;

        const auto this_tag(find_column_tag(row[field]));
        if (column_types[field] == this_tag)  // matching column type
          continue;

        if (capitalized(header[field]) && lower_case(row[field]))
          column_types[field] = string_tag;
        else if (upper_case(header[field]) && !upper_case(row[field]))
          column_types[field] = string_tag;
        else if (column_types[field] == none_tag)
          column_types[field] = this_tag;
        else
          column_types[field] = skip_tag;  // type is inconsistent, remove
      }                                    // column from consideration

      if (checked++ >= lines)
        break;
    }

  // Finally, compare results against first row and "vote" on whether it's a
  // header.
  int vote_header(0);

  for (std::size_t field(0); field < columns; ++field)
    switch (column_types[field])
    {
    case none_tag:
      if (header[field].length())
        ++vote_header;
      else
        --vote_header;
      break;

    case skip_tag:
      break;

    case number_tag:
      if (!is_number(header[field]))
        ++vote_header;
      else
        --vote_header;
      break;

    case string_tag:  // variable length strings
      ++vote_header;
      break;

    default:  // column containing fixed length strings
      assert(column_types[field] > 0);
      if (const auto length = static_cast<std::size_t>(column_types[field]);
          header[field].length() != length)
        ++vote_header;
      else
        --vote_header;
    }

  internal::rewind(is);

  return vote_header > 0 ? dialect::HAS_HEADER : dialect::NO_HEADER;
}

///
/// Attempts to infer the field delimiter used in a delimited text stream.
///
/// \param[in,out] is input stream to analyse. The stream is read sequentially
///                   and left at the position reached after scanning
/// \param[in] lines  maximum number of non-empty lines to inspect
///
/// \return           the inferred delimiter character, or `\0` if no suitable
///                   delimiter can be determined. A return value of `\0`
///                   indicates that the input is likely a single-column file
///                   with no field delimiter.
///
/// The function scans up to `lines` non-empty lines from the input stream and
/// counts occurrences of a small set of preferred delimiter characters
/// (`,`, `;`, `\t`, `:`, `|`). A delimiter is selected if it:
///
/// - appears a consistent number of times per line (single, non-zero mode);
/// - occurs in at least ~2/3 of the scanned non-empty lines.
///
/// \note
/// Empty or whitespace-only lines are ignored. If no delimiter satisfies
/// the consistency criteria, the function fails conservatively and returns
/// `\0`, which should be interpreted as a single-column input.
///
[[nodiscard]] inline char guess_delimiter(std::istream &is, std::size_t lines)
{
  static const std::vector preferred = {',', ';', '\t', ':', '|'};
  static const std::bitset<256> is_candidate([]
  {
    std::bitset<256> ret;

    for (unsigned char c : preferred)
      ret[c] = true;

    return ret;
  }());

  // `count[c]` is a vector with information about character `c`. It grows
  // one element every time a new input line is read.
  // `count[c][l]` contains the number of times character `c` appears in line
  // `l`.
  // `count` only contains entries for preferred delimiter candidates.
  std::map<char, std::vector<unsigned>> count;

  std::size_t scanned(0);

  for (std::string line; std::getline(is, line) && lines;)
  {
    if (trim(line).empty())
      continue;

    // A new non-empty line. Initially every character has a `0` counter.
    for (unsigned char c : preferred)
      count[c].push_back(0u);

    for (unsigned char c : line)
      if (is_candidate[c])
        ++count[c].back();

    --lines;
    ++scanned;
  }

  if (count.empty())  // empty input file
    return 0;

  // `mode_weight[c]` stores a couple of values specifying:
  // 1. how many time character `c` usually repeats in a line of the CSV file;
  // 2. a weight (the effective number of lines condition 1 is verified).
  std::map<char, char_stat> mode_weight;

  for (auto &[c, cf] : count)
  {
    std::sort(cf.begin(), cf.end());

    const auto mf(mode(cf));

    if (mf.empty() || mf.size() > 1 || mf.front().char_freq == 0)
      mode_weight[c] = {0u, 0u};
    else
      mode_weight[c] = mf.front();
  }

  const auto res(std::max_element(mode_weight.begin(), mode_weight.end(),
                                  [](const auto &l, const auto &r)
                                  {
                                    return l.second.weight < r.second.weight;
                                  }));

  if (res->second.char_freq == 0)
    return 0;

  // Delimiter must consistently appear in the input lines.
  if (3 * res->second.weight < 2 * scanned)
    return 0;

  return res->first;
}

}  // namespace internal

///
/// *Sniffs* the format of a CSV file (delimiter, headers).
///
/// \param[in] is    stream containing CSV data
/// \param[in] lines number of lines used for sniffing data format
/// \return          a `dialect` object
///
/// For detecting the **header** creates a dictionary of types of data in each
/// column. If any column is of a single type (say, integers), *except* for the
/// first row, then the first row is presumed to be labels. If the type cannot
/// be determined, it's assumed to be a string in which case the length of the
/// string is the determining factor: if all of the rows except for the first
/// are the same length, it's a header.
/// Finally, a 'vote' is taken at the end for each column, adding or
/// subtracting from the likelihood of the first row being a header.
///
/// ---
///
/// The delimiter *should* occur the same number of times on each row. However,
/// due to malformed data, it may not. We don't want an all or nothing
/// approach, so we allow for small variations in this number:
///
/// 1. build a table of the frequency of usual delimiters (comma, tab, colon,
///    semicolon, vertical bar) on every line;
/// 2. build a table of frequencies of this frequency (meta-frequency?), e.g.
///    'x occurred 5 times in 10 rows, 6 times in 1000 rows, 7 times in 2
///    rows';
/// 3. use the mode of the meta-frequency to determine the *expected* frequency
///    for that character;
/// 4. find out how often the character actually meets that goal;
/// 5. the character that best meets its goal is the delimiter.
///
/// \note
/// Somewhat inspired by the dialect sniffer developed by Clifford Wells for
/// his Python-DSV package (Wells, 2002) which was incorporated into Python
/// v2.3.
///
[[nodiscard]] inline dialect sniffer(std::istream &is, std::size_t lines = 20)
{
  dialect d;

  d.delimiter = internal::guess_delimiter(is, lines);
  d.has_header = internal::has_header(is, lines, d.delimiter);

  return d;
}

///
/// Initializes the parser trying to sniff the CSV format.
///
/// \param[in] is input stream containing CSV data
///
inline parser::parser(std::istream &is) : parser(is, {})
{
  dialect_ = sniffer(is);
}

///
/// Initializes the parser trying by sniffing the CSV format.
///
/// \param[in] is input stream containing CSV data
/// \param[in] d  dialect used for CSV data
///
inline parser::parser(std::istream &is, const dialect &d)
  : is_(&is), dialect_(d)
{
  internal::rewind(is);
}

///
/// \return a constant reference to the active CSV dialect
///
inline const pocket_csv::dialect &parser::active_dialect() const noexcept
{
  return dialect_;
}

///
/// \param[in] delim separator character for fields
/// \return          a reference to `this` object (fluent interface)
///
inline parser &parser::delimiter(char delim) & noexcept
{
  dialect_.delimiter = delim;
  return *this;
}
inline parser parser::delimiter(char delim) && noexcept
{
  dialect_.delimiter = delim;
  return *this;
}

///
/// \param[in] q quoting style (see `pocket_csv::dialect`)
/// \return      a reference to `this` object (fluent interface)
///
inline parser &parser::quoting(dialect::quoting_e q) & noexcept
{
  dialect_.quoting = q;
  return *this;
}

inline parser parser::quoting(dialect::quoting_e q) && noexcept
{
  dialect_.quoting = q;
  return *this;
}

///
/// Skips a possible header when iterating over the rows of the CSV file.
///
/// \return a reference to `this` object (fluent interface)
///
inline parser &parser::skip_header() & noexcept
{
  skip_header_ = true;
  return *this;
}
inline parser parser::skip_header() && noexcept
{
  skip_header_ = true;
  return *this;
}

///
/// \param[in] t if `true` trims leading and trailing spaces adjacent to
///              commas
/// \return      a reference to `this` object (fluent interface)
///
/// \remark
/// Trimming spaces is contentious and in fact the practice is specifically
/// prohibited by RFC 4180, which states: *spaces are considered part of a
/// field and should not be ignored*.
///
inline parser &parser::trim_ws(bool t) & noexcept
{
  dialect_.trim_ws = t;
  return *this;
}
inline parser parser::trim_ws(bool t) && noexcept
{
  dialect_.trim_ws = t;
  return *this;
}

///
/// \param[in] filter a filter function for CSV records
/// \return           a reference to `this` object (fluent interface)
///
/// \note
/// A filter function returns `true` for records to be keep.
///
/// \warning
/// Usually, in C++, a fluent interface returns a **reference**.
/// Here we return a **copy** of `this` object. The design decision is due to
/// the fact that a `parser' is a sort of Python generator and tends to be used
/// in for-loops.
/// Users often write:
///
///     for (auto record : parser(f).filter_hook(filter)) { ... }
///
/// but that's broken (it only works if `filter_hook` returns by value).
/// `parser` is a lighweight object and this shouldn't have an impact on
/// performance.
///
/// \see https://stackoverflow.com/q/10593686/3235496
///
inline parser &parser::filter_hook(filter_hook_t filter) & noexcept
{
  filter_hook_ = filter;
  return *this;
}
inline parser parser::filter_hook(filter_hook_t filter) && noexcept
{
  filter_hook_ = filter;
  return *this;
}

///
/// \return an iterator to the first record of the CSV file
///
inline parser::const_iterator parser::begin() const
{
  assert(is_);

  internal::rewind(*is_);

  if (*is_)
  {
    auto it(const_iterator(is_, filter_hook_, dialect_));

    if (dialect_.has_header == dialect::HAS_HEADER && skip_header_)
      ++it;

    return it;
  }

  return end();
}

///
/// \return an iterator used as sentry value to stop a cycle
///
inline parser::const_iterator parser::end() const noexcept
{
  return const_iterator();
}

///
/// Reads into the internal buffer the next record of the CSV file
///
inline void parser::const_iterator::get_input()
{
  if (!ptr_)
  {
    *this = const_iterator();
    return;
  }

  do
  {
    std::string line;

    do  // gets the first non-empty line
      if (!std::getline(*ptr_, line))
      {
        *this = const_iterator();
        return;
      }
    while (internal::trim(line).empty());

    value_ = parse_line(line);
  } while (filter_hook_ && !filter_hook_(value_));
}

///
/// This function parses a line of data by a delimiter.
///
/// \param[in] line line to be parsed
/// \return         a vector where each element is a field of the CSV line
///
/// If you pass in a comma as your delimiter it will parse out a
/// Comma Separated Value (*CSV*) file. If you pass in a `\t` char it will
/// parse out a tab delimited file (`.txt` or `.tsv`). CSV files often have
/// commas in the actual data, but accounts for this by surrounding the data in
/// quotes. This also means the quotes need to be parsed out, this function
/// accounts for that as well.
///
/// \note
/// If a quoted field is not terminated before the end of the line, the
/// remainder of the line is treated as part of the field. Multi-line
/// quoted fields are not supported.
///
inline parser::const_iterator::value_type parser::const_iterator::parse_line(
  const std::string &line)
{
  value_type record;  // the return value

  const char quote('"');

  bool inquotes(false);
  std::string curstring;

  const auto &add_field([&record, this](const std::string &field)
                        {
                          record.push_back(dialect_.trim_ws
                                           ? internal::trim(field) : field);
                        });

  const auto length(line.length());
  for (std::size_t pos(0); pos < length; ++pos)
  {
    const auto c(line[pos]);

    if (!inquotes && internal::trim(curstring).empty()
        && c == quote)  // begin quote char
    {
      if (dialect_.quoting == dialect::KEEP_QUOTES)
        curstring.push_back(c);

      inquotes = true;
    }
    else if (inquotes && c == quote)
    {
      if (pos + 1 < length && line[pos + 1] == quote)  // quote char
      {
        // Encountered 2 double quotes in a row (resolves to 1 double quote).
        curstring.push_back(c);
        ++pos;
      }
      else  // end quote char
      {
        if (dialect_.quoting == dialect::KEEP_QUOTES)
          curstring.push_back(c);

        inquotes = false;
      }
    }
    else if (!inquotes && c == dialect_.delimiter)  // end of field
    {
      add_field(curstring);
      curstring = "";
    }
    else if (!inquotes && (c == '\r' || c == '\n'))
      break;
    else
      curstring.push_back(c);
  }

  if (inquotes)
  {
#ifndef NDEBUG
    std::cerr << "Warning: unterminated quoted field; line accepted as-is\n";
#endif

    // Unterminated quoted field: accept the field as-is.
    // The closing quote is assumed to be missing.
    // This mirrors what Excel, LibreOffice, and many CSV readers do in
    // permissive mode.
  }

  add_field(curstring);

  return record;
}

///
/// Pretty-print the leading portion of a CSV file.
///
/// \param[in] is input stream containing CSV data
/// \param[in] n  number of data rows to extract
/// \return       a vector whose first element is the header (if present;
///               otherwise an empty row of the correct length), followed by up
///               to `n` data rows that match the header's column count.
///
/// If the CSV has a header, it appears in the first element of the returned
/// vector (`.front()`); otherwise the element is resized to the same width as
/// the first conforming data row. Subsequent elements are the first `n` rows
/// whose column count equals that of the first data row.
///
/// Resets the reading position of the input stream to the beginning before
/// returning.
///
[[nodiscard]] inline std::vector<parser::record_t> head(
  std::istream &is,
  const std::optional<dialect> &d = {},
  std::size_t n = 16)
{
  parser p(is, d ? *d : sniffer(is, n));
  const bool has_header(p.active_dialect().has_header == dialect::HAS_HEADER);

  std::vector<parser::record_t> ret;

  std::size_t expected_cols(0);

  auto it(p.begin());
  if (has_header)
  {
    ret.push_back(*it);
    expected_cols = ret.front().size();
    ++it;
  }
  else
    // Placeholder row, we'll resize once we know the column count.
    ret.emplace_back();

  while (it != p.end() && n)
  {
    if (!expected_cols)
      expected_cols = it->size();

    if (it->size() == expected_cols)
    {
      ret.push_back(*it);
      --n;
    }

    ++it;
  }

  // If there was no header, resize the placeholder to match data rows.
  if (!has_header)
  {
    if (ret.size() > 1)
      ret.front().resize(ret[1].size());
    else if (it != p.end() && it->size())
      ret.front().resize(it->size());
  }

  internal::rewind(is);

  return ret;
}

}  // namespace pocket_csv

#endif  // include guard
