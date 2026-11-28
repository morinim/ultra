file(READ "${INPUT}" hex_data HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," c_bytes "${hex_data}")
string(REGEX REPLACE "(.{80})" "\\1\n" c_bytes_wrapped "${c_bytes}")
file(WRITE "${OUTPUT}"
  "#include \"echarts_bundle.h\"\n"
  "namespace ultra::wopr {\n"
  "std::string_view echarts_min_js() noexcept {\n"
  "  static const unsigned char data[] = {\n"
  "${c_bytes_wrapped}\n"
  "  };\n"
  "  return {reinterpret_cast<const char *>(data), sizeof(data)};\n"
  "}\n"
  "}\n")
