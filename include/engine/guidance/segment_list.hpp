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
class SegmentList
{
  public:
    SegmentList(const InternalRouteResult &raw_route, bool extract_alternative );

	const std::vector<std::uint32_t>& GetViaIndices() const;
	std::uint32_t GetDistance() const;
	std::uint32_t GetDuration() const;

	std::vector<SegmentInformation> const& Get() const;
  private:
	void ExtractRoute(const InternalRouteResult &raw_route);
	void ExtractAlternative(const InternalRouteResult &raw_route);

    void AppendSegment(const FixedPointCoordinate &coordinate, const PathData &path_point);
	void Finalize();

	//journey length in tenth of a second
	std::uint32_t total_distance;
	std::uint32_t total_duration;

	//segments that are required to keep
	std::vector<std::uint32_t> via_indices;

	//a list of node based segments
    std::vector<SegmentInformation> segments;
};
} // namespace description
} // namespace route
} // namespace engine
} // namespace osrm
