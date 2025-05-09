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

class FROGAPP : public App {

	// Глобальные переменные
	glm::vec3 Color{0,0,1};
	glm::vec3 Size{ 0 };
	int state = 0;
	float sizeAccel = 0.00001f, impact = 0;
	bool sizeState = false, fullscreen = false;
	static Entity frog1, frog2, frog3;
	static Entity* frog;
	static Source bg;
	static bool setRainbowBG, showPrestButtons, randomizedCards;
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
	static unsigned int points, uPrestiege, selectedFrog, upgrade1, upgrade2;
	Shader base, text;
	Camera camera{glm::vec3(0),glm::vec3(0,0,-90)};
	Text counter;
	Source sfx, sfx2;
	Button prestiegeBtn, reset, mute, unmute, upgradeBtn1, upgradeBtn2;
	
	// Престиж
	static void prestiege() {
		points = 0;
		setRainbowBG = false;
		if(!showPrestButtons) uPrestiege += 1;
		if (uPrestiege == 0) {
			frog = &frog1;
			selectedFrog = 0;
		}
		if (uPrestiege == 1) {
			frog = &frog2;
			selectedFrog = 1;
		}
		if (uPrestiege == 2) {
			frog = &frog3;
			selectedFrog = 2;
		}
		showPrestButtons = false;
		randomizedCards = false;
	}

	// Во время запуска
	virtual void onInitialize() override {
		window.setIconFromMemory(ucIconData, ucIconDataSize);
		__time64_t long_time;
		_time64(&long_time);
		srand(long_time);		

		//Загрузка лягух
		frog1.load("res\\frogs\\tiny_frog\\scene.gltf");
		frog2.load("res\\frogs\\cartoon_frog\\cartoon_frog.obj");
		frog3.load("res\\frogs\\giga_frog\\giga_frog.obj");
		frog = &frog1;
		base = Shader("res/shaders/base.vs", "res/shaders/base.fs"), text = Shader("res/shaders/text.vs", "res/shaders/text.fs");
		camera.update();

		//Интерфейс
		TextRenderer::initialize();
		counter.loadFont("res/fonts/FatPixelFont.ttf", 16);
		upgradeBtn1.initialize("", glm::vec2(225, window.getHeight() - 100), glm::vec2(window.getWidth()/3, window.getHeight()-25));
		upgradeBtn2.initialize("", glm::vec2(window.getWidth() - 100, window.getHeight() - 100), glm::vec2(window.getWidth() / 3, window.getHeight() - 25));
		prestiegeBtn.initialize("", glm::vec2(window.getWidth()/2-120, 200), glm::vec2(280, 75));
		reset.initialize("res\\icons\\reset.png", glm::vec2(10, 60), glm::vec2(50));
		mute.initialize("res\\icons\\sound_off.png", glm::vec2(10, 110), glm::vec2(50));
		unmute.initialize("res\\icons\\sound_on.png", glm::vec2(10, 110), glm::vec2(50));

		//Стили
		reset.background = reset.hover = glm::vec4(glm::vec3(0), 1);
		mute.background = mute.hover = glm::vec4(glm::vec3(0), 1);
		unmute.background = unmute.hover = glm::vec4(glm::vec3(0), 1);
		prestiegeBtn.background = glm::vec4(glm::vec3(0), 0.45f);
		prestiegeBtn.hover = glm::vec4(glm::vec3(1), 0.25f);

		//Отклики на нажатие
		mute.setClickCallback([]() {
			bg.play();
			});
		unmute.setClickCallback([]() {
			bg.pause();
			});
		reset.setClickCallback([]() {
			points = 0;
			uPrestiege = 0;
			selectedFrog = 0;
			frog = &frog1;
			setRainbowBG = false;
			showPrestButtons = false; 
			upg.holdToClick = false, upg.idleGain = false, upg.multiFrog = false;
			randomizedCards = 0;
			upgrade1 = 0, upgrade2 = 0;
			});
		prestiegeBtn.setClickCallback([]() {
			if (uPrestiege == 2)
				showPrestButtons = true;
			else prestiege();
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
			});

		//Аудио
		FSOAL::initialize();
		bg.init("res/audio/chess.mp3", 0.1f, true)->play();
		sfx.init("res/audio/click1.mp3", 0.6F);
		sfx2.init("res/audio/click2.mp3", 0.6F);

		//Сохранения
		if (std::filesystem::exists("save")) {
			std::vector<std::string> saveFile = StrSplit(StrFromFile("save"),'\n');
			sscanf(saveFile[0].c_str(), "%zu", &points);
			if(saveFile.size()>1) sscanf(saveFile[1].c_str(), "%zu", &uPrestiege);
			if (uPrestiege == 1) {
				frog = &frog2;
				selectedFrog = 1;
			}
			if (uPrestiege == 2) {
				frog = &frog3;
				selectedFrog = 2;
			}
			if (saveFile.size() > 3) {
				sscanf(saveFile[2].c_str(), "%zu", &upg.holdToClick);
				sscanf(saveFile[3].c_str(), "%zu", &upg.idleGain);
				sscanf(saveFile[4].c_str(), "%zu", &upg.multiFrog);
			}
		}

		//Типы лягух в зависимости от престижа
		if(uPrestiege == 0) frog = &frog1;
		else if(uPrestiege == 1) frog = &frog2;
		else if(uPrestiege == 2) frog = &frog3;
	}

	// Управление фоном
	void handleBG() {
		if (selectedFrog == 0){
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
		else if (selectedFrog == 1) {
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
		}
		else if (selectedFrog == 2) {
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

	// Обновление интерфейса
	void updateUI(bool tButtonVisible) {
		//Кнопка престижа
		if(tButtonVisible && !showPrestButtons) {
			prestiegeBtn.setPositon(glm::vec2(window.getWidth() / 2 - 120, 200));
			prestiegeBtn.update(window.getSize());
		}
		//Вариации престижа
		if(showPrestButtons) {
			upgradeBtn1.setSize(glm::vec2(window.getWidth() * 0.2f, window.getHeight() * 0.35f));
			upgradeBtn2.setSize(glm::vec2(window.getWidth() * 0.2f, window.getHeight() * 0.35f));
			upgradeBtn1.setPositon(glm::vec2(70, upgradeBtn1.getSize().y + 12.5f));
			upgradeBtn2.setPositon(glm::vec2(window.getWidth() - upgradeBtn2.getSize().x - 70, upgradeBtn2.getSize().y + 12.5f));
			upgradeBtn1.update(window.getSize());
			upgradeBtn2.update(window.getSize());
		}
		//Настройки
		reset.update(window.getSize());
		if(!bg.isPlaying()) mute.update(window.getSize());
		else unmute.update(window.getSize());
	}

	// Вращение лягухи и импакт
	void controlledMovement() {
		//Вращение
		frog->transform.rotation += glm::vec3(0, 0.02f, 0.02f);
		if (frog->transform.size.y > 7 && !sizeState) {
			sizeState = true;
			sizeAccel = 0.00001f;
		}
		else if (frog->transform.size.y < 1 && sizeState) {
			sizeState = false;
			sizeAccel = 0.00001f;
		}
		//Импакт
		if (!sizeState)
			Size += glm::vec3(sizeAccel * deltaTime);
		else Size -= glm::vec3(sizeAccel * deltaTime);
		sizeAccel += 0.00001f;
		if (impact > 0) impact -= 0.001f;
		frog->transform.size = glm::vec3(3 + std::clamp(points / 10000, 0U, 5U), 1, 1.2f) + Size + glm::vec3(impact);
	}

	// Управление
	void handleInput() {
		//Клики
		if((Mouse::buttonDown(0) || Keyboard::keyDown(KeyCode::SPACEBAR) || Mouse::buttonDown(1))&&!upg.holdToClick
		|| (Mouse::getButton(0) || Keyboard::getKey(KeyCode::SPACEBAR) || Mouse::getButton(1))&&upg.holdToClick) {
			switch (selectedFrog) {
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
			if (impact <= 3.5f) impact += 0.25f;
			if (rand() % 2 == 0) sfx.play();
			else sfx2.play();
		}
		//Клавиши
		if(Keyboard::keyDown(KeyCode::M)) {
			if (bg.isPlaying()) bg.pause();
			else bg.play();
		}
		if(Keyboard::keyDown(KeyCode::ESCAPE)) {
			window.close();
		}
		if(Keyboard::keyDown(KeyCode::F1))
			points += points * 0.5f;
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

	// Отрисовка кнопок
	void drawUI(bool tButtonVisible) {

		//Счётчик
		counter.draw(&text, std::to_string(points), window.getSize(),
			glm::vec2((window.getWidth() / 2) - 16 * (std::to_string(points).length()), window.getHeight() - 75), glm::vec2(1), glm::vec4(1));

		//Престиж
		if (tButtonVisible && !showPrestButtons) {
			prestiegeBtn.draw(&text, window.getSize());
			glDisable(GL_DEPTH_TEST);
			counter.draw(&text, "Prestiege", window.getSize(),
				glm::vec2(window.getWidth() / 2 - 100, 185), glm::vec2(1), glm::vec4(1));
			glEnable(GL_DEPTH_TEST);
		}
		//Картонки
		if (showPrestButtons) {
			if (!randomizedCards) {
				upgrade1 = rand() % upgradeCount;
				upgrade2 = rand() % upgradeCount;
				if(upgrade2==upgrade1) upgrade2 = rand() % upgradeCount;
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
		}

		//Хз чё настройки наверн
		text.enable();
		text.setBool("dontApplyColor", true);
		reset.draw(&text, window.getSize());
		if (!bg.isPlaying()) mute.draw(&text, window.getSize());
		else unmute.draw(&text, window.getSize());
	}

	// Во время игры (каждый фрейм)
	virtual void onUpdate() override {
		handleBG();
		
		//Кнопка престижа
		bool buttonVisible =
			uPrestiege == 0 && points >= 3000 ||
			uPrestiege == 1 && points >= 10000 ||
			uPrestiege == 2 && points >= 12000;
		updateUI(buttonVisible);

		//Управление лягухой
		camera.aspect = window.aspect();
		controlledMovement();

		//Управление
		handleInput();

		//Отрисовка лягухи
		glm::mat4 proj = camera.getProjection(), view = camera.getView();
		base.enable();
		base.setMat4("projection", proj);
		base.setMat4("view", view);
		frog->draw(&base);

		//Отрисовка кнопок
		drawUI(buttonVisible);
	}

	// Выключи свой пк пж
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
		StrToFile("save",
			std::to_string(points) + "\n"
			+ std::to_string(uPrestiege) + "\n"
			+ std::to_string(upg.holdToClick) + "\n"
			+ std::to_string(upg.idleGain) + "\n"
			+ std::to_string(upg.multiFrog));
		frog1.remove();
		frog2.remove();
		frog3.remove();
	}
	
};

// Статичные переменные
Entity FROGAPP::frog1 = Entity{ glm::vec3(0,0,-2), glm::vec3(90,180,45), glm::vec3(3,1,1.2f) };
Entity FROGAPP::frog2 = Entity{ glm::vec3(0,0,-2), glm::vec3(0,225,0), glm::vec3(3,1,1.2f) };
Entity FROGAPP::frog3 = Entity{ glm::vec3(0,0,-2), glm::vec3(0,225,0), glm::vec3(3,1,1.2f) };
unsigned int FROGAPP::points=0, FROGAPP::uPrestiege=0, FROGAPP::selectedFrog=0, FROGAPP::upgrade1 = 0, FROGAPP::upgrade2 = 0;
bool FROGAPP::setRainbowBG = 0, FROGAPP::showPrestButtons = 0, FROGAPP::randomizedCards=0;
Entity* FROGAPP::frog = nullptr;
Source FROGAPP::bg{};
FROGAPP::Upgrades FROGAPP::upg{};

// Рандомный текст в начале
const size_t winNameVariantsNum = 10;
const char* winNameVariants[winNameVariantsNum] = {
	"You need to burn your pc, now.",
	"Also try &^@#$*%!",
	"You burned your pc? Good. Now disintegrate yourself.",
	"With fine layer of BBQ.",
	"You burned your pc? Good. Now kys.",
	"Never gonna throw you into the window.",
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
