#include "utility.hpp"

namespace utilities
{
    uint16_t posToSector(olc::vf2d position)
    {
        uint16_t x = static_cast<uint16_t>(position.x);
        uint16_t y = static_cast<uint16_t>(position.y);

        return (x & 0xFF00) | ((y & 0xFF00) >> 8);
    }

}
