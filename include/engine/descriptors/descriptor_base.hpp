#ifndef DESCRIPTOR_BASE_HPP
#define DESCRIPTOR_BASE_HPP

#include "config.h"

#include "util/coordinate_calculation.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include "osrm/json_container.hpp"

#include <string>
#include <unordered_map>
#include <vector>

struct DescriptorTable : public std::unordered_map<std::string, unsigned>
{
    unsigned get_id(const std::string &key)
    {
        auto iter = find(key);
        if (iter != end())
        {
            return iter->second;
        }
        return 0;
    }
};

template <class DataFacadeT> class BaseDescriptor
{
  public:
    BaseDescriptor() {}
    // Maybe someone can explain the pure virtual destructor thing to me (dennis)
    virtual ~BaseDescriptor() {}
    virtual void Run(const InternalRouteResult &raw_route, osrm::json::Object &json_result) = 0;
    virtual void SetConfig(const DescriptorConfig &c) = 0;
};

#endif // DESCRIPTOR_BASE_HPP
