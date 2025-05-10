#include "embedded.hpp"
#include "engine/include/app.hpp"
#include "engine/include/entity.hpp"
#include "engine/include/camera.hpp"
#include "engine/include/input/input.hpp"
#include "engine/include/utils/utils.hpp"
#include <fs.ui/include/text.hpp>
#include <openal/include/listener.hpp>
#include <openal/include/source.hpp>
#include <fs.ui/include/generic.hpp>
#include <time.h>
#include <future>
#include <chrono>
#include <thread>

using namespace Firesteel;
using namespace FSOAL;

// For God's sake don't do unicode (cyrillic) comments.
// They don't properly work with git.
// That's why ALL my code is documented in ENGLISH.

class FROGAPP : public App {

	// ���������� ����������
	glm::vec3 Color{0,0,1}, Size{ 0 };
	int state = 0;
	float sizeAccel = 0.00001f, impact = 0;
	bool sizeState = false, fullscreen = false;
	static Entity frog1, frog2, frog3;
	static Entity* frog;
	static Source bg, upgradeSfxs;
	static bool setRainbowBG, showPrestButtons, randomizedCards, canClick, canIdleGain, triggeredAnEnding;
	static int upgradeHovered;
	const size_t upgradeCount = 3;
	const char* upgradeDescription[3] = {
		"Hold any mouse button/spacebar to gain points",
		"Get points from all the frogs",
		"Frogs idly generate points"
	};
	struct Upgrades {
		bool holdToClick=0;
		bool multiFrog=0;
		bool idleGain=0;
	};
	static Upgrades upg;
	static unsigned int points, uPrestiege, upgrade1, upgrade2;
	Shader base, text;
	Camera camera{glm::vec3(0),glm::vec3(0,0,-90)};
	Text counter;
	Source sfx, sfx2;
	Button prestiegeBtn, reset, mute, unmute, upgradeBtn1, upgradeBtn2;

	std::future<void> cooldown(int milliseconds) {
		return std::async(std::launch::async, [milliseconds]() {
			if(!upg.holdToClick) return;
			canClick=false;
    	    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
			canClick=true;
    	});
	}
	std::future<void> idleGain(int milliseconds) {
    	return std::async(std::launch::async, [milliseconds]() {
			if(!upg.idleGain || !canIdleGain) return;
			canIdleGain=false;
    	    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
			switch (uPrestiege)
			{
			case 0:
				points+=1;
				break;
			case 1:
				points+=3;
				break;
			case 2:
				points+=8;
				break;
			}
			canIdleGain=true;
    	});
	}
	
	// �������
	static void prestiege() {
		LOG_INFO("Triggered prestiege");
		points = 0;
		setRainbowBG = false;
		if(!showPrestButtons) uPrestiege += 1;
		if (uPrestiege == 0)
			frog = &frog1;
		if (uPrestiege == 1)
			frog = &frog2;
		if (uPrestiege == 2)
			frog = &frog3;
		showPrestButtons = false;
		randomizedCards = false;
	}

	// �� ����� �������
	virtual void onInitialize() override {
		window.setIconFromMemory(ucIconData, ucIconDataSize);

		//�������� �����
		frog1.load("res\\frogs\\tiny_frog\\scene.gltf");
		frog2.load("res\\frogs\\cartoon_frog\\cartoon_frog.obj");
		frog3.load("res\\frogs\\giga_frog\\giga_frog.obj");
		frog = &frog1;
		base = Shader("res/shaders/base.vs", "res/shaders/base.fs"), text = Shader("res/shaders/text.vs", "res/shaders/text.fs");
		camera.update();

		//���������
		TextRenderer::initialize();
		counter.loadFont("res/fonts/FatPixelFont.ttf", 16);
		upgradeBtn1.initialize("", glm::vec2(225, window.getHeight() - 100), glm::vec2(window.getWidth()/3, window.getHeight()-25));
		upgradeBtn2.initialize("", glm::vec2(window.getWidth() - 100, window.getHeight() - 100), glm::vec2(window.getWidth() / 3, window.getHeight() - 25));
		prestiegeBtn.initialize("", glm::vec2(window.getWidth()/2-120, 200), glm::vec2(280, 75));
		reset.initialize("res\\icons\\reset.png", glm::vec2(10, 60), glm::vec2(50));
		mute.initialize("res\\icons\\sound_off.png", glm::vec2(10, 110), glm::vec2(50));
		unmute.initialize("res\\icons\\sound_on.png", glm::vec2(10, 110), glm::vec2(50));

		//�����
		reset.background = reset.hover = glm::vec4(glm::vec3(0), 1);
		mute.background = mute.hover = glm::vec4(glm::vec3(0), 1);
		unmute.background = unmute.hover = glm::vec4(glm::vec3(0), 1);
		prestiegeBtn.background = glm::vec4(glm::vec3(0), 0.45f);
		prestiegeBtn.hover = glm::vec4(glm::vec3(1), 0.25f);

		//������� �� �������
		mute.setClickCallback([]() {
			bg.play();
			});
		unmute.setClickCallback([]() {
			bg.pause();
			});
		reset.setClickCallback([]() {
			LOG_INFO("Triggered reset");
			frog=&frog1;
			points=uPrestiege=setRainbowBG=triggeredAnEnding=
			showPrestButtons=upg.holdToClick=upg.idleGain=upg.multiFrog=
			randomizedCards=upgrade1=upgrade2=0;
			bg.play();
			});
		prestiegeBtn.setClickCallback([]() {
			if(points < 77777) {
				if(uPrestiege == 2) {
					showPrestButtons = true;
					bg.pause();
				} else prestiege();
			} else {
				bg.pause();
				if(!(upg.holdToClick&&upg.idleGain&&upg.multiFrog&&(uPrestiege==2)))
					upgradeSfxs.init("res/audio/an_ending.mp3")->play();
				else upgradeSfxs.init("res/audio/end_real.mp3")->play();
				triggeredAnEnding=true;
			}
			});
		upgradeBtn1.setClickCallback([]() {
			uPrestiege = 0;
			switch (upgrade1) {
			case 0:
				upg.holdToClick = true;
				break;
			case 1:
				upg.idleGain = true;
				break;
			case 2:
				upg.multiFrog = true;
				break;
			}
			prestiege();
			upgradeSfxs.init("res/audio/end_intermediate.mp3", 0.8f)->play();
			bg.play();
			});
		upgradeBtn1.setHoverCallback([]() {
			upgradeHovered=upgrade1;
			});
		upgradeBtn2.setClickCallback([]() {
			uPrestiege = 0;
			switch (upgrade2) {
			case 0:
				upg.holdToClick = true;
				break;
			case 1:
				upg.idleGain = true;
				break;
			case 2:
				upg.multiFrog = true;
				break;
			}
			prestiege();
			upgradeSfxs.init("res/audio/end_intermediate.mp3", 0.8f)->play();
			bg.play();
			});
		upgradeBtn2.setHoverCallback([]() {
			upgradeHovered=upgrade2;
			});

		//�����
		FSOAL::initialize();
		bg.init("res/audio/chess.mp3", 0.1f, true)->play();
		sfx.init("res/audio/click1.mp3", 0.6F);
		sfx2.init("res/audio/click2.mp3", 0.6F);

		//����������
		if (std::filesystem::exists("save")) {
			std::vector<std::string> saveFile = StrSplit(StrFromFile("save"),'\n');

			points = std::stoi(saveFile[0]);

			if(saveFile.size()>1) uPrestiege = std::stoi(saveFile[1]);
			if(uPrestiege == 0) frog = &frog1;
			else if(uPrestiege == 1) frog = &frog2;
			else if(uPrestiege == 2) frog = &frog3;

			if(saveFile.size() > 3) {
				upg.holdToClick = std::stoi(saveFile[2]);
				upg.idleGain = std::stoi(saveFile[3]);
				upg.multiFrog = std::stoi(saveFile[4]);
			}
		}
	}

	// ���������� �����
	void handleBG() {
		if (uPrestiege == 0){
			if (!setRainbowBG && points >= 2500) {
				setRainbowBG = true;
				Color = glm::vec3(0, 0, 1);
			}
			if (points < 100)		Color = glm::vec3(0.75f);				//White
			else if (points < 200)	Color = glm::vec3(0.8f, 0.8f, 0.15f);	//Yellow
			else if (points < 400)	Color = glm::vec3(0.15f, 0.15f, 0.8f);	//Blue
			else if (points < 600)	Color = glm::vec3(0.8f, 0.15f, 0.15f);	//Red
			else if (points < 800)	Color = glm::vec3(0.15f, 0.8f, 0.15f);	//Green
			else if (points < 1000)	Color = glm::vec3(0.45f, 0.15f, 0.2f);	//Purple
			else if (points < 2500)	Color = glm::vec3(0.85f, 0.3f, 0.3f);	//Pink
			else {
				if (Color.r >= 0.99f) state = 1;
				if (Color.g >= 0.99f) state = 2;
				if (Color.b >= 0.99f) state = 0;
				if (state == 0) Color += glm::vec3(0.0002f, 0, -0.0002f);
				if (state == 1) Color += glm::vec3(-0.0002f, 0.0002f, 0);
				if (state == 2) Color += glm::vec3(0, -0.0002f, 0.0002f);
			}
		}
		else if (uPrestiege == 1) {
			if (!setRainbowBG && points >= 2500) {
				setRainbowBG = true;
				Color = glm::vec3(0, 0, 1);
			}
			if (points < 100)		Color = HexToRGB("#8f430d");	//Brown
			else if (points < 200)	Color = HexToRGB("#2b4cb5");	//Blue
			else if (points < 400)	Color = HexToRGB("#641cd9");	//Purple
			else if (points < 600)	Color = HexToRGB("#8b911c");	//Yellow
			else if (points < 800)	Color = HexToRGB("#1c9126");	//Green
			else if (points < 1000)	Color = HexToRGB("#2fb1bd");	//Cyan
			else if (points < 2500)	Color = HexToRGB("#18c792");	//Cyan-Green
			else {
				if (Color.r >= 0.99f) state = 1;
				if (Color.g >= 0.99f) state = 2;
				if (Color.b >= 0.99f) state = 0;
				if (state == 0) Color += glm::vec3(0.0002f, 0, -0.0002f);
				if (state == 1) Color += glm::vec3(-0.0002f, 0.0002f, 0);
				if (state == 2) Color += glm::vec3(0, -0.0002f, 0.0002f);
			}
		} else {
			if (!setRainbowBG && points >= 2500) {
				setRainbowBG = true;
				Color = glm::vec3(0, 0, 1);
			}
			if (points < 100)		Color = HexToRGB("#454d4a");	//Dark-Gray
			else if (points < 200)	Color = HexToRGB("#731a1a");	//Dark-Red
			else if (points < 400)	Color = HexToRGB("#c7bf87");	//Light-Yellow
			else if (points < 600)	Color = HexToRGB("#1b4d27");	//Dark-Green
			else if (points < 800)	Color = HexToRGB("#1b4d41");	//Dark-Cyan
			else if (points < 1000)	Color = HexToRGB("#17247a");	//Dark-Blue
			else if (points < 2500)	Color = HexToRGB("#c44764");	//Light-Red
			else {
				if (Color.r >= 0.99f) state = 1;
				if (Color.g >= 0.99f) state = 2;
				if (Color.b >= 0.99f) state = 0;
				if (state == 0) Color += glm::vec3(0.0002f, 0, -0.0002f);
				if (state == 1) Color += glm::vec3(-0.0002f, 0.0002f, 0);
				if (state == 2) Color += glm::vec3(0, -0.0002f, 0.0002f);
			}
		}
		window.setClearColor(Color);
	}

	// ���������� ����������
	void updateUI(bool tButtonVisible) {
		//������ ��������
		if(tButtonVisible && !showPrestButtons) {
			prestiegeBtn.setPositon(glm::vec2(window.getWidth() / 2 - 120, 200));
			prestiegeBtn.update(window.getSize());
		}
		//�������� ��������
		if(showPrestButtons) {
			upgradeBtn1.setSize(glm::vec2(window.getWidth() * 0.2f, window.getHeight() * 0.35f));
			upgradeBtn2.setSize(glm::vec2(window.getWidth() * 0.2f, window.getHeight() * 0.35f));
			upgradeBtn1.setPositon(glm::vec2(70, upgradeBtn1.getSize().y + 32));
			upgradeBtn2.setPositon(glm::vec2(window.getWidth() - upgradeBtn2.getSize().x - 70, upgradeBtn2.getSize().y + 32));
			upgradeHovered=-1;
			upgradeBtn1.update(window.getSize());
			upgradeBtn2.update(window.getSize());
		}
		//���������
		reset.setPositon(glm::vec2(10, 60));
		reset.update(window.getSize());
		if(!bg.isPlaying()) mute.update(window.getSize());
		else unmute.update(window.getSize());
	}

	// �������� ������ � ������
	void controlledMovement() {
		//��������
		if(Size.y > 7 && !sizeState) {
			sizeState = true;
			sizeAccel = 0.00001f;
		} else if (Size.y <= 1 && sizeState) {
			sizeState = false;
			sizeAccel = 0.00001f;
		}
		//������
		if(!sizeState)
		Size += glm::vec3(sizeAccel * deltaTime);
		else Size -= glm::vec3(sizeAccel * deltaTime);
		sizeAccel += 0.00001f;
		if(impact > 0) impact -= 0.001f;
		frog->transform.size = glm::vec3(3 + std::clamp(points / 10000, 0U, 5U), 1, 1.2f) + Size + glm::vec3(impact);
		frog->transform.rotation += glm::vec3(0, 0.02f, 0.02f);
	}
	
	// ����������
	void handleInput() {
		if(Keyboard::keyDown(KeyCode::ESCAPE)) window.close();
		//�����
		bool recievedClick = upg.holdToClick?
			Mouse::getButton(0)||Keyboard::getKey(KeyCode::SPACEBAR)||Mouse::getButton(1) :
			Mouse::buttonDown(0)||Keyboard::keyDown(KeyCode::SPACEBAR)||Mouse::buttonDown(1);
		if(recievedClick && canClick) {
			//Causes some lag (because syncronizes at the end of function).
			auto c = cooldown(60);
			switch (uPrestiege) {
			case 0:
				points += 1;
				break;
			case 1:
				points += 2;
				if(upg.multiFrog) points += 1;
				break;
			case 2:
				points += 5;
				if(upg.multiFrog) points += 3;
				break;
			}
			if(impact <= 3.5f && Mouse::buttonDown(0)) impact += 0.25f;
			if(getRandom()) sfx.play();
			else sfx2.play();
		}
		//�������
		if(Keyboard::keyDown(KeyCode::M)) {
			if (bg.isPlaying()) bg.pause();
			else bg.play();
		}
		if(Keyboard::keyDown(KeyCode::F1))
			points *= 2;
		if(Keyboard::keyDown(KeyCode::F11)) {
			if (!fullscreen) {
				const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
				glfwSetWindowMonitor(window.ptr(), glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, 0);
				glfwSetWindowAttrib(window.ptr(), GLFW_DECORATED, GLFW_FALSE);
			} else {
				glfwSetWindowMonitor(window.ptr(), nullptr, 0, 0, 800, 600, 0);
				glfwSetWindowAttrib(window.ptr(), GLFW_DECORATED, GLFW_TRUE);
			}
			fullscreen = !fullscreen;
		}
	}

	// ��������� ������
	void drawUI(bool tButtonVisible) {
		//�������
		counter.draw(&text, std::to_string(points), window.getSize(),
			glm::vec2((window.getWidth() / 2) - 16 * (std::to_string(points).length()), window.getHeight() - 75), glm::vec2(1), glm::vec4(1));

		//�������
		if (tButtonVisible && !showPrestButtons) {
			//TODO: Add TextButton to `fs.ui`.
			prestiegeBtn.draw(&text, window.getSize());
			glDisable(GL_DEPTH_TEST);
			if(points < 77777)
				counter.draw(&text, "Prestiege", window.getSize(),
					glm::vec2(window.getWidth() / 2 - 100, 185), glm::vec2(1), glm::vec4(1));
			else
				counter.draw(&text, "Ascend", window.getSize(),
					glm::vec2(window.getWidth() / 2 - 67, 185), glm::vec2(1), glm::vec4(1));
			glEnable(GL_DEPTH_TEST);
		}
		//��������
		if (showPrestButtons) {
			if (!randomizedCards) {
				upgradeSfxs.init("res/audio/card_shuffle.mp3", 0.8f)->play();
				upgrade1 = rand() % upgradeCount;
				upgrade2 = rand() % upgradeCount;
				while(upgrade2==upgrade1) upgrade2 = rand() % upgradeCount;
				switch (upgrade1) {
				case 0:
					upgradeBtn1.background = glm::vec4(1,0,0, 1);
					break;
				case 1:
					upgradeBtn1.background = glm::vec4(0, 1, 0, 1);
					break;
				case 2:
					upgradeBtn1.background = glm::vec4(0, 0, 1, 1);
					break;
				}
				switch (upgrade2) {
				case 0:
					upgradeBtn2.background = glm::vec4(1, 0, 0, 1);
					break;
				case 1:
					upgradeBtn2.background = glm::vec4(0, 1, 0, 1);
					break;
				case 2:
					upgradeBtn2.background = glm::vec4(0, 0, 1, 1);
					break;
				}
				randomizedCards = true;
			}
			upgradeBtn1.draw(&text, window.getSize());
			upgradeBtn2.draw(&text, window.getSize());
			if(upgradeHovered!=-1){
				const char* desc = upgradeDescription[upgradeHovered];
				counter.draw(&text, desc, window.getSize(),
					glm::vec2((window.getWidth() / 2) - 8 * (std::string(desc).length()), 30), glm::vec2(0.5f), glm::vec4(1));
			}
		}

		//�� �� ��������� ������
		text.enable();
		text.setBool("dontApplyColor", true);
		reset.draw(&text, window.getSize());
		if (!bg.isPlaying()) mute.draw(&text, window.getSize());
		else unmute.draw(&text, window.getSize());
		text.enable();
		text.setBool("dontApplyColor", false);
	}

	// �� ����� ���� (������ �����)
	virtual void onUpdate() override {
		if(!triggeredAnEnding) {
			handleBG();
			
			//������ ��������
			bool buttonVisible =
				uPrestiege == 0 && points >= 3000 ||
				uPrestiege == 1 && points >= 10000 ||
				uPrestiege == 2 && points >= 12000;
			updateUI(buttonVisible);

			//���������� �������
			camera.aspect = window.aspect();
			controlledMovement();

			//����������
			handleInput();

			//Commented because causes a lot of lag (because syncronizes at the end of function).
			//auto x = idleGain(360);

			//��������� ������
			glm::mat4 proj = camera.getProjection(), view = camera.getView();
			base.enable();
			base.setMat4("projection", proj);
			base.setMat4("view", view);
			frog->draw(&base);

			//��������� ������
			drawUI(buttonVisible);
		} else {
			reset.setPositon(glm::vec2(window.getWidth()/2, window.getHeight()/2));
			reset.update(window.getSize());
			text.enable();
			if(!(upg.holdToClick&&upg.idleGain&&upg.multiFrog&&(uPrestiege==2)))
				window.setClearColor(glm::vec3(1, 0.01f, 0.01f));
			else window.setClearColor(glm::vec3(0.9f));
			reset.draw(&text, window.getSize());
		}
	}

	// ������� ���� �� ��
	virtual void onShutdown() override {
		base.remove();
		prestiegeBtn.remove();
		reset.remove();
		mute.remove();
		unmute.remove();
		upgradeBtn1.remove();
		upgradeBtn2.remove();
		counter.remove();
		sfx.remove();
		sfx2.remove();
		bg.remove();
		FSOAL::deinitialize();
		frog1.remove();
		frog2.remove();
		frog3.remove();
		StrToFile("save",
			std::to_string(points) + "\n"
			+ std::to_string(uPrestiege) + "\n"
			+ std::to_string(upg.holdToClick) + "\n"
			+ std::to_string(upg.idleGain) + "\n"
			+ std::to_string(upg.multiFrog));
	}
	
};

// ��������� ����������
Entity FROGAPP::frog1 = Entity{ glm::vec3(0,0,-2), glm::vec3(90,180,45), glm::vec3(3,1,1.2f) };
Entity FROGAPP::frog2 = Entity{ glm::vec3(0,0,-2), glm::vec3(0,225,0), glm::vec3(3,1,1.2f) };
Entity FROGAPP::frog3 = Entity{ glm::vec3(0,0,-2), glm::vec3(0,225,0), glm::vec3(3,1,1.2f) };
unsigned int FROGAPP::points=0, FROGAPP::uPrestiege=0, FROGAPP::upgrade1 = 0, FROGAPP::upgrade2 = 0;
int FROGAPP::upgradeHovered = -1;
bool FROGAPP::setRainbowBG = 0, FROGAPP::showPrestButtons = 0, FROGAPP::randomizedCards=0, FROGAPP::canClick = 1, FROGAPP::canIdleGain=1, FROGAPP::triggeredAnEnding = 0;
Entity* FROGAPP::frog = nullptr;
Source FROGAPP::bg{}, FROGAPP::upgradeSfxs{};
FROGAPP::Upgrades FROGAPP::upg{};

// ��������� ����� � ������
const size_t winNameVariantsNum = 11;
const char* winNameVariants[winNameVariantsNum] = {
	"You need to burn your pc, now.",
	"Also try &^@#$*%!",
	"You burned your pc? Good. Now disintegrate yourself.",
	"With a fine layer of BBQ.",
	"Better hot.",
	"Better cold.",
	"Never gonna throw you out of the window.",
	"*beep*",
	"*womp*,*womp*",
	"Get gnomed!",
	"GET DUNKED OOOOOOOOOON!!!"
};

int main() {
	__time64_t long_time;
	_time64(&long_time);
	srand(static_cast<unsigned int>(long_time));
	return FROGAPP{}.start(winNameVariants[rand() % winNameVariantsNum]);
}
