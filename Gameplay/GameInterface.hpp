#pragma once

#include <vector>
#include <algorithm>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/random.hpp"
#include "TaskManagerHelpers.hpp"
#include "TaskManager.hpp"

#define THREADS 3

namespace Game
{

	struct InputData
	{
		double dt;
		float horizontal, vertical;
		glm::vec2 screenSize;
		bool shoot;
		bool clicking, clickDown, clickingRight, clickDownRight;
		glm::vec2 mousePosition;
	};

	struct RenderData
	{
		enum class TextureNames
		{
			LENA,
			CHECKERS,
			IMGUI,
			COUNT,
			CIRCLE
		};

		struct Sprite
		{
			glm::dvec2 position, size;
			double angle;
			TextureNames texture;
			glm::vec4 color;
		};

		std::vector<Sprite> sprites;
	};

	struct Ball {
		Ball() {}
		~Ball() {  }
		glm::dvec2 pos, vel, acceleration, force, nextVel;
		glm::vec4 color;
		double radius, mass;
		
		glm::dvec2 GetExtreme(glm::dvec2 dir) const { return pos + dir*radius; }
	};

	struct GameData;
	
	GameData* InitGamedata(glm::vec2 screenSize);

	RenderData Update(GameData &gameData, const InputData& input, Utilities::Profiler& profiler,Utilities::TaskManager::JobContext & context);

	void FinalizeGameData(GameData* gameData);
}

namespace Physics {
	struct ContactData;
	struct ContactGroup;
	std::vector<ContactGroup> GenerateContactGroups(std::vector<ContactData> contactData);
	void GUI();
	void CalculateSeparationForce(Game::Ball &b1, Game::Ball &b2, ContactData contactData);
	void CalculateSeparationForce(ContactData * contactData);
}

namespace Collision {
	
	std::vector<Game::Ball*> doPhysics(std::vector<Game::Ball*>& balls, double dt, glm::dvec2 screenSize, Utilities::Profiler& profiler, Utilities::TaskManager::JobContext & context);
	void doPhysics(Game::Ball* hero, glm::vec2 screenSize);
	bool BallAndBall(Game::Ball &b1, Game::Ball &b2);


}
