#include <iostream>

#include "GameInterface.hpp"
#include "Game.hpp"

namespace Game {
	
	GameData* InitGamedata(glm::vec2 screenSize)
	{
		float radius = 4.0f;
		GameData* gameData = new GameData();
		gameData->balls.reserve(gameData->totalBalls + 1);
		for (int i = 0; i < gameData->totalBalls; i++) {
			Ball* ball = new Ball();
			ball->radius = radius;
			ball->pos = { glm::linearRand(-screenSize.x + ball->radius , screenSize.x - ball->radius ),
				glm::linearRand(-screenSize.y + ball->radius, screenSize.y - ball->radius + ball->radius) };
			ball->vel = glm::normalize(glm::linearRand(glm::vec2(-1.0), glm::vec2(1.0))) * 100.0f;
			ball->mass = glm::linearRand(5.0f, 15.0f);
			switch ((int)ball->mass) {
			case 5: case 6:case 7:
				ball->color = { 0.0, 0.0, 1.0, 1.0 };
				break;
			case 8: case 9:case 10:
				ball->color = { 0.0, 1.0, 0.0, 1.0 };
				break;
			case 11: case 12:case 13: case 14:
				ball->color = { 1.0, 0.0, 0.0, 1.0 };
				break;
			}
			gameData->balls.push_back(ball);
		}

		// Player
		gameData->player.radius = radius*3.0;
		gameData->player.mass = 120.0f;
		gameData->player.pos = { 0.0f, 0.0f };
		gameData->player.vel = { 20.0f, 20.0f };
		gameData->player.color = { 1.0f, 1.0f, 0.0f, 1.0f };
		gameData->balls.push_back(&gameData->player);

		gameData->force = 1200.f;
		return gameData;
	}

	void FinalizeGameData(GameData* gameData)
	{
		delete gameData;
	}

	Game::RenderData Update(GameData &gameData, const Game::InputData& input, Utilities::Profiler& profiler, Utilities::TaskManager::JobContext & context)
	{
		std::vector<Ball*> prevBalls;
		{
			
			auto guard = context.profiler->CreateProfileMarkGuard("Input Handler");
			// Update input
			gameData.force += input.vertical;
			if (gameData.force > 5550.0) gameData.force = 5550.0f;
			if (gameData.force < 0.0) gameData.force = 0.0f;

			// update
			prevBalls = std::move(gameData.balls);
			gameData.balls.clear();

			gameData.angle += input.horizontal;
			if (input.shoot) {
				gameData.force = 10000;
				glm::dvec2 vec(glm::cos(gameData.angle), glm::sin(gameData.angle));

				gameData.player.force = gameData.force * glm::normalize(glm::dvec2(glm::cos(glm::radians(gameData.angle)), glm::sin(glm::radians(gameData.angle))));
				//gameData.player.vel = gameData.force * glm::normalize(glm::dvec2(glm::cos(glm::radians(gameData.angle)), glm::sin(glm::radians(gameData.angle))));
			}
			else
			{
				gameData.player.force = glm::vec2(0);
				gameData.force = 0;
			}
		}
			// Physics
		gameData.balls = Collision::doPhysics(prevBalls, input.dt, input.screenSize, profiler,context);

		//auto guard2 = profiler.CreateProfileMarkGuard("Prepare render");
		Game::RenderData renderData = {};
		Game::RenderData::Sprite sprite = {};
		for (Game::Ball* ball : gameData.balls) {
			sprite.position = ball->pos;
			sprite.size = glm::dvec2(ball->radius);
			sprite.color = ball->color;
			sprite.texture = RenderData::TextureNames::CIRCLE;
			renderData.sprites.push_back(sprite);
		}

		// arrow mode
		sprite = {};
		sprite.size = {100.0f, 30.0 };
		sprite.angle = glm::radians(gameData.angle);
		sprite.position = gameData.player.pos;
		sprite.color = { -1.0, 0.0, 0.0, 0.0 };
		sprite.texture = RenderData::TextureNames::LENA;
		renderData.sprites.push_back(sprite);
		return renderData;
	}
}