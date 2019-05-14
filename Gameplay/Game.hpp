#pragma once

namespace Game {

	struct GameData {
		static const int totalBalls = 3800;
		std::vector<Ball*> balls;
		Ball player;
		double force, angle;
	};
}