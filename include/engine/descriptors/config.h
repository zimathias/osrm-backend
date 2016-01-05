#pragma once

struct DescriptorConfig
{
    DescriptorConfig() : instructions(true), geometry(true), encode_geometry(true), zoom_level(18)
    {
    }

    template <class OtherT>
    DescriptorConfig(const OtherT &other)
        : instructions(other.print_instructions), geometry(other.geometry),
          encode_geometry(other.compression), zoom_level(other.zoom_level)
    {
        BOOST_ASSERT(zoom_level >= 0);
    }

    bool instructions;
    bool geometry;
    bool encode_geometry;
    short zoom_level;
};
