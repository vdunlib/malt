#include <tuple>
#include <vector>
#include <string>

#include "vdunlib/text/Joiner.hpp"

namespace vdunlib {

template<typename VType, typename ...ArgTypes>
std::tuple<std::string, std::string> formatParam(
        std::string prefix, VType&& v, ArgTypes&& ...args) {
    return std::make_tuple(std::move(prefix), text::Joiner{}.join(
            std::forward<VType>(v), std::forward<ArgTypes>(args)...));
}

using ParamDescrList = std::vector<std::tuple<std::string, std::string>>;

std::string formatParams(const ParamDescrList& pds);

} // namespace vdunlib
