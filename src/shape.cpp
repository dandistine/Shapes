#include "shape.hpp"

#include "utilities/utility.hpp"

Shape::iterator Shape::begin() {
	return tris.begin();
}

Shape::iterator Shape::end() {
    return tris.end();
}

Shape::const_iterator Shape::begin() const {
    return tris.cbegin();
}

Shape::const_iterator Shape::end() const {
    return tris.cend();
}

void Shape::SetPrototype(const Prototype& proto) {
    prototype = &proto;
}

void Shape::Draw(olc::PixelGameEngine* pge) const {
    for (const auto& t : tris) {
        pge->FillTriangleDecal(t.pos[0], t.pos[1], t.pos[2], color);
    }
}

// Scale, Rotate, and Translate a point from shape-space to world-space.  Rotation vector must be provided separately
olc::vf2d Shape::Translate(olc::vf2d pos, const olc::vf2d& sc) const {
    return utilities::rotate(pos, sc) * scale + position;
}

void Shape::MoveTo(olc::vf2d new_position) {
    position = new_position;
    const auto sc = olc::vf2d{std::sinf(theta), std::cosf(theta)};

    tris.clear();

    for(const auto& t : *prototype) {
        tris.push_back(
            {
                Translate(t.pos[0], sc),
                Translate(t.pos[1], sc),
                Translate(t.pos[2], sc),
            }
        );
    }
}

bool Shape::intersects(const Shape& other) const {
    for(const auto& t : other) {
        for(const auto& self_t : tris) {
            if(olc::utils::geom2d::overlaps(t, self_t)) {
                return true;
            }
        }
    }

    return false;
};

const std::vector<olc::vf2d>& Shape::WeaponPoints() {
    const auto sc = olc::vf2d{std::sinf(theta), std::cosf(theta)};
    weapon_points.clear();
    for(auto p : prototype->weapon_points) {
        weapon_points.push_back(Translate(p, sc));
    }
    return weapon_points;
}

size_t Shape::WeaponPointCount() const {
    return prototype->weapon_points.size();
}
