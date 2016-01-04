/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include "algorithms/douglas_peucker.hpp"

#include "data_structures/internal_route_result.hpp"

#include <osrm/json_container.hpp>

#include "segment_list.h"

#include <cstdint>

namespace osrm
{
namespace annotate
{
template <typename DataFacadeT> class Descriptor
{
  public:
    typedef DataFacadeT DataFacade;
    Descriptor(shared_ptr<DataFacade> facade);

    // Requires Data Facade
    osrm::json::Object describeRoute(const DescriptorConfig &config,
                                     const InternalRouteResult &raw_route);

  private:
    // generate an overview of a raw route
    osrm::json::Object summarizeRoute(const InternalRouteResult &raw_route,
                                      const SegmentList &segment_list) const;

    // create an array containing all via points used in the query
    osrm::json::Array listViaPoints() const;

    // TODO find out why this is needed?
    osrm::json::Array listViaIndices() const;

    // data access to translate ids back into names
    shared_ptr<DataFacade> facade;
};

template <typedef DataFacadeT>
Descriptor<DataFacadeT>::Descriptor(shared_ptr<DataFacadeT> facade_)
    : facade(facade_)
{
}

template <typedef DataFacadeT>
osrm::json::Object Descriptor<DataFacadeT>::describeRoute(const DescriptorConfig &config,
                                                          const InternalRouteResult &raw_route)
{
    osrm::json::Object json_result;

	SegmentList segment_list( raw_route );
    json_result.values["route_summary"] = summarizeRoute(raw_route, segment_list);
    json_result.values["via_points"] = listViaPoints(raw_route);
    json_result.values["via_indices"] = buildViaIndices(segment_list);

    if (config.geometry)
    {
        const auto getGeometry = [this](const bool return_encoded)
        {
            return return_encoded ? PolylineFormatter().printEncodedString(segment_list.Get())
                                  : PolylineFormatter().printUnencodedString(segment_list.Get());
        };

        json_result.values["route_geometry"] = getGeometry(config.encode_geometry);
    }

    if (config.instructions)
    {
    }
    return json_result;
}

template <typedef DataFacadeT>
osrm::json::Object Descriptor<DataFacadeT>::summarizeRoute(const InternalRouteResult &raw_route,
                                                           const SegmentList &segment_list) const
{
    osrm::json::Object json_route_summary;
    json_route_summary.values["total_distance"] = segment_list.GetDistance();
    json_route_summary.values["total_time"] = segment_list.GetDuration();
    if (not raw_route.segment_end_coordinates.empty())
    {
        auto start_name_id =
            internal_route_result.segment_end_coordinates.front().source_phantom.name_id;
        json_route_summary.values["start_point"] = facade->get_name_for_id(start_name_id);
        auto destination_name_id =
            internal_route_result.segment_end_coordinates.back().target_phantom.name_id;
        json_route_summary.values["end_point"] = facade->get_name_for_id(destination_name_id);
    }
    return json_route_summary;
}

template <typedef DataFacadeT>
osrm::json::Array Descriptor<DataFacadeT>::listViaPoints(const InternalRouteResult &raw_route) const
{
    osrm::json::Array json_via_points_array;
    osrm::json::Array json_first_coordinate;
    json_first_coordinate.values.push_back(
        raw_route.segment_end_coordinates.front().source_phantom.location.lat /
        COORDINATE_PRECISION);
    json_first_coordinate.values.push_back(
        raw_route.segment_end_coordinates.front().source_phantom.location.lon /
        COORDINATE_PRECISION);
    json_via_points_array.values.push_back(json_first_coordinate);
    for (const PhantomNodes &nodes : raw_route.segment_end_coordinates)
    {
        std::string tmp;
        osrm::json::Array json_coordinate;
        json_coordinate.values.push_back(nodes.target_phantom.location.lat / COORDINATE_PRECISION);
        json_coordinate.values.push_back(nodes.target_phantom.location.lon / COORDINATE_PRECISION);
        json_via_points_array.values.push_back(json_coordinate);
    }
    return json_via_points_array;
}

template <typedef DataFacadeT>
osrm::json::Array Descriptor<DataFacadeT>::listViaIndices(const SegmentList &segment_list) const
{
    osrm::json::Array via_indices;
	via_indices.values = segment_list.GetViaIndices();
	return via_indices
}

} // namespace guidance
} // namespace osrm
