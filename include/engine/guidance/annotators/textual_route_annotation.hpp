#pragma once

#include "engine/segment_information.hpp"
#include "engine/guidance/segment_list.hpp"

#include "extractor/turn_instructions.hpp"

#include "osrm/json_container.hpp"

#include "util/bearing.hpp"
#include "util/cast.hpp"

#include <cstdint>

#include <limits>
#include <string>
#include <vector>

namespace osrm
{
namespace engine
{
namespace route
{
namespace description
{
template< typename DataFacadeT >
inline osrm::json::Array
AnnotateRoute(const std::vector<SegmentInformation> &route_segments, DataFacadeT* facade)
{
    osrm::json::Array json_instruction_array;
    // Segment information has following format:
    //["instruction id","streetname",length,position,time,"length","earth_direction",azimuth]
    uint32_t necessary_segments_running_index = 0;

    struct RoundAbout
    {
        int32_t start_index;
        uint32_t name_id;
        int32_t leave_at_exit;
    } round_about;

    round_about = {std::numeric_limits<int32_t>::max(), 0, 0};
    std::string temp_dist, temp_length, temp_duration, temp_bearing, temp_instruction;

	//Generate annotations for every segment
    for (const SegmentInformation &segment : route_segments)
    {
        osrm::json::Array json_instruction_row;
        TurnInstruction current_instruction = segment.turn_instruction;
        if (TurnInstructionsClass::TurnIsNecessary(current_instruction))
        {
            if (TurnInstruction::EnterRoundAbout == current_instruction)
            {
                round_about.name_id = segment.name_id;
                round_about.start_index = necessary_segments_running_index;
            }
            else
            {
                std::string current_turn_instruction;
                if (TurnInstruction::LeaveRoundAbout == current_instruction)
                {
                    temp_instruction =
                        std::to_string(cast::enum_to_underlying(TurnInstruction::EnterRoundAbout));
                    current_turn_instruction += temp_instruction;
                    current_turn_instruction += "-";
                    temp_instruction = std::to_string(round_about.leave_at_exit + 1);
                    current_turn_instruction += temp_instruction;
                    round_about.leave_at_exit = 0;
                }
                else
                {
                    temp_instruction =
                        std::to_string(cast::enum_to_underlying(current_instruction));
                    current_turn_instruction += temp_instruction;
                }
                json_instruction_row.values.push_back(current_turn_instruction);

                json_instruction_row.values.push_back(facade->get_name_for_id(segment.name_id));
                json_instruction_row.values.push_back(std::round(segment.length));
                json_instruction_row.values.push_back(necessary_segments_running_index);
                json_instruction_row.values.push_back(std::round(segment.duration / 10.));
                json_instruction_row.values.push_back(
                    std::to_string(static_cast<uint32_t>(segment.length)) + "m");

                // post turn bearing
                const double post_turn_bearing_value = (segment.post_turn_bearing / 10.);
                json_instruction_row.values.push_back(bearing::get(post_turn_bearing_value));
                json_instruction_row.values.push_back(
                    static_cast<uint32_t>(round(post_turn_bearing_value)));

                json_instruction_row.values.push_back(segment.travel_mode);

                // pre turn bearing
                const double pre_turn_bearing_value = (segment.pre_turn_bearing / 10.);
                json_instruction_row.values.push_back(bearing::get(pre_turn_bearing_value));
                json_instruction_row.values.push_back(
                    static_cast<uint32_t>(round(pre_turn_bearing_value)));

                json_instruction_array.values.push_back(json_instruction_row);
            }
        }
        else if (TurnInstruction::StayOnRoundAbout == current_instruction)
        {
            ++round_about.leave_at_exit;
        }
        if (segment.necessary)
        {
            ++necessary_segments_running_index;
        }
    }

    osrm::json::Array json_last_instruction_row;
    temp_instruction =
        std::to_string(cast::enum_to_underlying(TurnInstruction::ReachedYourDestination));
    json_last_instruction_row.values.push_back(temp_instruction);
    json_last_instruction_row.values.push_back("");
    json_last_instruction_row.values.push_back(0);
    json_last_instruction_row.values.push_back(necessary_segments_running_index - 1);
    json_last_instruction_row.values.push_back(0);
    json_last_instruction_row.values.push_back("0m");
    json_last_instruction_row.values.push_back(bearing::get(0.0));
    json_last_instruction_row.values.push_back(0.);
    json_last_instruction_row.values.push_back(bearing::get(0.0));
    json_last_instruction_row.values.push_back(0.);
    json_instruction_array.values.push_back(json_last_instruction_row);

    return json_instruction_array;
}

} // namespace description
} // namespace route
} // namespace engine
} // namespace osrm
