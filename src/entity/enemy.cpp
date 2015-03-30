﻿
#include "enemy.h"

/**
* Creates a new Entity and assigns it the provided entityID, model, and camera.
* @param entityID an int which must uniquely identify this Entity. It is suggested that this
* be a value generated from {@link #getNextEntityID()}
* @param model a Model that will be used for this entity
* @param camera a Camera that will be used for this entity
*/
Enemy::Enemy(std::shared_ptr<Model> model, Camera camera) : Entity(model, camera)
{
	if (model)
	{
		this->boundingBox = model->getAABB();
	}
	else
	{
		this->boundingBox = AABB(camera.getPosition(), glm::vec3(2.5, 5, 2.5));
	}
}

Enemy::~Enemy()
{

}

void Enemy::onGameTick(Player &player, float deltaTime, AABB &worldBounds)
{
	// Figure out where the entity is relative to the player
	glm::vec3 toPlayer = (player.getPosition() - getPosition());
	toPlayer.y = 0;
	float distanceSquared = glm::dot(toPlayer, toPlayer);
	toPlayer = glm::normalize(toPlayer); 
	
	if (state == AIState::IDLE)
	{
		if (distanceSquared < 400)
		{
			state = AIState::ATTACK;
		}
		// Otherwise, idle and do nothing.
	}
	else if (state == AIState::ATTACK)
	{
		if (distanceSquared > 400 && distanceSquared < 35 * 35)
		{
			state = AIState::LOSING_SIGHT;
		}
		// Else chase and attack
		float movementSpeed = 3.0f * deltaTime;
		this->accel(toPlayer * movementSpeed);
	}
	else if (state == AIState::LOSING_SIGHT)
	{
		if (distanceSquared > 35 * 35)
		{
			state = AIState::IDLE;
		}
		if (distanceSquared < 400)
		{
			state = AIState::ATTACK;
		}
		// Persue at this distance if previously chasing the player. otherwise, ignore them.
		float movementSpeed = 3.0f * deltaTime;
		this->accel(toPlayer * movementSpeed);
	}

	// Update the position
	this->move();
	this->boundsCheckPosition(worldBounds);
	auto pos = getPosition();
	boundingBox.moveTo(pos.x, pos.y, pos.z);
}

void Enemy::draw()
{

}