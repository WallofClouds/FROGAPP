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

/// Random window titles
const size_t winNameVariantsNum=12;
const char* winNameVariants[winNameVariantsNum]={
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
	"GET DUNKED OOOOOOOOOON!!!",
	"Try Terraria Infernum \U+1F480"
};
/// Upgrades
const unsigned int upgradeCount=4;
unsigned int obtainedUpgradeCount=0;
const char* upgradeDescription[upgradeCount]={
	"Hold any mouse button/spacebar to gain points",
	"Get points from all the frogs",
	"Frogs idly generate points",
	"Get points faster when holding"
};
glm::vec3 upgradeColor[upgradeCount]={
	glm::vec3(1,0,0),
	glm::vec3(0,1,0),
	glm::vec3(0,0,1),
	glm::vec3(1,0,0.5f)
};
/// Custom data types
struct Timer {
	double time=0.0;
	double limit=60.0;

	void start() {time=limit;}
	void reset() {time=0;}
	void tick() {time-=1;}
	void tick(double tDT) {time-=tDT;}
	bool isOver() {return time<=0;}
};
struct SaveData {
	unsigned int points;
	unsigned int prestiege;
	bool upg[upgradeCount];

	void obtain(unsigned int tId) {
		if(tId>=upgradeCount) return;
		obtainedUpgradeCount+=1;
	}
	bool holdToClick() {return upg[0];}
	bool multiFrog() {return upg[1];}
	bool idleGain() {return upg[2];}
	bool betterHoldToClick() {return upg[3];}
	bool hasAll() {
		for(unsigned int i=0;i<upgradeCount;i++)
			if(!upg[i]) return false;
		return true;
	}

	void load() {
		if(!std::filesystem::exists("save")) return;
		//Parse save if it exists
		std::vector<std::string> saveFile=StrSplit(StrFromFile("save"),'\n');
		points=std::stoi(saveFile[0]);
		if(saveFile.size()>1) prestiege=std::stoi(saveFile[1]);
		//Load upgrades
		if(saveFile.size()>2) {
			upg[0]=std::stoi(saveFile[2]);
			upg[1]=std::stoi(saveFile[3]);
			upg[2]=std::stoi(saveFile[4]);
		}
		if(saveFile.size()>5) upg[3]=std::stoi(saveFile[5]);
		//Enumerate obtained updates
		for(unsigned int i=0;i<upgradeCount;i++)
			if(upg[i]) obtainedUpgradeCount+=1;
	}
	void save() {
		StrToFile("save",
			std::to_string(points) + "\n"
			+ std::to_string(prestiege) + "\n"
			+ std::to_string(holdToClick()) + "\n"
			+ std::to_string(multiFrog()) + "\n"
			+ std::to_string(idleGain()) + "\n"
			+ std::to_string(betterHoldToClick()));
	}
};

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
	// Have fun with background
	void handleBG() {
		unsigned int& points=save.points;
		switch(save.prestiege) {
		case 0:
			if(!isBgRainbow && points >= 2500) {
				isBgRainbow=true;
				Color=glm::vec3(0, 0, 1);
			}
			if(points<100)		Color=glm::vec3(0.75f);					//White
			else if(points<200)	Color=glm::vec3(0.8f, 0.8f, 0.15f);		//Yellow
			else if(points<400)	Color=glm::vec3(0.15f, 0.15f, 0.8f);	//Blue
			else if(points<600)	Color=glm::vec3(0.8f, 0.15f, 0.15f);	//Red
			else if(points<800)	Color=glm::vec3(0.15f, 0.8f, 0.15f);	//Green
			else if(points<1000)	Color=glm::vec3(0.45f, 0.15f, 0.2f);	//Purple
			else if(points<2500)	Color=glm::vec3(0.85f, 0.3f, 0.3f);		//Pink
			else {
				if(Color.r >= 0.99f) state=1;
				if(Color.g >= 0.99f) state=2;
				if(Color.b >= 0.99f) state=0;
				if(state == 0) Color += glm::vec3(0.0002f, 0, -0.0002f);
				if(state == 1) Color += glm::vec3(-0.0002f, 0.0002f, 0);
				if(state == 2) Color += glm::vec3(0, -0.0002f, 0.0002f);
			}
			break;
		case 1:
			if(!isBgRainbow && points >= 2500) {
				isBgRainbow=true;
				Color=glm::vec3(0, 0, 1);
			}
			if(points<100)		Color=HexToRGB("#8f430d");	//Brown
			else if(points<200)	Color=HexToRGB("#2b4cb5");	//Blue
			else if(points<400)	Color=HexToRGB("#641cd9");	//Purple
			else if(points<600)	Color=HexToRGB("#8b911c");	//Yellow
			else if(points<800)	Color=HexToRGB("#1c9126");	//Green
			else if(points<1000)	Color=HexToRGB("#2fb1bd");	//Cyan
			else if(points<2500)	Color=HexToRGB("#18c792");	//Cyan-Green
			else {
				if(Color.r >= 0.99f) state=1;
				if(Color.g >= 0.99f) state=2;
				if(Color.b >= 0.99f) state=0;
				if(state == 0) Color += glm::vec3(0.0002f, 0, -0.0002f);
				if(state == 1) Color += glm::vec3(-0.0002f, 0.0002f, 0);
				if(state == 2) Color += glm::vec3(0, -0.0002f, 0.0002f);
			}
			break;
		default:
			if(!isBgRainbow && points >= 2500) {
				isBgRainbow=true;
				Color=glm::vec3(0, 0, 1);
			}
			if(points<100)		Color=HexToRGB("#454d4a");	//Dark-Gray
			else if(points<200)	Color=HexToRGB("#731a1a");	//Dark-Red
			else if(points<400)	Color=HexToRGB("#c7bf87");	//Light-Yellow
			else if(points<600)	Color=HexToRGB("#1b4d27");	//Dark-Green
			else if(points<800)	Color=HexToRGB("#1b4d41");	//Dark-Cyan
			else if(points<1000)	Color=HexToRGB("#17247a");	//Dark-Blue
			else if(points<2500)	Color=HexToRGB("#c44764");	//Light-Red
			else {
				if(Color.r >= 0.99f) state=1;
				if(Color.g >= 0.99f) state=2;
				if(Color.b >= 0.99f) state=0;
				if(state == 0) Color += glm::vec3(0.0002f, 0, -0.0002f);
				if(state == 1) Color += glm::vec3(-0.0002f, 0.0002f, 0);
				if(state == 2) Color += glm::vec3(0, -0.0002f, 0.0002f);
			}
			break;
		}
		window.setClearColor(Color);
	}
	// Move and rotate the frog.
	void controlledMovement() {
		//Control size
		if(Size.y>7 && !sizeState) {
			sizeState=true;
			sizeAccel=0.00001f;
		} else if(Size.y <= 1 && sizeState) {
			sizeState=false;
			sizeAccel=0.00001f;
		}
		//Determine if frog should expand or shrink
		if(!sizeState) Size += glm::vec3(sizeAccel * deltaTime);
		else Size -= glm::vec3(sizeAccel * deltaTime);
		//Update knwon acceleration
		sizeAccel += 0.00001f;
		if(impact>0) impact -= 0.001f;
		//Apply new knowledge
		displayFrog->transform.size=glm::vec3(3 + std::clamp(save.points/10000, 0U, 5U), 1, 1.2f) + Size + glm::vec3(impact);
		displayFrog->transform.rotation += glm::vec3(0, 0.02f, 0.02f);
	}
	// Setup UI positions
	void updateUI(bool tButtonVisible) {
		//Is prestiege button needed?
		if(tButtonVisible && !showUpgradeCards) {
			prestiegeBtn.setPositon(glm::vec2(window.getWidth() / 2 - 120, 200));
			prestiegeBtn.update(window.getSize());
		}
		//Setup upgrade cards
		if(showUpgradeCards) {
			upgradeBtn1.setSize(glm::vec2(window.getWidth() * 0.2f, window.getHeight() * 0.35f));
			upgradeBtn2.setSize(glm::vec2(window.getWidth() * 0.2f, window.getHeight() * 0.35f));
			upgradeBtn1.setPositon(glm::vec2(70, upgradeBtn1.getSize().y + 32));
			upgradeBtn2.setPositon(glm::vec2(window.getWidth() - upgradeBtn2.getSize().x - 70, upgradeBtn2.getSize().y + 32));
			upgradeHovered=-1;
			upgradeBtn1.update(window.getSize());
			upgradeBtn2.update(window.getSize());
		}
		//Setup general UI
		resetBtn.setPositon(glm::vec2(10, 60));
		resetBtn.update(window.getSize());
		if(!bg.isPlaying()) muteBtn.update(window.getSize());
		else unmuteBtn.update(window.getSize());
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
	// Draw User Interface(wow!)
	void drawUI(bool tButtonVisible) {
		//Draw counter
		float themeValue=0;
		if((Color.r+Color.g+Color.b)<1.5f) themeValue=1;
		if(oldThemeVal!=themeValue) {
			oldThemeVal=themeValue;
			resetBtn.background=muteBtn.background=unmuteBtn.background=glm::vec4(glm::vec3(themeValue), 0.75f);
			resetBtn.hover=muteBtn.hover=unmuteBtn.hover=glm::vec4(glm::vec3(themeValue), 1);
		}
		counter.draw(&text, std::to_string(save.points), window.getSize(),
			glm::vec2((window.getWidth() / 2) - 16 *(std::to_string(save.points).length()), window.getHeight() - 75), glm::vec2(1),
			glm::vec4(glm::vec3(themeValue),1));
		//Draw general UI
		text.enable();
		text.setBool("colorToSampledAlpha", true);
		resetBtn.draw(&text, window.getSize());
		if(!bg.isPlaying()) muteBtn.draw(&text, window.getSize());
		else unmuteBtn.draw(&text, window.getSize());
		text.enable();
		text.setBool("colorToSampledAlpha", false);
		//Draw prestiege button
		if(tButtonVisible && !showUpgradeCards) {
			//TODO: Add TextButton to `fs.ui`.
			if((upgradeCount!=obtainedUpgradeCount)||(save.points>=77777&&save.prestiege>2))
				prestiegeBtn.draw(&text, window.getSize());
			if((save.points<77777||save.prestiege>2)&&upgradeCount!=obtainedUpgradeCount)
				counter.draw(&text, "Prestiege", window.getSize(),
					glm::vec3(window.getWidth() / 2 - 100, 185, 1), glm::vec2(1), glm::vec4(1));
			else if(save.points>=77777)
				counter.draw(&text, "Ascend", window.getSize(),
					glm::vec3(window.getWidth() / 2 - 67, 185, 1), glm::vec2(1), glm::vec4(1));
		}
		//Draw upgrade cards
		if(showUpgradeCards) {
			if(!randomizedCards) {
				upgradeSfxs.init("res/audio/card_shuffle.mp3", 0.8f)->play();
				if(randomizedCards!=upgradeCount) randomizeUpgrades();
				else upgradeBtn1.onClick();
				upgradeBtn1.background=glm::vec4(upgradeColor[upgrade1],1);
				upgradeBtn2.background=glm::vec4(upgradeColor[upgrade2],1);
				upgradeBtn1.hover=glm::vec4(upgradeColor[upgrade1],0.75f);
				upgradeBtn2.hover=glm::vec4(upgradeColor[upgrade2],0.75f);
				randomizedCards=true;
			}
			upgradeBtn1.draw(&text, window.getSize());
			upgradeBtn2.draw(&text, window.getSize());
			if(upgradeHovered!=-1){
				const char* desc=upgradeDescription[upgradeHovered];
				counter.draw(&text, desc, window.getSize(),
					glm::vec3((window.getWidth() / 2) - 8 *(std::string(desc).length()), 30, 1), glm::vec2(0.5f), glm::vec4(glm::vec3(themeValue),1));
			}
		}
	}

	// Runs after window and renderer initialization.
	virtual void onInitialize() override {
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
			handleBG();
			//Check if prestiege button should be visible
			bool buttonVisible =
				(save.prestiege == 0 && save.points >= 3000) ||
				(save.prestiege == 1 && save.points >= 10000) ||
				(save.prestiege == 2 && save.points >= 12000);
			//Handle updates
			if(save.betterHoldToClick()) holdClickCooldown.limit=0.3;
			updateUI(buttonVisible);
			controlledMovement();
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
