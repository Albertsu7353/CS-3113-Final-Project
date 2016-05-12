#include "Entity.h"

float Entity::lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}
void Entity::Update(float elapsed){
	collidedBottom = false;


	velocity_y += gravity * elapsed;
	velocity_x = lerp(velocity_x, 0.0f, elapsed * friction_x);
	velocity_x += acceleration_x * elapsed;

	y += velocity_y * elapsed;
	x += velocity_x * elapsed;
	acceleration_x = 0;
}

void Entity::Render(ShaderProgram *program){
	Matrix modelMatrix;
	modelMatrix.identity();
	modelMatrix.Translate(x, y, 0);
	program->setModelMatrix(modelMatrix);
	sprite.Draw();
}
