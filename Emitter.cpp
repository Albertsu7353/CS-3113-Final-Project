#include "Emitter.h"
#include <random>
#include <time.h>

float randomInt(int i, int j) {
	return (rand() % i - j) / 250.0f;
	srand(time(NULL));
}

ParticleEmitter::ParticleEmitter(unsigned int particleCount) {
	maxLifetime = 100.0f;
	size = 7.5f;
	for (int i = 0; i < particleCount; i++) {
		Particle p;
		p.position.x = position.x;
		p.position.y = position.y;
		p.velocity.x = randomInt(400, 200);
		p.velocity.y = randomInt(400, 0);
		p.r = 1.0f;
		p.g = 1.0f;
		p.b = 1.0f;
		particles.push_back(p);
	}
}
void ParticleEmitter::Reset() {
	for (Particle &p : particles) {
		p.lifetime = 0.0f;
		p.position.x = position.x;
		p.position.y = position.y;
		p.velocity.x = randomInt(400, 200);
		p.velocity.y = randomInt(400, 0);
		p.r = 1.0f;
		p.g = 0.0f;
		p.b = 0.0f;
	}
}
void ParticleEmitter::Update(float elapsed) {
	for (Particle &p : particles) {
		p.velocity.y -= 1.0 * elapsed;
		p.position.x += p.velocity.x * elapsed;
		p.position.y += p.velocity.y * elapsed;
		p.lifetime += 0.1f;
	}
}

void ParticleEmitter::Render() {
	std::vector<float> particleVertices;
	std::vector<float> colorVertices;
	for (int i = 0; i < particles.size(); i++) {
		if (particles[i].lifetime < maxLifetime) {
			particles[i].r -= 0.001f;
			particles[i].g -= 0.001f;
			particles[i].b -= 0.001f;
			particleVertices.push_back(particles[i].position.x);
			particleVertices.push_back(particles[i].position.y);
			colorVertices.push_back(particles[i].r);
			colorVertices.push_back(particles[i].g);
			colorVertices.push_back(particles[i].b);
		}
	}
	glVertexPointer(2, GL_FLOAT, 0, particleVertices.data());
	glEnableClientState(GL_VERTEX_ARRAY);

	glColorPointer(3, GL_FLOAT, 0, colorVertices.data());
	glEnableClientState(GL_COLOR_ARRAY);

	glEnable(GL_PROGRAM_POINT_SIZE);
	glPointSize(size);

	glDrawArrays(GL_POINTS, 0, particleVertices.size() / 2);

	glDisableClientState(GL_COLOR_ARRAY);
}