#include "GameInterface.hpp"

namespace Collision {

	// out: return direction if collided
	glm::vec2 SphereAndBox(const glm::vec2 sPos, const glm::vec2 sSize, const glm::vec2 bPos, const glm::vec2 bSize) {
		float distSquared = sSize.x * sSize.x;
		float closestX = sPos.x;
		float closestY = sPos.y;

		if (closestX < (bPos.x - bSize.x / 2.0f)) {
			closestX = bPos.x - bSize.x / 2.0f;
		}
		else if (closestX >(bPos.x + bSize.x / 2.0f)) {
			closestX = bPos.x + bSize.x / 2.0f;
		}

		if (closestY < (bPos.y - bSize.y / 2.0f)) {
			closestY = bPos.y - bSize.y / 2.0f;
		}
		else if (closestY >(bPos.y + bSize.y / 2.0f)) {
			closestY = bPos.y + bSize.y / 2.0f;
		}
		closestX -= sPos.x;
		closestX *= closestX;
		closestY -= sPos.y;
		closestY *= closestY;
		if (closestX + closestY < distSquared) {
			if (closestX < closestY) {
				return { 1.0f, -1.0f };
			}
			else {
				return { -1.0f, 1.0f };
			}
		}
		return glm::vec2();
	}

}