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

///
/// RAII wrapper for `tinyxml2::XMLPrinter::OpenElement`.
///
/// Automatically calls `CloseElement()` on the printer when the object goes
/// out of scope (i.e. when its destructor is called).
///
class xml_closer
{
public:
  /// Constructor that calls `OpenElement` on the printer.
  ///
  /// \param[out] p    reference to the tinyxml2::XMLPrinter
  /// \param[in]  name the name of the element to open
  xml_closer(tinyxml2::XMLPrinter &p, const std::string& name) : printer_(p)
  {
    printer_.OpenElement(name.c_str());
  }

  /// Destructor that calls CloseElement on the printer.
  ///
  /// The resource is released here.
  ~xml_closer() { printer_.CloseElement(); }

  xml_closer(const xml_closer &) = delete;
  xml_closer &operator=(const xml_closer &) = delete;

private:
  tinyxml2::XMLPrinter &printer_;
};

#include "utility/xml_tools.tcc"

}  // namespace ultra

#endif  // include guard
