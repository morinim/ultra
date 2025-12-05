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

#include "utility/xml_tools.h"
#include "utility/assert.h"

#include <array>
#include <sstream>

namespace ultra
{

namespace crc32
{

const std::size_t checksum_length {8};

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

    for (std::uint32_t i(0); i < ret.size(); ++i)
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

std::pair<std::size_t, std::string> checksum_value_find(const std::string &xml)
{
  const std::string checksum_tag("checksum");

  const std::string open_checksum_tag("<" + checksum_tag + ">");
  const std::string close_checksum_tag("</" + checksum_tag + ">");

  const auto open_checksum_pos(xml.find(open_checksum_tag));
  if (open_checksum_pos == std::string::npos)
    return {std::string::npos, ""};  // no checksum found

  const auto close_checksum_pos(
    xml.find(close_checksum_tag,
             open_checksum_pos + open_checksum_tag.length()));

  if (close_checksum_pos == std::string::npos
      || close_checksum_pos < open_checksum_pos)
    return {std::string::npos, ""};  // no checksum found

  const auto pos(open_checksum_pos + open_checksum_tag.length());
  const auto length(close_checksum_pos - open_checksum_pos
                    - open_checksum_tag.length());
  const auto val(xml.substr(pos, length));

  return {pos, val};
}

std::string replace_checksum_value(
  const std::string &xml,
  const std::string &value = std::string(checksum_length, '0'))
{
  const auto [extracted_crc_pos, extracted_crc] = checksum_value_find(xml);

  Expects(extracted_crc_pos != std::string::npos);

  return xml.substr(0, extracted_crc_pos)
         + value
         + xml.substr(extracted_crc_pos + value.length());
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
  const auto temp_xml(replace_checksum_value(xml));

  std::stringstream ss;
  ss << std::hex << std::uppercase << std::setfill('0')
     << std::setw(checksum_length) << calculate(temp_xml);

  return replace_checksum_value(temp_xml, ss.str());
}

///
/// Verifies CRC32 in XML file.
///
/// \param[in] xml a string containing xml data and a checksum
/// \return        `true` is the checksum is correct
///
bool verify_xml_signature(const std::string &xml)
{
  const auto [extracted_crc_pos, extracted_crc] = checksum_value_find(xml);

  if (extracted_crc_pos == std::string::npos)
    return false;  // no checksum found

  const auto temp_xml(replace_checksum_value(xml));

  std::stringstream ss;
  ss << std::hex << std::uppercase << std::setfill('0')
     << std::setw(checksum_length) << calculate(temp_xml);

  return extracted_crc == ss.str();
}

}  // namespace crc32

}  // namespace ultra
