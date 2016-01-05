#pragma once

#include "osrm/coordinate.hpp"

#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "engine/segment_information.hpp"

#include "extractor/turn_instructions.hpp"

#include <vector>

// transfers the internal edge based data structures to a more useable format
namespace osrm
{
namespace engine
{
namespace route
{
namespace description
{
template <typename DataFacadeT> class SegmentList
{
  public:
    typedef DataFacadeT DataFacade;
    SegmentList(const InternalRouteResult &raw_route, bool extract_alternative, DataFacade *facade);

    const std::vector<std::uint32_t> &GetViaIndices() const;
    std::uint32_t GetDistance() const;
    std::uint32_t GetDuration() const;

    std::vector<SegmentInformation> const &Get() const;

  private:
    void ExtractRoute(const InternalRouteResult &raw_route, DataFacade *facade);
    void ExtractAlternative(const InternalRouteResult &raw_route, DataFacade *facade);

    void AppendSegment(const FixedPointCoordinate& coordinate, const PathData &path_point);
    void Finalize(const InternalRouteResult &raw_route);

    // journey length in tenth of a second
    std::uint32_t total_distance;
    std::uint32_t total_duration;

    // segments that are required to keep
    std::vector<std::uint32_t> via_indices;

    // a list of node based segments
    std::vector<SegmentInformation> segments;
};

template <typename DataFacadeT>
SegmentList<DataFacadeT>::SegmentList(const InternalRouteResult &raw_route,
                                      bool extract_alternative,
                                      DataFacade *facade)
    : total_distance(0), total_duration(0)
{
    if (extract_alternative)
    {
        ExtractAlternative(raw_route, facade);
    }
    else
    {
        ExtractRoute(raw_route, facade);
    }

	Finalize(raw_route);
}

template <typename DataFacadeT>
void SegmentList<DataFacadeT>::ExtractRoute(const InternalRouteResult &raw_route,
                                            DataFacade *facade)
{
    // only operate on valid routes
    if (not raw_route.is_valid())
        return;

    {
        const auto source = raw_route.segment_end_coordinates.front().source_phantom;
        const bool traversed_in_reverse = raw_route.source_traversed_in_reverse.front();
        const auto segment_duration =
            (traversed_in_reverse ? source.reverse_weight : source.forward_weight);
        const auto travel_mode =
            (traversed_in_reverse ? source.backward_travel_mode : source.forward_travel_mode);

        AppendSegment(source.location, PathData(0, source.name_id, TurnInstruction::HeadOn,
                                                segment_duration, travel_mode));
    }

    for (std::size_t raw_index = 0; raw_index < raw_route.unpacked_path_segments.size();
         ++raw_index)
    {
        // Get all the coordinates for the computed route
        FixedPointCoordinate current_coordinate;
        for (const PathData &path_data : raw_route.unpacked_path_segments[raw_index])
        {
            AppendSegment(facade->GetCoordinateOfNode(path_data.node), path_data);
        }

        const auto target = raw_route.segment_end_coordinates[raw_index].target_phantom;
        const bool traversed_in_reverse = raw_route.target_traversed_in_reverse[raw_index];
        const EdgeWeight segment_duration =
            (traversed_in_reverse ? target.reverse_weight : target.forward_weight);
        const TravelMode travel_mode =
            (traversed_in_reverse ? target.backward_travel_mode : target.forward_travel_mode);
        segments.emplace_back(target.location, target.name_id, segment_duration, 0.f,
                              raw_route.is_via_leg(raw_index) ? TurnInstruction::ReachViaLocation
                                                              : TurnInstruction::NoTurn,
                              true, true, travel_mode);
    }
}

template <typename DataFacadeT>
void SegmentList<DataFacadeT>::ExtractAlternative(const InternalRouteResult &raw_route,
                                                  DataFacade *facade)
{
    { // build initial segment
        const auto source = raw_route.segment_end_coordinates.front().source_phantom;
        const bool traversed_in_reverse = raw_route.alt_source_traversed_in_reverse.front();
        const auto segment_duration =
            (traversed_in_reverse ? source.reverse_weight : source.forward_weight);
        const auto travel_mode =
            (traversed_in_reverse ? source.backward_travel_mode : source.forward_travel_mode);

        AppendSegment(source.location, PathData(0, source.name_id, TurnInstruction::HeadOn,
                                                segment_duration, travel_mode));
    }
    // Get all the coordinates for the computed route
    for (const PathData &path_data : raw_route.unpacked_alternative)
    {
        AppendSegment(facade->GetCoordinateOfNode(path_data.node), path_data);
    }
    {
        const auto target = raw_route.segment_end_coordinates.back().target_phantom;
        const bool traversed_in_reverse = raw_route.target_traversed_in_reverse.back();
        const EdgeWeight segment_duration =
            (traversed_in_reverse ? target.reverse_weight : target.forward_weight);
        const TravelMode travel_mode =
            (traversed_in_reverse ? target.backward_travel_mode : target.forward_travel_mode);
        segments.emplace_back(target.location, target.name_id, segment_duration, 0.f,
                              TurnInstruction::NoTurn, true, true, travel_mode);
    }
}

template <typename DataFacadeT> std::uint32_t SegmentList<DataFacadeT>::GetDistance() const
{
    return total_distance;
}
template <typename DataFacadeT> std::uint32_t SegmentList<DataFacadeT>::GetDuration() const
{
    return total_duration;
}

template <typename DataFacadeT>
std::vector<std::uint32_t> const &SegmentList<DataFacadeT>::GetViaIndices() const
{
    return via_indices;
}

template <typename DataFacadeT>
std::vector<SegmentInformation> const &SegmentList<DataFacadeT>::Get() const
{
    return segments;
}

template <typename DataFacadeT>
void SegmentList<DataFacadeT>::AppendSegment(const FixedPointCoordinate &coordinate,
                                             const PathData &path_point)
{
    // if the start location is on top of a node, the first movement might be zero-length,
    // in which case we dont' add a new description, but instead update the existing one
    if ((1 == segments.size()) && (segments.front().location == coordinate))
    {
        if (path_point.segment_duration > 0)
        {
            segments.front().name_id = path_point.name_id;
            segments.front().travel_mode = path_point.travel_mode;
        }
        return;
    }

    // make sure mode changes are announced, even when there otherwise is no turn
    const TurnInstruction turn = [&]() -> TurnInstruction
    {
        if (TurnInstruction::NoTurn == path_point.turn_instruction &&
            segments.front().travel_mode != path_point.travel_mode &&
            path_point.segment_duration > 0)
        {
            return TurnInstruction::GoStraight;
        }
        return path_point.turn_instruction;
    }();

    segments.emplace_back(coordinate, path_point.name_id, path_point.segment_duration, 0.f, turn,
                          path_point.travel_mode);
}

template <typename DataFacadeT> void SegmentList<DataFacadeT>::Finalize(const InternalRouteResult& raw_route)
{
    segments[0].length = 0.f;
    for (const auto i : osrm::irange<std::size_t>(1, segments.size()))
    {
        // move down names by one, q&d hack
        segments[i - 1].name_id = segments[i].name_id;
        segments[i].length = coordinate_calculation::great_circle_distance(segments[i - 1].location,
                                                                           segments[i].location);
    }

    float segment_length = 0.;
    EdgeWeight segment_duration = 0;
    std::size_t segment_start_index = 0;

    double path_length = 0;

    for (const auto i : osrm::irange<std::size_t>(1, segments.size()))
    {
        path_length += segments[i].length;
        segment_length += segments[i].length;
        segment_duration += segments[i].duration;
        segments[segment_start_index].length = segment_length;
        segments[segment_start_index].duration = segment_duration;

        if (TurnInstruction::NoTurn != segments[i].turn_instruction)
        {
            BOOST_ASSERT(segments[i].necessary);
            segment_length = 0;
            segment_duration = 0;
            segment_start_index = i;
        }
    }

    total_distance = static_cast<std::uint32_t>(round(path_length));
	total_duration = static_cast<std::uint32_t>(round(raw_route.shortest_path_length / 10.));

	auto start_phantom = raw_route.segment_end_coordinates.front().source_phantom;
	auto target_phantom = raw_route.segment_end_coordinates.back().target_phantom;

    // Post-processing to remove empty or nearly empty path segments
    if (segments.size() > 2 && std::numeric_limits<float>::epsilon() > segments.back().length &&
        !(segments.end() - 2)->is_via_location)
    {
        segments.pop_back();
        segments.back().necessary = true;
        segments.back().turn_instruction = TurnInstruction::NoTurn;
        target_phantom.name_id = (segments.end() - 2)->name_id;
    }

    if (segments.size() > 2 && std::numeric_limits<float>::epsilon() > segments.front().length &&
        !(segments.begin() + 1)->is_via_location)
    {
        segments.erase(segments.begin());
        segments.front().turn_instruction = TurnInstruction::HeadOn;
        segments.front().necessary = true;
        start_phantom.name_id = segments.front().name_id;
    }
    unsigned necessary_segments = 0; // a running index that counts the necessary pieces
    const auto markNecessarySegments =
        [this, &necessary_segments](SegmentInformation &first, const SegmentInformation &second)
    {
        if (!first.necessary)
            return;

        // mark the end of a leg (of several segments)
        if (first.is_via_location)
            via_indices.push_back(necessary_segments);

        const double post_turn_bearing =
            coordinate_calculation::bearing(first.location, second.location);
        const double pre_turn_bearing =
            coordinate_calculation::bearing(second.location, first.location);
        first.post_turn_bearing = static_cast<short>(post_turn_bearing * 10);
        first.pre_turn_bearing = static_cast<short>(pre_turn_bearing * 10);

        ++necessary_segments;
    };

    // calculate which segments are necessary and update segments for bearings
    osrm::for_each_pair(segments, markNecessarySegments);
    via_indices.push_back(necessary_segments);

    BOOST_ASSERT(via_indices.size() >= 2);
}

} // namespace description
} // namespace route
} // namespace engine
} // namespace osrm
