#pragma once

#include "utilities/olcUTIL_Geometry2D.h"

#include "olcPixelGameEngine.h"

#include <vector>

enum class ShapePrototypes {
	Triangle,
	Square,
	Cursor,
	Pentagon,
	Star5_2,
	Star6_2,
	Star7_3,
	Star8_2,
	Star9_3
};

struct Prototype {
	using iterator = std::vector<olc::utils::geom2d::triangle<float>>::iterator;
	using const_iterator = std::vector<olc::utils::geom2d::triangle<float>>::const_iterator;
	
	iterator begin() {
		return tris.begin();
	}

	iterator end() {
		return tris.end();
	}

	const_iterator begin() const {
		return tris.cbegin();
	}

	const_iterator end() const {
		return tris.cend();
	}

	std::vector<olc::utils::geom2d::triangle<float>> tris;
	std::vector<olc::vf2d> weapon_points;
    ShapePrototypes type;
};

struct Shape {
	using iterator = std::vector<olc::utils::geom2d::triangle<float>>::iterator;
	using const_iterator = std::vector<olc::utils::geom2d::triangle<float>>::const_iterator;

	Shape(const Prototype& other) : tris(other.tris), prototype(&other) { }
    Shape(const Shape& other) : tris(other.tris), prototype(other.prototype) {}

	iterator begin();

	iterator end();

	const_iterator begin() const;

	const_iterator end() const;

	void SetPrototype(const Prototype& proto);

	void Draw(olc::PixelGameEngine* pge) const;

	// Scale, Rotate, and Translate a point from shape-space to world-space.  Rotation vector must be provided separately
	olc::vf2d Translate(olc::vf2d pos, const olc::vf2d& sc) const;

	void MoveTo(olc::vf2d new_position);

	bool intersects(const Shape& other) const;

	const std::vector<olc::vf2d>& WeaponPoints();

	size_t WeaponPointCount() const ;

	float scale {1.0f};
	float theta {0.0f};
	olc::vf2d position {0.0f, 0.0f};
	olc::Pixel color {olc::MAGENTA};

	virtual ~Shape() = default;
protected:
	std::vector<olc::utils::geom2d::triangle<float>> tris;
	std::vector<olc::vf2d> weapon_points;
	const Prototype* prototype;
};
