#include "SheetSprite.h"
#include <vector>

class Entity {
public:
	Entity(SheetSprite sprite, float x, float y, float width, float height, float velocity_x, float  velocity_y, float acceleration_x, float acceleration_y, float friction, int type) :sprite(sprite), x(x), y(y), width(width), height(height), velocity_x(velocity_x), velocity_y(velocity_y), acceleration_x(acceleration_x), acceleration_y(acceleration_y), friction_x(friction), type(type){}
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

	float gravity = -7.0f;
	float friction_x;
	float friction_y = 1.3;
	//0 for player 1, 1 for ball, 2 for ball holder,3 for ball in player 1 hand, 4 for ball in player 2 hand, 5 for player 2
	int type;

};