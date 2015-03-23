#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <memory>
#include "glm/vec3.hpp"
#include "physics/aabb.h"
#include "physics/aabs.h"
#include "graphics/camera.h"
#include "graphics/model.h"
#include "render/sphere.h"
#include "entity.h"

/**
 * Entity is the base class for all things that exist in the world with some sort of model, and position.
 * @author Alec Sobeck
 * @author Matthew Robertson
 */
class Projectile
{
public:
	int size;
	AABS boundingSphere;
    /** An integer which uniquely identifies this entity in the world. Generated by {@link #getNextEntityID} */
	int entityID;

    /**
     * Constructs a new Entity, assigning it the next valid entityID available and a camera with position
     * and rotation of 0.
     */
	Projectile();
    /**
     * Constructs a new Entity, assigning it the entityID provided and a camera with position
     * and rotation of 0.
     * @param entityID an int which must uniquely identify this Entity. It is suggested that this
     * be a value generated from {@link #getNextEntityID()}
     */
	Projectile(int entityID);
	/**
	 * Creates a new Entity and assigns it the provided entityID, model, and camera.
     * @param entityID an int which must uniquely identify this Entity. It is suggested that this
     * be a value generated from {@link #getNextEntityID()}
	 * @param model a Model that will be used for this entity
	 * @param camera a Camera that will be used for this entity
	 */
	Projectile(int entityID, Camera camera, float size = 0);
	Projectile(Camera camera, float size = 0);
	~Projectile();
    float getX();
	float getY();
	float getZ();
	void draw();
	bool affectedByGravity();
	void setAffectedByGravity(bool isAffectedByGravity);
	int getEntityID();
	AABB getAABB();
    Camera *getCamera();
	void setCamera(Camera camera);
	/**
	 * Moves the Camera the specified amount.
	 * @param movement a glm::vec3 that describes the movement of the Camera
	 */
	void move(float deltaTime);
    void accel(glm::vec3 movement);
	void rotate(glm::vec3 rotation);
    glm::vec3 getRotation();
    glm::vec3 getPosition();
	void onGameTick(float deltaTime);
	float getHealthPercent();

	glm::vec3 velocity;
	glm::vec3 acceleration;
	double maxMoveSpeed;
protected:
	Sphere sphere;
    /** A camera which controls the movement of this entity. */
    Camera camera;
private:
	/** True if the entity should follow the laws of gravity, or false if gravity ignores them. */
	bool isAffectedByGravity;
};

inline bool operator==(const Projectile& one, const Projectile& two)
{
	return false;
}

#endif
