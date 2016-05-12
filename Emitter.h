#include <vector>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include "Vector.h"
using namespace std;


class Particle {
public:
	Vector position;
	Vector velocity;
	float lifetime;
	float r;
	float g;
	float b;
};

class ParticleEmitter {
public:
	ParticleEmitter(unsigned int particleCount);
	ParticleEmitter(){}
	~ParticleEmitter(){}
	void Update(float elapsed);
	void Render();
	Vector position;
	Vector gravity;
	float maxLifetime;
	void Reset();
	std::vector<Particle> particles;
	float size;
};