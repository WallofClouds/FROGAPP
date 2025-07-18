#include "embedded.hpp"
#include "engine/include/app.hpp"
#include "engine/include/entity.hpp"
#include "engine/include/camera.hpp"
#include "engine/include/input/input.hpp"
#include "engine/include/utils/utils.hpp"
#include "include/ambient.hpp"
#include "include/ui.hpp"
#include "include/datatypes.hpp"
#include <fs.ui/include/text.hpp>
#include <fs.ui/include/generic.hpp>
#include <openal/include/listener.hpp>
#include <openal/include/source.hpp>
#include <time.h>
#include <future>
#include <chrono>
#include <thread>

using namespace Firesteel;
using namespace FSOAL;

unsigned int obtainedUpgradeCount=0;

class FROGADROID : public Firesteel::App {
	// Local variables
	bool sizeState=false, fullscreen=false, recievedClick=false;
	int state=0;
	float sizeAccel=0.00001f, impact=0, speeenMultiFrog=0;
	glm::vec3 Color{0,0,1}, Size{0};
	Shader base, text;
	Camera camera{glm::vec3(0),glm::vec3(0,0,-90)};
	Text counter;
	Source sfx, sfx2;
	Button prestiegeBtn, resetBtn, muteBtn, unmuteBtn, upgradeBtn1, upgradeBtn2;
	// Static variables
	static bool isBgRainbow, showUpgradeCards, randomizedCards, canClick, triggeredAnEnding;
	static int upgradeHovered;
	static float oldThemeVal;
	static unsigned int upgrade1, upgrade2;
	static Timer holdClickCooldown, idleCooldown;
	static SaveData save;
	static Entity* displayFrog;
	static Entity frogVariants[3];
	static Source bg, upgradeSfxs;

	// Handle user input
	void handleInput() {
		if(Keyboard::keyDown(KeyCode::ESCAPE)) window.close();
		//Idle gain
		if(save.idleGain()&&idleCooldown.isOver()) {
			switch(save.prestiege) {
			case 0:
				save.points+=1;
				break;
			case 1:
				save.points+=3;
				break;
			case 2:
				save.points+=8;
				break;
			}
			idleCooldown.start();
		} else if(save.idleGain()) idleCooldown.tick(deltaTime);
		//Clicks
		recievedClick=save.holdToClick()?
			Mouse::getButton(0)||Keyboard::getKey(KeyCode::SPACEBAR)||Mouse::getButton(1) :
			Mouse::buttonDown(0)||Keyboard::keyDown(KeyCode::SPACEBAR)||Mouse::buttonDown(1);
		//Hold to click
		if(save.holdToClick()&&holdClickCooldown.isOver()) {
			canClick=true;
			if(recievedClick) holdClickCooldown.start();
		} else if(save.holdToClick()) {
			canClick=false;
			holdClickCooldown.tick(deltaTime);
		} else {
			canClick=true;
			holdClickCooldown.reset();
		}
		//Handle click
		if(recievedClick && canClick) {
			switch(save.prestiege) {
			case 0:
				save.points += 1;
				break;
			case 1:
				save.points += 2;
				if(save.multiFrog()) save.points += 1;
				break;
			case 2:
				save.points += 5;
				if(save.multiFrog()) save.points += 3;
				break;
			}
			if(impact <= 3.5f && recievedClick && canClick) impact += 0.25f;
			if(getRandom()) sfx.play();
			else sfx2.play();
		}
		//Quality of Life
		if(Keyboard::keyDown(KeyCode::M)) {
			if(bg.isPlaying()) bg.pause();
			else bg.play();
		}
		if(Keyboard::keyDown(KeyCode::F1)) save.points *= 2;
		if(Keyboard::keyDown(KeyCode::F11)) {
			if(!fullscreen) {
				const GLFWvidmode* mode=glfwGetVideoMode(glfwGetPrimaryMonitor());
				glfwSetWindowMonitor(window.ptr(), glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, 0);
				glfwSetWindowAttrib(window.ptr(), GLFW_DECORATED, GLFW_FALSE);
			} else {
				glfwSetWindowMonitor(window.ptr(), nullptr, 0, 0, 800, 600, 0);
				glfwSetWindowAttrib(window.ptr(), GLFW_DECORATED, GLFW_TRUE);
			}
			fullscreen=!fullscreen;
		}
	}
	// On button click
	static void prestiegeEvent() {
		LOG_INFO("Triggered prestiege");
		//Pseudo-reset
		oldThemeVal=2;
		save.points=0;
		isBgRainbow=false;
		randomizedCards=false;
		frogVariants[0].transform.position=
		frogVariants[1].transform.position=
		frogVariants[2].transform.position=
		glm::vec3(0,0,-2);
		//Check prestiege
		if(!showUpgradeCards) save.prestiege += 1;
		showUpgradeCards=false;
		switch(save.prestiege) {
		case 0:
			displayFrog=&frogVariants[0];
			break;
		case 1:
			displayFrog=&frogVariants[1];
			break;
		default:
			displayFrog=&frogVariants[2];
			break;
		}
	}
	
	// Very complex upgrade randomization system (whataheil)
	void randomizeUpgrades() {
		//Randomize first upg. Test if it matches other already obtained upgrades.
		while(save.upg[upgrade1]) upgrade1=rand()%upgradeCount;
		//If there remains only one not obtained upgrade - stop here.
		//Otherwise it'll go in infinite loop.
		if(obtainedUpgradeCount==upgradeCount-1) return;
		//For the second check if it's already present or matches the first one.
		while(save.upg[upgrade2]||upgrade2==upgrade1) upgrade2=rand()%upgradeCount;
	}

	// Runs after window and renderer initialization.
	virtual void onInitialize() override {
		window.setVSync(true);
		window.setIconFromMemory(ucIconData, ucIconDataSize);
		//Initialize rendering stuff
		frogVariants[0].load("res\\frogs\\tiny_frog\\scene.gltf");
		frogVariants[1].load("res\\frogs\\cartoon_frog\\cartoon_frog.obj");
		frogVariants[2].load("res\\frogs\\giga_frog\\giga_frog.obj");
		displayFrog=&frogVariants[0];
		base=Shader("res/shaders/base.vs", "res/shaders/base.fs"), text=Shader("res/shaders/text.vs", "res/shaders/text.fs");
		camera.update();
		//Load the save
		save.load();
		switch(save.prestiege) {
		case 0:
			displayFrog=&frogVariants[0];
			break;
		case 1:
			displayFrog=&frogVariants[1];
			break;
		default:
			displayFrog=&frogVariants[2];
			break;
		}
		//Initialize UI
		TextRenderer::initialize();
		counter.loadFont("res/fonts/FatPixelFont.ttf", 16);
		upgradeBtn1.initialize("",
			glm::vec2(1),
			glm::vec2(1),
			-25.f);
		upgradeBtn2.initialize("",
			glm::vec2(1),
			glm::vec2(1),
			25.f);
		prestiegeBtn.initialize("", glm::vec2(window.getWidth()/2-120, 200), glm::vec2(280, 75));
		resetBtn.initialize("res\\icons\\reset.png", glm::vec2(10, 60), glm::vec2(50));
		muteBtn.initialize("res\\icons\\sound_off.png", glm::vec2(10, 110), glm::vec2(50));
		unmuteBtn.initialize("res\\icons\\sound_on.png", glm::vec2(10, 110), glm::vec2(50));
		//Set UI styles
		prestiegeBtn.background=glm::vec4(glm::vec3(0), 0.45f);
		prestiegeBtn.hover=glm::vec4(glm::vec3(1), 0.25f);
		//Set UI events
		muteBtn.setClickCallback([]() {bg.play();});
		unmuteBtn.setClickCallback([]() {bg.pause();});
		resetBtn.setClickCallback([]() {
			LOG_INFO("Triggered reset");
			displayFrog=&frogVariants[0];
			save.points=save.prestiege=isBgRainbow=triggeredAnEnding=obtainedUpgradeCount=
			showUpgradeCards=save.upg[0]=save.upg[1]=save.upg[2]=save.upg[3]=
			randomizedCards=upgrade1=upgrade2=0;
			holdClickCooldown.limit=0.5;
			frogVariants[0].transform.position=
				frogVariants[1].transform.position=
				frogVariants[2].transform.position=
				glm::vec3(0,0,-2);
			upgradeSfxs.stop();
			bg.play();
		});
		prestiegeBtn.setClickCallback([]() {
			if(save.points<77777) {
				if(save.prestiege == 2) {
					showUpgradeCards=true;
					bg.pause();
				} else prestiegeEvent();
			} else {
				bg.pause();
				if(!(save.holdToClick()&&
					save.idleGain()&&
					save.multiFrog()&&
					(save.prestiege==2))) upgradeSfxs.init("res/audio/an_ending.mp3")->play();
				else upgradeSfxs.init("res/audio/end_real.mp3")->play();
				triggeredAnEnding=true;
			}
		});
		upgradeBtn1.setClickCallback([]() {
			save.obtain(upgrade1);
			save.prestiege=0;
			prestiegeEvent();
			upgradeSfxs.init("res/audio/end_intermediate.mp3", 0.8f)->play();
			bg.play();
		});
		upgradeBtn1.setHoverCallback([]() {upgradeHovered=upgrade1;});
		upgradeBtn2.setClickCallback([]() {
			save.obtain(upgrade2);
			save.prestiege=0;
			prestiegeEvent();
			upgradeSfxs.init("res/audio/end_intermediate.mp3", 0.8f)->play();
			bg.play();
		});
		upgradeBtn2.setHoverCallback([]() {upgradeHovered=upgrade2;});
		//Initialize audio
		FSOAL::initialize();
		bg.init("res/audio/chess.mp3", 0.1f, true)->play();
		sfx.init("res/audio/click1.mp3", 0.6F);
		sfx2.init("res/audio/click2.mp3", 0.6F);
	}
	// Runs each frame.
	virtual void onUpdate() override {
		if(!triggeredAnEnding) {
			handleBG(Color, isBgRainbow, save.points, save.prestiege);
			//Check if prestiege button should be visible
			bool buttonVisible =
				(save.prestiege == 0 && save.points >= 3000) ||
				(save.prestiege == 1 && save.points >= 10000) ||
				(save.prestiege == 2 && save.points >= 12000);
			//Handle updates
			if(save.betterHoldToClick()) holdClickCooldown.limit=0.3;
			updateUI(buttonVisible);
			controlledMovement(deltaTime, save.points, displayFrog, Size);
			handleInput();
			//Prepare for a draw call
			camera.aspect=window.aspect();
			glm::mat4 proj=camera.getProjection(), view=camera.getView();
			//Draw. The. Frog.
			base.enable();
			base.setMat4("projection", proj);
			base.setMat4("view", view);
			displayFrog->draw(&base);
			if(save.multiFrog()) {
				if(save.prestiege>=1) {
					frogVariants[0].transform.size=glm::vec3(0.8f);
					frogVariants[0].transform.rotation=-displayFrog->transform.rotation;
					frogVariants[0].transform.position=glm::vec3(
						1.25f*cos(speeenMultiFrog*(3.14f/180.f)),
						1.25f*sin(speeenMultiFrog*(3.14f/180.f)),
						-4
					);
					frogVariants[0].draw(&base);
				}
				if(save.prestiege==2) {
					frogVariants[1].transform.size=glm::vec3(0.8f);
					frogVariants[1].transform.rotation=-displayFrog->transform.rotation;
					frogVariants[1].transform.position=glm::vec3(
						1.25f*cos((speeenMultiFrog-180.f)*(3.14f/180.f)),
						1.25f*sin((speeenMultiFrog-180.f)*(3.14f/180.f)),
						-4
					);
					frogVariants[1].draw(&base);
				}
				speeenMultiFrog+=0.035f;
				if(speeenMultiFrog>=360.f) speeenMultiFrog=0;
			}
			//Almost forgot about UI
			drawUI(buttonVisible);
		} else {
			//Handle ending screen
			resetBtn.setPositon(glm::vec2(window.getWidth()/2, window.getHeight()/2));
			resetBtn.update(window.getSize());
			//Prepare for a draw call
			text.enable();
			if(!(save.hasAll()&&(save.prestiege==2)))
				window.setClearColor(glm::vec3(1, 0.01f, 0.01f));
			else window.setClearColor(glm::vec3(0.9f));
			//Draw reset button
			resetBtn.draw(&text, window.getSize());
		}
		window.setClearColor(Color);
	}
	// Runs after window.close() is called or on window closing.
	virtual void onShutdown() override {
		//Destroy 3d stuff
		for(size_t f=0;f<3; f++) frogVariants[f].remove();
		base.remove();
		//Remove UI
		prestiegeBtn.remove();
		resetBtn.remove();
		muteBtn.remove();
		unmuteBtn.remove();
		upgradeBtn1.remove();
		upgradeBtn2.remove();
		counter.remove();
		text.remove();
		//Remove audio
		sfx.remove();
		sfx2.remove();
		bg.remove();
		FSOAL::deinitialize();
		save.save();
	}
	
};

/// Initialize static variables
Entity FROGADROID::frogVariants[3]={
	Entity{glm::vec3(0,0,-2), glm::vec3(90,180,45), glm::vec3(3,1,1.2f)},
	Entity{glm::vec3(0,0,-2), glm::vec3(0,225,0), glm::vec3(3,1,1.2f)},
	Entity{glm::vec3(0,0,-2), glm::vec3(0,225,0), glm::vec3(3,1,1.2f)}
};
unsigned int FROGADROID::upgrade1=0, FROGADROID::upgrade2=0;
Timer FROGADROID::holdClickCooldown{0,0.5}, FROGADROID::idleCooldown{0,5};
int FROGADROID::upgradeHovered=-1;
float FROGADROID::oldThemeVal=2;
bool FROGADROID::isBgRainbow=false, FROGADROID::showUpgradeCards=false, FROGADROID::randomizedCards=false, FROGADROID::triggeredAnEnding=false,
	FROGADROID::canClick=true;
Entity* FROGADROID::displayFrog=nullptr;
Source FROGADROID::bg{}, FROGADROID::upgradeSfxs{};
SaveData FROGADROID::save{};

int main() {
	__time64_t long_time;
	_time64(&long_time);
	srand(static_cast<unsigned int>(long_time));
	return FROGADROID{}.start(winNameVariants[rand()%winNameVariantsNum]);
}
