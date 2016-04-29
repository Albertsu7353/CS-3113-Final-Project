#include "SheetSprite.h"
#include <vector>

class Entity {
public:
	Entity(SheetSprite sprite, float x, float y, float width, float height, float velocity_x, float  velocity_y, float acceleration_x, float acceleration_y) :sprite(sprite), x(x), y(y), width(width), height(height), velocity_x(velocity_x), velocity_y(velocity_y), acceleration_x(acceleration_x), acceleration_y(acceleration_y){}
	Entity();
	void Update(float elapsed);
	void Render(ShaderProgram* program);
	float lerp(float v0, float v1, float t);

	SheetSprite sprite;
	float x;
	float y;
	float width;
	float height;
	float velocity_x;
	float velocity_y;
	float acceleration_x;
	float acceleration_y;

	bool collidedTop = false;
	bool collidedBottom = false;
	bool collidedLeft = false;
	bool collidedRight = false;

	float gravity = -2.5f;
	float friction_x = 1.0;
};