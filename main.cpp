#include "embedded.hpp"
#include "engine/include/app.hpp"
#include "engine/include/entity.hpp"
#include "engine/include/camera.hpp"
#include "engine/include/input/input.hpp"
#include "engine/include/utils/utils.hpp"
#include <fs.ui/include/text.hpp>
#include <openal/include/listener.hpp>
#include <openal/include/source.hpp>
#include <time.h>
#include <fs.ui/include/generic.hpp>

using namespace Firesteel;
using namespace FSOAL;

class FROGAPP :public App {

	//Глобальные переменные
	glm::vec3 Color{0,0,1};
	glm::vec3 Size{ 0 };
	int state = 0;
	float sizeAccel = 0.00001f, impact = 0;
	bool sizeState = false;
	bool fullscreen = false;
	static Entity frog1, frog2, frog3;
	static Entity* frog;
	static Source bg;
	static bool setRainbowBG;
	static unsigned int points, prestiege, selectedFrog;
	Shader base, text;
	Camera camera{glm::vec3(0),glm::vec3(0,0,-90)};
	Text counter;
	Source sfx, sfx2;
	Button button, reset, mute, unmute;
	
	virtual void onInitialize() override {
		window.setIconFromMemory(ucIconData, ucIconDataSize);
		__time64_t long_time;
		_time64(&long_time);
		srand(long_time);		

		frog1.load("res\\frogs\\tiny_frog\\scene.gltf");
		frog2.load("res\\frogs\\cartoon_frog\\cartoon_frog.obj");
		frog3.load("res\\frogs\\giga_frog\\giga_frog.obj");
		frog = &frog1;
		base = Shader("res/shaders/base.vs", "res/shaders/base.fs"), text = Shader("res/shaders/text.vs", "res/shaders/text.fs");
		camera.update();

		TextRenderer::initialize();
		counter.loadFont("res/fonts/FatPixelFont.ttf", 16);
		
		button.initialize("Престиж", glm::vec2(window.getWidth()/2-120, 200), glm::vec2(280, 75));
		reset.initialize("res\\icons\\reset.png", glm::vec2(10, 60), glm::vec2(50));
		reset.background = reset.hover = glm::vec4(glm::vec3(0), 1);
		mute.initialize("res\\icons\\sound_off.png", glm::vec2(10, 110), glm::vec2(50));
		mute.background = mute.hover = glm::vec4(glm::vec3(0), 1);
		unmute.initialize("res\\icons\\sound_on.png", glm::vec2(10, 110), glm::vec2(50));
		unmute.background = unmute.hover = glm::vec4(glm::vec3(0), 1);
		button.background = glm::vec4(glm::vec3(0), 0.45f);
		button.hover = glm::vec4(glm::vec3(1), 0.25f);
		mute.setClickCallback([]() {
			bg.play();
			});
		unmute.setClickCallback([]() {
			bg.pause();
			});
		reset.setClickCallback([]() {
			points = 0;
			prestiege = 0;
			selectedFrog = 0;
			frog = &frog1;
			setRainbowBG = false;
			});
		button.setClickCallback([]() {
			points = 0;
			setRainbowBG = false;
			prestiege += 1;
			if (prestiege == 1) {
				frog = &frog2;
				selectedFrog = 1;
			}
			if (prestiege == 2) {
				frog = &frog3;
				selectedFrog = 2;
			}
			});

		FSOAL::initialize();
		bg.init("res/audio/chess.mp3", 0.1f, true)->play();
		sfx.init("res/audio/click1.mp3", 0.6F);
		sfx2.init("res/audio/click2.mp3", 0.6F);

		if (std::filesystem::exists("save")) {
			std::vector<std::string> saveFile = StrSplit(StrFromFile("save"),'\n');
			sscanf(saveFile[0].c_str(), "%zu", &points);
			if(saveFile.size()>1) sscanf(saveFile[1].c_str(), "%zu", &prestiege);
			if (prestiege == 1) {
				frog = &frog2;
				selectedFrog = 1;
			}
			if (prestiege == 2) {
				frog = &frog3;
				selectedFrog = 2;
			}
		}
		if (prestiege == 1)
			frog = &frog2;
		else if (prestiege == 2)
			frog = &frog3;
	}

	void HandleBG() {

		if(!setRainbowBG && points >= 2500) {
			setRainbowBG = true;
			Color = glm::vec3(0, 0, 1);
		}

		if(points < 100)		Color = glm::vec3(0.75f);				//White
		else if(points < 200)	Color = glm::vec3(0.8f, 0.8f, 0.15f);	//Yellow
		else if(points < 400)	Color = glm::vec3(0.15f, 0.15f, 0.8f);	//Blue
		else if(points < 600)	Color = glm::vec3(0.8f, 0.15f, 0.15f);	//Red
		else if(points < 800)	Color = glm::vec3(0.15f, 0.8f, 0.15f);	//Green
		else if (points < 1000)	Color = glm::vec3(0.45f, 0.15f, 0.2f);	//Purple
		else if(points < 2500)	Color = glm::vec3(0.85f, 0.3f, 0.3f);	//Pink
		else {
			if(Color.r >= 0.99f) state = 1;
			if(Color.g >= 0.99f) state = 2;
			if(Color.b >= 0.99f) state = 0;
			if(state == 0) Color += glm::vec3(0.0002f, 0, -0.0002f);
			if(state == 1) Color += glm::vec3(-0.0002f, 0.0002f, 0);
			if(state == 2) Color += glm::vec3(0, -0.0002f, 0.0002f);
		}

	}
	virtual void onUpdate() override {
		HandleBG();
		window.setClearColor(Color);
		bool buttonVisible = prestiege == 0 && points >= 3000 ||
			prestiege == 1 && points >= 10000;
		if(buttonVisible) {
			button.setPositon(glm::vec2(window.getWidth() / 2 - 120, 200));
			button.update(window.getSize());
		}
		reset.update(window.getSize());
		if(!bg.isPlaying()) mute.update(window.getSize());
		else unmute.update(window.getSize());;;;;;;;;;

		camera.aspect = window.aspect();
		glViewport(0, 0, static_cast<GLsizei>(window.getWidth()), static_cast<GLsizei>(window.getHeight()));
		frog->transform.rotation += glm::vec3(0, 0.02f, 0.02f);
		if (frog->transform.size.y > 7 && !sizeState) {
			sizeState = true;
			sizeAccel = 0.00001f;
		} else if(frog->transform.size.y < 1 && sizeState) {
			sizeState = false;
			sizeAccel = 0.00001f;
		}

		if(!sizeState)
			Size += glm::vec3(sizeAccel * deltaTime);
		else Size -= glm::vec3(sizeAccel * deltaTime);
		sizeAccel += 0.00001f;
		if(impact > 0) impact -= 0.001f;
		frog->transform.size = glm::vec3(3+std::clamp(points/10000, 0U, 5U), 1, 1.2f) + Size + glm::vec3(impact);

		if(Mouse::buttonDown(0)||Keyboard::keyDown(KeyCode::SPACEBAR)|| Mouse::buttonDown(1)) {
			switch (selectedFrog) {
			case 0:
				points += 1;
				break;
			case 1:
				points += 2;
				break;
			case 2:
				points += 5;
				break;
			}
			if(impact<=3.5f) impact += 0.25f;
			if(rand() % 2 == 0) sfx.play();
			else sfx2.play();
		}
		if(Keyboard::keyDown(KeyCode::M)) {
			if(bg.isPlaying()) bg.pause();
			else bg.play();
		}
		if (Keyboard::keyDown(KeyCode::ESCAPE)) {
			window.close();
		}
		if(Keyboard::keyDown(KeyCode::F1))
			points += points*0.5f;
		if (Keyboard::keyDown(KeyCode::F11)) {
			if(!fullscreen) {
				const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
				glfwSetWindowMonitor(window.ptr(), glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, 0);
				glfwSetWindowAttrib(window.ptr(), GLFW_DECORATED, GLFW_FALSE);
			} else {
				glfwSetWindowMonitor(window.ptr(), nullptr, 0, 0, 800, 600, 0);
				glfwSetWindowAttrib(window.ptr(), GLFW_DECORATED, GLFW_TRUE);
			}
			fullscreen = !fullscreen;
		}

		glm::mat4 proj = camera.getProjection(), view = camera.getView();
		base.enable();
		base.setMat4("projection", proj);
		base.setMat4("view", view);
		frog->draw(&base);

		counter.draw(&text, std::to_string(points), window.getSize(),
			glm::vec2((window.getWidth()/2)-16*(std::to_string(points).length()), window.getHeight()-75), glm::vec2(1), glm::vec4(1));

		if (buttonVisible) {
			button.draw(&text, window.getSize());
			glDisable(GL_DEPTH_TEST);
			counter.draw(&text, "Prestiege", window.getSize(),
				glm::vec2(window.getWidth()/2-100, 185), glm::vec2(1), glm::vec4(1));
			glEnable(GL_DEPTH_TEST);
		}

		text.enable();
		text.setBool("dontApplyColor", true);
		reset.draw(&text, window.getSize());
		if (!bg.isPlaying()) mute.draw(&text, window.getSize());
		else unmute.draw(&text, window.getSize());
		text.enable();
		text.setBool("dontApplyColor", false);
		
	}
	virtual void onShutdown() override {
		base.remove();
		button.remove();
		reset.remove();
		mute.remove();
		unmute.remove();
		counter.remove();
		sfx.remove();
		sfx2.remove();
		bg.remove();
		FSOAL::deinitialize();
		StrToFile("save", std::to_string(points) + "\n" + std::to_string(prestiege));
		frog1.remove();
		frog2.remove();
	}
	
};

Entity FROGAPP::frog1 = Entity{ glm::vec3(0,0,-2), glm::vec3(90,180,45), glm::vec3(3,1,1.2f) };
Entity FROGAPP::frog2 = Entity{ glm::vec3(0,0,-2), glm::vec3(0,225,0), glm::vec3(3,1,1.2f) };
Entity FROGAPP::frog3 = Entity{ glm::vec3(0,0,-2), glm::vec3(0,225,0), glm::vec3(3,1,1.2f) };
unsigned int FROGAPP::points=0, FROGAPP::prestiege=0, FROGAPP::selectedFrog=0;
bool FROGAPP::setRainbowBG=0;
Entity* FROGAPP::frog = nullptr;
Source FROGAPP::bg{};

int main() {
	return FROGAPP{}.start("You need to burn your pc, now.");
}
