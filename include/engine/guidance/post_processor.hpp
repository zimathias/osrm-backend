#pragma once

#include "engine/segment_information.hpp"

#include <string>

namespace osrm
{
namespace route
{
namespace description
{
namespace detail
{
template <typename DataFacadeT>
bool canCombine(const SegmentInformation &first,
                const SegmentInformation &second,
                const DataFacadeT &facade);
                //second_name = facade->get_name_for_id(second.name_id);

} // namespace detail

std::vector<SegmentInformation> Run(std::vector<SegmentInformation> segments) { return segments; }

} // namespace description
} // namespace route
} // namespace osrm
