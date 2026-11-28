#if !defined(ULTRA_ECHARTS_BUNDLE_H)
#define      ULTRA_ECHARTS_BUNDLE_H

#include <string_view>

namespace ultra::wopr
{

/// Returns the vendored minified Apache ECharts library source.
[[nodiscard]] std::string_view echarts_min_js() noexcept;

}  // namespace ultra::wopr

#endif  // include guard
