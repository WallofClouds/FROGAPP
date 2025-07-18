#include <glm/vec3.hpp>
#include "../engine/include/entity.hpp"
#include "../engine/include/utils/utils.hpp"

int state;
bool sizeState = false;
float sizeAccel, impact;

// Have fun with background
void handleBG(glm::vec3&Color, bool isBgRainbow, unsigned int points, unsigned int prestiege) {
	switch (prestiege) {
	case 0:
		if (!isBgRainbow && points >= 2500) {
			isBgRainbow = true;
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
		break;
	case 1:
		if (!isBgRainbow && points >= 2500) {
			isBgRainbow = true;
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
		break;
	default:
		if (!isBgRainbow && points >= 2500) {
			isBgRainbow = true;
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
		break;
	}
}
// Move and rotate the frog.
void controlledMovement(double deltaTime, unsigned int points, Firesteel::Entity* displayFrog, glm::vec3 Size) {
	//Control size
	if (Size.y > 7 && !sizeState) {
		sizeState = true;
		sizeAccel = 0.00001f;
	}
	else if (Size.y <= 1 && sizeState) {
		sizeState = false;
		sizeAccel = 0.00001f;
	}
	//Determine if frog should expand or shrink
	if (!sizeState) Size += glm::vec3(sizeAccel * deltaTime);
	else Size -= glm::vec3(sizeAccel * deltaTime);
	//Update knwon acceleration
	sizeAccel += 0.00001f;
	if (impact > 0) impact -= 0.001f;
	//Apply new knowledge
	displayFrog->transform.size = glm::vec3(3 + std::clamp(points / 10000, 0U, 5U), 1, 1.2f) + Size + glm::vec3(impact);
	displayFrog->transform.rotation += glm::vec3(0, 0.02f, 0.02f);
}