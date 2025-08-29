#include "utility.hpp"

namespace utilities
{
    uint16_t posToSector(olc::vf2d position)
    {
        uint16_t x = static_cast<uint16_t>(position.x);
        uint16_t y = static_cast<uint16_t>(position.y);

        return (x & 0xFF00) | ((y & 0xFF00) >> 8);
    }

    olc::Pixel RandomColor() {
        return olc::Pixel(rand() % 256, rand() % 256, rand() % 256);
    }

    olc::Pixel RandomDarkColor() {
        return olc::Pixel(rand() % 128, rand() % 128, rand() % 128);
    }

    olc::Pixel RandomBrightColor() {
        return olc::Pixel(128 + (rand() % 128), 128 + (rand() % 128), 128 + (rand() % 128));
    }

    olc::Pixel RandomRedColor() {
        return olc::Pixel(128 + (rand() % 128), rand() % 128, rand() % 128);
    }

    olc::Pixel RandomGreenColor() {
        return olc::Pixel(rand() % 128, 128 + (rand() % 128), rand() % 128);
    }
    
    olc::Pixel RandomBlueColor() {
        return olc::Pixel(rand() % 128, rand() % 128, 128 + (rand() % 128));
    }
}
