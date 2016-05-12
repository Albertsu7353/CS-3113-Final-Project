#include "Vector.h"
#include <math.h>

Vector::Vector() {}

Vector::Vector(float x, float y, float z = 0.0f) {
	this->x = x;
	this->y = y;
	this->z = z;
}

float Vector::length() {
	return sqrt(x * x + y * y + z * z);
}

void Vector::normalize() {
	float length = this->length();
	x /= length;
	y /= length;
	z /= length;
}