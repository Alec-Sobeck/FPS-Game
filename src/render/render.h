#ifndef ENGINE_RENDER_H
#define ENGINE_RENDER_H

#include <map>
#include <glbinding/gl/gl.h>
#include "render/texture.h"
#include "graphics/camera.h"
#include "graphics/gluhelper.h"
#include "utils/textureloader.h"
#include "physics/aabb.h"

void drawSkybox(std::shared_ptr<Texture> skybox, Camera *cam);
void renderAxes(Camera *cam);
void drawAABB(AABB &box);

#endif
