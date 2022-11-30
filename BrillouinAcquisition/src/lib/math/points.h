#ifndef POINTS_H
#define POINTS_H

struct POINT3 {
	double x{ 0 };
	double y{ 0 };
	double z{ 0 };
	POINT3& operator+=(const POINT3& pos) {
		x += pos.x;
		y += pos.y;
		z += pos.z;
		return *this;
	}
	POINT3 operator+(const POINT3& pos) {
		return POINT3{ x + pos.x, y + pos.y, z + pos.z };
	}
	POINT3& operator-=(const POINT3& pos) {
		x -= pos.x;
		y -= pos.y;
		z -= pos.z;
		return *this;
	}
	POINT3 operator-(const POINT3& pos) {
		return POINT3{ x - pos.x, y - pos.y, z - pos.z };
	}
	POINT3 operator*(double scaling) const {
		return POINT3{ scaling * x, scaling * y, scaling * z };
	}
	POINT3 operator/(double scaling) const {
		return POINT3{ x / scaling, y / scaling, z / scaling };
	}
	POINT3& operator/=(const double scaling) {
		x /= scaling;
		y /= scaling;
		z /= scaling;
		return *this;
	}
};

inline POINT3 operator*(double scaling, const POINT3& pos) {
	return pos * scaling;
}

inline double abs(POINT3 point) {
	return sqrt(pow(point.x, 2) + pow(point.y, 2) + pow(point.z, 2));
}

struct POINT2 {
	double x{ 0 };
	double y{ 0 };
	POINT2& operator+=(const POINT2& pos) {
		x += pos.x;
		y += pos.y;
		return *this;
	}
	POINT2 operator+(const POINT2& pos) {
		return POINT2{ x + pos.x, y + pos.y };
	}
	POINT2& operator-=(const POINT2& pos) {
		x -= pos.x;
		y -= pos.y;
		return *this;
	}
	POINT2 operator-(const POINT2& pos) {
		return POINT2{ x - pos.x, y - pos.y };
	}
	POINT2 operator*(double scaling) const {
		return POINT2{ scaling * x, scaling * y };
	}
	POINT2 operator/(double scaling) const {
		return POINT2{ x / scaling, y / scaling };
	}
	POINT2& operator/=(const double scaling) {
		x /= scaling;
		y /= scaling;
		return *this;
	}
};

inline POINT2 operator*(double scaling, const POINT2& pos) {
	return pos * scaling;
}

inline double abs(POINT2 point) {
	return sqrt(pow(point.x, 2) + pow(point.y, 2));
}

struct INDEX3 {
	int x{ 0 };
	int y{ 0 };
	int z{ 0 };
	INDEX3 operator+(const INDEX3& pos) {
		return INDEX3{ x + pos.x, y + pos.y, z + pos.z };
	}
	INDEX3 operator-(const INDEX3& pos) {
		return INDEX3{ x - pos.x, y - pos.y, z - pos.z };
	}
};

#endif // POINTS_H