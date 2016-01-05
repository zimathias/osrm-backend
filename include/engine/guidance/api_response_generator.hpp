#pragma once

#include "segment_list.hpp"
#include "annotators/textual_route_annotation.hpp"

#include "engine/douglas_peucker.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/object_encoder.hpp"
#include "engine/phantom_node.hpp"
#include "engine/polyline_formatter.hpp"
#include "engine/route_name_extraction.hpp"
#include "engine/segment_information.hpp"
#include "extractor/turn_instructions.hpp"
#include "osrm/coordinate.hpp"
#include "osrm/json_container.hpp"
#include "util/integer_range.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <cstdint>
#include <cstddef>
#include <cmath>

#include <limits>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{
namespace detail
{
struct Segment
{
    uint32_t name_id;
    int32_t length;
    std::size_t position;
};
} // namespace detail

template <typename DataFacadeT> class ApiResponseGenerator
{
  public:
    typedef DataFacadeT DataFacade;
    typedef SegmentList<DataFacade> SegmentListT;
    typedef detail::Segment Segment;
    typedef ExtractRouteNames<DataFacade, Segment> RouteNameExtractor;

    ApiResponseGenerator(DataFacade *facade);

    // This runs a full annotation, according to config.
    // The output is tailored to the viaroute plugin.
    void DescribeRoute(const DescriptorConfig &config,
                       const InternalRouteResult &raw_route,
                       osrm::json::Object &json_result);

    // The following functions allow access to the different parts of the Describe Route
    // functionality.
    // For own responses, they can be used to generate only subsets of the information.
    // In the normal situation, Describe Route is the desired usecase.

    // generate an overview of a raw route
    osrm::json::Object SummarizeRoute(const InternalRouteResult &raw_route,
                                      const SegmentListT &segment_list) const;

    // create an array containing all via-points/-indices used in the query
    osrm::json::Array ListViaPoints(const InternalRouteResult &raw_route) const;
    osrm::json::Array ListViaIndices(const SegmentListT &segment_list) const;

    osrm::json::Value GetGeometry(const bool return_encoded, const SegmentListT &segments) const;

    // TODO this dedicated creation seems unnecessary? Only used for route names
    std::vector<Segment> BuildRouteSegments(const SegmentListT &segment_list) const;

    // adds checksum and locations
    osrm::json::Object BuildHintData(const InternalRouteResult &raw_route) const;

  private:
    // data access to translate ids back into names
    DataFacade *facade;
};

template <typename DataFacadeT>
ApiResponseGenerator<DataFacadeT>::ApiResponseGenerator(DataFacadeT *facade_)
    : facade(facade_)
{
}

template <typename DataFacadeT>
void ApiResponseGenerator<DataFacadeT>::DescribeRoute(const DescriptorConfig &config,
                                                      const InternalRouteResult &raw_route,
                                                      osrm::json::Object &json_result)
{
    const bool ALLOW_SIMPLIFICATION = true;
    const bool EXTRACT_ROUTE = false;
    const bool EXTRACT_ALTERNATIVE = true;
    SegmentListT segment_list(raw_route, EXTRACT_ROUTE, config.zoom_level, ALLOW_SIMPLIFICATION,
                              facade);
    json_result.values["route_summary"] = SummarizeRoute(raw_route, segment_list);
    json_result.values["via_points"] = ListViaPoints(raw_route);
    json_result.values["via_indices"] = ListViaIndices(segment_list);

    if (config.geometry)
    {
        json_result.values["route_geometry"] = GetGeometry(config.encode_geometry, segment_list);
    }

    if (config.instructions)
    {
        json_result.values["route_instructions"] =
            annotators::AnnotateRoute(segment_list.Get(), facade);
    }

    RouteNames route_names;
    RouteNameExtractor generate_route_names;

    if (raw_route.has_alternative())
    {
        SegmentListT alternate_segment_list(raw_route, EXTRACT_ALTERNATIVE, config.zoom_level,
                                            ALLOW_SIMPLIFICATION, facade);

        // Alternative Route Summaries are stored in an array to (down the line) allow multiple
        // alternatives
        osrm::json::Array json_alternate_route_summary_array;
        json_alternate_route_summary_array.values.push_back(
            SummarizeRoute(raw_route, alternate_segment_list));
        json_result.values["alternative_summaries"] = json_alternate_route_summary_array;
        json_result.values["alternative_indices"] = ListViaIndices(alternate_segment_list);

        if (config.geometry)
        {
            osrm::json::Value alternate_geometry_string =
                GetGeometry(config.encode_geometry, alternate_segment_list);
            osrm::json::Array json_alternate_geometries_array;
            json_alternate_geometries_array.values.push_back(alternate_geometry_string);
            json_result.values["alternative_geometries"] = json_alternate_geometries_array;
        }

        if (config.instructions)
        {
            osrm::json::Array json_alternate_annotations_array;
            json_alternate_annotations_array.values.push_back(
                annotators::AnnotateRoute(alternate_segment_list.Get(), facade));
            json_result.values["alternative_instructions"] = json_alternate_annotations_array;
        }

        // generate names for both the main path and the alternative route
        auto path_segments = BuildRouteSegments(segment_list);
        auto alternate_segments = BuildRouteSegments(alternate_segment_list);
        route_names = generate_route_names(path_segments, alternate_segments, facade);

        osrm::json::Array json_alternate_names_array;
        osrm::json::Array json_alternate_names;
        json_alternate_names.values.push_back(route_names.alternative_path_name_1);
        json_alternate_names.values.push_back(route_names.alternative_path_name_2);
        json_alternate_names_array.values.push_back(json_alternate_names);
        json_result.values["alternative_names"] = json_alternate_names_array;
        json_result.values["found_alternative"] = osrm::json::True();
    }
    else
    {
        json_result.values["found_alternative"] = osrm::json::False();
        // generate names for the main route on its own
        auto path_segments = BuildRouteSegments(segment_list);
        std::vector<detail::Segment> alternate_segments;
        route_names = generate_route_names(path_segments, alternate_segments, facade);
    }

    osrm::json::Array json_route_names;
    json_route_names.values.push_back(route_names.shortest_path_name_1);
    json_route_names.values.push_back(route_names.shortest_path_name_2);
    json_result.values["route_name"] = json_route_names;

    json_result.values["hint_data"] = BuildHintData(raw_route);
}

template <typename DataFacadeT>
osrm::json::Object
ApiResponseGenerator<DataFacadeT>::SummarizeRoute(const InternalRouteResult &raw_route,
                                                  const SegmentListT &segment_list) const
{
    osrm::json::Object json_route_summary;
    if (not raw_route.segment_end_coordinates.empty())
    {
        auto start_name_id = raw_route.segment_end_coordinates.front().source_phantom.name_id;
        json_route_summary.values["start_point"] = facade->get_name_for_id(start_name_id);
        auto destination_name_id = raw_route.segment_end_coordinates.back().target_phantom.name_id;
        json_route_summary.values["end_point"] = facade->get_name_for_id(destination_name_id);
    }
    json_route_summary.values["total_time"] = segment_list.GetDuration();
    json_route_summary.values["total_distance"] = segment_list.GetDistance();
    return json_route_summary;
}

template <typename DataFacadeT>
osrm::json::Array
ApiResponseGenerator<DataFacadeT>::ListViaPoints(const InternalRouteResult &raw_route) const
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

template <typename DataFacadeT>
osrm::json::Array
ApiResponseGenerator<DataFacadeT>::ListViaIndices(const SegmentListT &segment_list) const
{
    osrm::json::Array via_indices;
    via_indices.values.insert(via_indices.values.end(), segment_list.GetViaIndices().begin(),
                              segment_list.GetViaIndices().end());
    return via_indices;
}

template <typename DataFacadeT>
osrm::json::Value ApiResponseGenerator<DataFacadeT>::GetGeometry(const bool return_encoded, const SegmentListT &segments) const
{
    if (return_encoded)
        return PolylineFormatter().printEncodedString(segments.Get());
    else
        return PolylineFormatter().printUnencodedString(segments.Get());
}

template <typename DataFacadeT>
std::vector<detail::Segment>
ApiResponseGenerator<DataFacadeT>::BuildRouteSegments(const SegmentListT &segment_list) const
{
    std::vector<detail::Segment> result;
    for (const auto &segment : segment_list.Get())
    {
        auto current_turn = segment.turn_instruction;
        if (TurnInstructionsClass::TurnIsNecessary(current_turn) and
            (TurnInstruction::EnterRoundAbout != current_turn))
        {

            detail::Segment seg = {segment.name_id, static_cast<int32_t>(segment.length),
                                   static_cast<std::size_t>(result.size())};
            result.emplace_back(std::move(seg));
        }
    }
    return result;
}

template <typename DataFacadeT>
osrm::json::Object
ApiResponseGenerator<DataFacadeT>::BuildHintData(const InternalRouteResult &raw_route) const
{
    osrm::json::Object json_hint_object;
    json_hint_object.values["checksum"] = facade->GetCheckSum();
    osrm::json::Array json_location_hint_array;
    std::string hint;
    for (const auto i : osrm::irange<std::size_t>(0, raw_route.segment_end_coordinates.size()))
    {
        ObjectEncoder::EncodeToBase64(raw_route.segment_end_coordinates[i].source_phantom, hint);
        json_location_hint_array.values.push_back(hint);
    }
    ObjectEncoder::EncodeToBase64(raw_route.segment_end_coordinates.back().target_phantom, hint);
    json_location_hint_array.values.push_back(hint);
    json_hint_object.values["locations"] = json_location_hint_array;

    return json_hint_object;
}

} // namespace guidance
} // namespace engine
} // namespace osrm
