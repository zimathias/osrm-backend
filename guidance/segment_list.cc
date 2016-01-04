#include "segment_list.h"

#include <cstddef>

#include <boost/assert.hpp>

namespace osrm
{
namespace guidance
{

SegmentList::SegmentList(const InternalRouteResult &raw_route) : total_weight(0)
{
    // only operate on valid routes
    if (not raw_route.is_valid())
        return;

    const auto source = raw_route.segment_end_coordinates.front().source_phantom;
    const bool traversed_in_reverse = raw_route.source_traversed_in_reverse.front();
    const auto segment_duration =
        (traversed_in_reverse ? source.reverse_weight : source.forward_weight);
    const auto travel_mode =
        (traversed_in_reverse ? source.backward_travel_mode : source.forward_travel_mode);

    AppendSegment(source.location, PathData(0, source.name_id, TurnInstruction::HeadOn,
                                            segment_duration, travel_mode));

    for (std::size_t raw_index = 0; raw_index < raw_route.unpacked_path_segments.size())
    {
        unsigned added_element_count = 0;
        // Get all the coordinates for the computed route

        FixedPointCoordinate current_coordinate;
        for (const PathData &path_data : raw_route.unpacked_path_segments[raw_index])
        {
            current_coordinate = facade->GetCoordinateOfNode(path_data.node);
            description_factory.AppendSegment(current_coordinate, path_data);
            ++added_element_count;
        }
        description_factory.SetEndSegment(
            raw_route.segment_end_coordinates[raw_index].target_phantom,
            raw_route.target_traversed_in_reverse[raw_index], raw_route.is_via_leg(raw_index));
        ++added_element_count;

        BOOST_ASSERT((raw_route.unpacked_path_segments[raw_size].size() + 1) ==
                     added_element_count);
    }
}

std::uint32_t SegmentList::GetDistance() const { return total_distance; }
std::uint32_t SegmentList::GetDuration() const { return total_duration; }

std::vector<std::uint32_t> const &SegmentList::GetViaIndices() const { return via_indices; }

std::vector<SegmentInformation> const &SegmentList::Get() { return segments; }

void SegmentList::AppendSegment(const FixedPointCoordinate &coordinate, const PathData &path_point)
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

void SegmentList::Finalize()
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
	total_duration = static_cast<std::uint32_t>(round( raw_route.shortest_path_length/10.) );

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
        [&via_indices,&necessary_segments](auto &first, const auto &second)
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
    }

	//calculate which segments are necessary and update segments for bearings
    osrm::for_each_pair(segments, markNecessarySegments);
    via_indices.push_back(necessary_segments);

    BOOST_ASSERT(via_indices.size() >= 2);
    return;
}

} // namespace guidance
} // namespace osrm
