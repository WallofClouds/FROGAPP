#include "engine/include/app.hpp"
#include "engine/include/entity.hpp"
#include "engine/include/camera.hpp"
#include "engine/include/input/input.hpp"
#include "engine/include/utils/utils.hpp"
#include <fs.ui/include/text.hpp>
#include <openal/include/listener.hpp>
#include <openal/include/source.hpp>
#include <time.h>

using namespace Firesteel;
using namespace FSOAL;

class FROGAPP :public App {

	//Глобальные переменные
	glm::vec3 Color{0,0,1};
	size_t points = 0;
	int state = 0;
	float sizeAccel = 0.00001f;
	bool sizeState = false, setRainbowBG = false;
	Entity frog{glm::vec3(0,0,-2), glm::vec3(90,180,45), glm::vec3(3,1,1.2f)};
	Shader base, text;
	Camera camera{glm::vec3(0),glm::vec3(0,0,-90)};
	Text counter;
	Source bg, sfx, sfx2;
	
	virtual void onInitialize() override {
		frog.load("res\\tiny_frog\\scene.gltf");
		base = Shader("res/shaders/base.vs", "res/shaders/base.fs"), text = Shader("res/shaders/text.vs", "res/shaders/text.fs");
		window.setIcon("frog.png");
		camera.update();
		TextRenderer::initialize();
		counter.loadFont("res/fonts/FatPixelFont.ttf", 16);
		FSOAL::initialize();
		bg.init("res/audio/chess.mp3", 0.1f, true)->play();
		sfx.init("res/audio/click1.mp3", 0.6F);
		sfx2.init("res/audio/click2.mp3", 0.6F);
		if(std::filesystem::exists("save"))
			sscanf(StrFromFile("save").c_str(), "%zu", &points);
		__time64_t long_time;
		_time64(&long_time);
		srand(long_time);
	}
	void HandleBG() {

		if (points < 100)
			Color = glm::vec3(0.8f, 0.8f, 0.15f);
		else if (points < 200)
			Color = glm::vec3(0.15f, 0.15f, 0.8f);
		else if (points < 400)
			Color = glm::vec3(0.8f, 0.15f, 0.15f);
		else if (points < 600)
			Color = glm::vec3(0.15f, 0.8f, 0.15f);
		else if (points < 800)
			Color = glm::vec3(0.45f, 0.15f, 0.2f);
		else if (points < 1000)
			Color = glm::vec3(0.5f, 0.2f, 0.8f);
		if (!setRainbowBG && points == 2500) {
			setRainbowBG = true;
			Color = glm::vec3(0, 0, 1);
		}
		if (points >= 2500 && points<5000) {
			if (Color.r >= 0.99f) state = 1;
			if (Color.g >= 0.99f) state = 2;
			if (Color.b >= 0.99f) state = 0;
			if (state == 0) Color += glm::vec3(0.0002f, 0, -0.0002f);
			if (state == 1) Color += glm::vec3(-0.0002f, 0.0002f, 0);
			if (state == 2) Color += glm::vec3(0, -0.0002f, 0.0002f);
		}

	}
	virtual void onUpdate() override {
		HandleBG();
		camera.aspect = window.aspect();
		glViewport(0, 0, static_cast<GLsizei>(window.getWidth()), static_cast<GLsizei>(window.getHeight()));
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
		if (Mouse::buttonDown(0)) {
			points += 1;
			if (rand() % 2 == 0)
				sfx.play();
			else sfx2.play();
		}
		if (Keyboard::keyDown(KeyCode::M)) {
			if (bg.isPlaying())
				bg.pause();
			else
				bg.play();
		}
		if (Keyboard::keyDown(KeyCode::D)) {
			points = 0;
			setRainbowBG = false;
		}
		window.setClearColor(Color);
		glm::mat4 proj = camera.getProjection(), view = camera.getView();
		base.enable();
		base.setMat4("projection", proj);
		base.setMat4("view", view);
		frog.draw(&base);
		counter.draw(&text, std::to_string(points), window.getSize(), glm::vec2(window.getWidth()/2, window.getHeight()-75), glm::vec2(1), glm::vec3(1));
	}
	virtual void onShutdown() override {
		frog.remove();
		base.remove();
		counter.remove();
		sfx.remove();
		sfx2.remove();
		bg.remove();
		FSOAL::deinitialize();
		StrToFile("save", std::to_string(points));
	}
	
};

int main() {
	return FROGAPP{}.start("You need to burn your pc, now.");
}
