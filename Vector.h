#ifndef VECTOR_H
#define VECTOR_H

class Vector {
public:
	Vector();
	Vector(float x, float y, float z);
	float length();
	void normalize();
	float x;
	float y;
	float z;
};

#endif