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

#include <array>
#include <atomic>
#include <regex>
#include <thread>

#include "utility/misc.h"
#include "kernel/nullary.h"

namespace ultra
{
///
/// \param[in] lhs first term of comparison
/// \param[in] rhs second term of comparison
/// \return        `true` if all elements in both strings are same (case
///                insensitively)
///
bool iequals(const std::string &lhs, const std::string &rhs)
{
  return std::ranges::equal(lhs, rhs,
                            [](auto c1, auto c2)
                            { return std::tolower(c1) == std::tolower(c2); });
}

///
/// \param[in] s the string to be tested
/// \return      `true` if `s` contains a number
///
bool is_number(std::string s)
{
  s = trim(s);

  char *end;
  const double val(std::strtod(s.c_str(), &end));  // if no conversion can be
                                                   // performed, `end` is set
                                                   // to `s.c_str()`
  return end != s.c_str() && *end == '\0' && std::isfinite(val);
}

///
/// \param[in] s the input string
/// \return      a copy of `s` with spaces removed on both sides of the string
///
/// \see http://stackoverflow.com/a/24425221/3235496
///
std::string trim(const std::string &s)
{
  auto front = std::ranges::find_if_not(
    s, [](auto c) { return std::isspace(c); });

  // The search is limited in the reverse direction to the last non-space value
  // found in the search in the forward direction.
  auto back = std::find_if_not(s.rbegin(), std::make_reverse_iterator(front),
                               [](auto c) { return std::isspace(c); }).base();

  return {front, back};
}

///
/// Replaces the first occurrence of a string with another string.
///
/// \param[in] s    input string
/// \param[in] from substring to be searched for
/// \param[in] to   substitute string
/// \return         the modified input
///
std::string replace(std::string s,
                    const std::string &from, const std::string &to)
{
  const auto start_pos(s.find(from));
  if (start_pos != std::string::npos)
    s.replace(start_pos, from.length(), to);

  return s;
}

///
/// Replaces all occurrences of a string with another string.
///
/// \param[in] s    input string
/// \param[in] from substring to be searched for
/// \param[in] to   substitute string
/// \return         the modified input
///
std::string replace_all(std::string s,
                        const std::string &from, const std::string &to)
{
  if (!from.empty())
  {
    std::size_t start(0);
    while ((start = s.find(from, start)) != std::string::npos)
    {
      s.replace(start, from.length(), to);
      start += to.length();  // in case `to` contains `from`, like replacing
                             // "x" with "yx"
    }
  }

  return s;

  // With std::regex it'd be something like:
  //     s = std::regex_replace(s, std::regex(from), to);
  // (possibly escaping special characters in the `from` string)
}

///
/// Converts a `value_t` to `double`.
///
/// \param[in] v value that should be converted to `double`
/// \return      the result of the conversion of `v`. If the conversion cannot
///              be performed returns `0.0`
///
/// This function is useful for:
/// * debugging purpose;
/// * symbolic regression and classification task (the value returned by
///   the interpreter will be used in a "numeric way").
///
/// \note
/// This is not the same of `std::get<T>(v)`.
///
template<>
double lexical_cast<double>(const ultra::value_t &v)
{
  using namespace ultra;

  switch (v.index())
  {
  case d_double:  return std::get<D_DOUBLE>(v);
  case d_int:     return std::get<D_INT>(v);
  case d_string:  return lexical_cast<double>(std::get<D_STRING>(v));
  default:        return 0.0;
  }
}

template<>
int lexical_cast<int>(const ultra::value_t &v)
{
  using namespace ultra;

  switch (v.index())
  {
  case d_double:  return static_cast<int>(std::get<D_DOUBLE>(v));
  case d_int:     return std::get<D_INT>(v);
  case d_string:  return lexical_cast<int>(std::get<D_STRING>(v));
  default:        return 0;
  }
}

///
/// Converts a `value_t` to `std::string`.
///
/// \param[in] v value that should be converted to `std::string`
/// \return      the result of the conversion of `v`. If the conversion cannot
///              be performed returns an empty string
///
/// This function is useful for debugging purpose.
///
template<>
std::string lexical_cast<std::string>(const ultra::value_t &v)
{
  using namespace ultra;

  switch (v.index())
  {
  case d_double:  return std::to_string(std::get<D_DOUBLE>(v));
  case d_int:     return std::to_string(   std::get<D_INT>(v));
  case d_string:  return std::get<D_STRING>(v);
  case d_nullary: return get_if_nullary(v)->name();
  default:        return {};
  }
}

template<>
std::string lexical_cast<std::string>(std::chrono::milliseconds d)
{
  using namespace std;
  using namespace std::chrono_literals;

  const auto ds(chrono::duration_cast<chrono::days>(d));
  const auto hrs(chrono::duration_cast<chrono::hours>(d - ds));
  const auto mins(chrono::duration_cast<chrono::minutes>(d - ds - hrs));
  const auto secs(chrono::duration_cast<chrono::seconds>(d - ds - hrs - mins));

  std::stringstream ss;

  if (ds.count())
    ss << ds.count() << ':';
  if (ds.count() || hrs.count())
    ss << std::setw(2) << std::setfill('0') << hrs.count() << ':';
  if (ds.count() || hrs.count() || mins.count())
    ss << std::setw(2) << std::setfill('0') << mins.count() << ':';
  if (ds.count() || hrs.count() || mins.count())
    ss << std::setw(2);

  ss << std::setfill('0') << secs.count();

  if (ds.count() == 0 && hrs.count() == 0 && mins.count() == 0)
  {
    const auto ms(chrono::duration_cast<chrono::milliseconds>(d - ds - hrs
                                                              - mins - secs));
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
  }

  return ss.str();
}

app_level_uid::operator unsigned() const noexcept
{
  return val_;
}

unsigned app_level_uid::next_id() noexcept
{
  // The initialisation is thread-safe because it's static and C++11 guarantees
  // that static initialisation will be thread-safe. Subsequent access will be
  // thread-safe because it's an atomic.
  static std::atomic<unsigned> count(0);

  return count++;
}

namespace crc32
{

///
/// Calculates the CRC-32 checksum of a string as defined by ISO 3309.
///
/// \param[in] data input string
/// \return         CRC checksum of `data`
///
/// A cyclic redundancy check (CRC) is an error-detecting code commonly used in
/// digital networks and storage devices to detect accidental changes to
/// digital data.
///

std::uint32_t calculate(const std::string &data) noexcept
{
  static constexpr auto table =
  []
  {
    std::array<std::uint32_t, 256> ret {};

    for (std::uint32_t i(0); i < 256; ++i)
    {
      std::uint32_t crc(i);
      for (unsigned j(0); j < 8; ++j)
        if (crc & 1)
          crc = std::uint32_t(0xEDB88320) ^ (crc >> 1);
        else
          crc >>= 1;

      ret[i] = crc;
    }

    return ret;
  }();

  std::uint32_t crc(0xFFFFFFFF);

  for (unsigned char byte : data)
    crc = (crc >> 8) ^ table[(crc ^ byte) & 0xFF];

  return crc ^ 0xFFFFFFFF;
}

///
/// Embeds CRC32 in a XML string.
///
/// \param[in] xml a string containing xml data
/// \return        `xml` with embedded crc (`checksum` tag)
///
/// The CRC should be computed excluding the part where it is embedded.
/// Out approach is to calculate the CRC with a placeholder (`00000000`), then
/// replace it with the actual value.
///
std::string embed_xml_signature(const std::string &xml)
{
  std::regex crc_regex(R"(<checksum>\s*[0-9A-Fa-f]*\s*</checksum>)");
  std::string temp_xml(std::regex_replace(xml, crc_regex,
                                          "<checksum>00000000</checksum>"));

  std::stringstream ss;
  ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8)
     << calculate(temp_xml);

  return std::regex_replace(xml, crc_regex,
                            "<checksum>" + ss.str() + "</checksum>");
}

///
/// Verifies CRC32 in XML file.
///
/// \param[in] xml a string containing xml data and a checksum
/// \return        `true` is the checksum is correct
///
bool verify_xml_signature(const std::string &xml)
{
  std::smatch match;
  std::regex crc_regex(R"(<checksum>([0-9A-Fa-f]+)</checksum>)");

  if (!std::regex_search(xml, match, crc_regex) || match.size() < 2)
    return false;  // no checksum found

  const std::string extracted_crc(match[1].str());
  const std::string temp_xml(std::regex_replace(
                               xml, crc_regex,
                               "<checksum>00000000</checksum>"));

  std::stringstream ss;
  ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8)
     << calculate(temp_xml);

  return extracted_crc == ss.str();
}

}  // namespace crc32

}  // namespace ultra
