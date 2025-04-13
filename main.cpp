#include "engine/include/app.hpp"
#include "engine/include/entity.hpp"
#include "engine/include/camera.hpp"

using namespace Firesteel;

class FROGAPP :public App {

	//Глобальные переменные
	glm::vec3 Color{0,0,1};
	int state = 0;
	float sizeAccel = 0.00001f;
	bool sizeState = false;
	Entity frog{glm::vec3(0,0,-2), glm::vec3(90,180,45), glm::vec3(3,1,1.2f)};
	Shader shader;
	Camera camera{glm::vec3(0),glm::vec3(0,0,-90)};
	
	virtual void onInitialize() override {
		frog.load("res\\tiny_frog\\scene.gltf");
		shader=Shader("res/shader.vs","res/shader.fs");
		window.setIcon("frog.png");
		camera.update();
	}
	virtual void onUpdate() override {
		camera.aspect = window.aspect();
		glViewport(0, 0, static_cast<GLsizei>(window.getWidth()), static_cast<GLsizei>(window.getHeight()));
		if (Color.r >= 0.99f) state = 1;
		if (Color.g >= 0.99f) state = 2;
		if (Color.b >= 0.99f) state = 0;
		if (state == 0) Color += glm::vec3(0.0002f, 0, -0.0002f);
		if (state == 1) Color += glm::vec3(-0.0002f, 0.0002f, 0);
		if (state == 2) Color += glm::vec3(0, -0.0002f, 0.0002f);
		frog.transform.rotation += glm::vec3(0, 0.02f, 0.02f);
		if (frog.transform.size.y > 7 && !sizeState) {
			sizeState = true;
			sizeAccel = 0.00001f;
		}
		else if(frog.transform.size.y < 1 && sizeState) {
			sizeState = false;
			sizeAccel = 0.00001f;
		}
		if (!sizeState) {
			frog.transform.size += glm::vec3(sizeAccel * deltaTime);
			sizeAccel += 0.00001f;
		}
		else {
			frog.transform.size -= glm::vec3(sizeAccel * deltaTime);
			sizeAccel += 0.00001f;
		}
		window.setClearColor(Color);
		window.clearBuffers();
		glm::mat4 proj = camera.getProjection(), view = camera.getView();
		shader.enable();
		shader.setMat4("projection", proj);
		shader.setMat4("view", view);
		frog.draw(&shader);
	}
	virtual void onShutdown() override {
		frog.remove();
		shader.remove();
	}

};

int main() {

	return FROGAPP{}.start("You need to burn your pc, now.");


}
