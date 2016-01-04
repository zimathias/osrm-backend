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

#include "data_structures/coordinate.h"
#include "data_structures/internal_route_result.hpp"
#include "data_structures/segment_information.hpp"

#include <vector>

// transfers the internal edge based data structures to a more useable format
namespace osrm
{
namespace guidance
{
class SegmentList
{
  public:
    SegmentList(const InternalRouteResult &raw_route) final;

	const std::vector<std::uint32_t>& GetViaIndices() const;
	std::uint32_t GetDistance() const;
	std::uint32_t GetDuration() const;

	std::vector<SegmentInformation> const& Get();
  private:
    AppendSegment(const FixedPointCoordinate &coordinate, const PathData &path_point);
	Finalize();

	//journey length in tenth of a second
	std::uint32_t total_distance;
	std::uint32_t total_duration;

	//segments that are required to keep
	std::vector<std::uint32_t> via_indices;

	//a list of node based segments
    std::vector<SegmentInformation> segments;
};
} // namespace guidance
} // namespace osrm
