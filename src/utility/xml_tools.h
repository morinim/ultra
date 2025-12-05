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

#if !defined(ULTRA_XML_TOOLS_H)
#define      ULTRA_XML_TOOLS_H

#include "tinyxml2/tinyxml2.h"

#include <cstdint>
#include <iomanip>
#include <string>

namespace ultra
{

namespace crc32
{

[[nodiscard]] std::uint32_t calculate(const std::string &) noexcept;
[[nodiscard]] std::string embed_xml_signature(const std::string &);
[[nodiscard]] bool verify_xml_signature(const std::string &);

}  // namespace crc32

#include "utility/xml_tools.tcc"

}  // namespace ultra

#endif  // include guard
