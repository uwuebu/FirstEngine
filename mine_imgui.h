#pragma once
#ifndef MINEIMGUI
#define MINEIMGUI

#include "imgui.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "scene.h"
#include "object.h"
#include "model.h"
#include <custom/camera.h>
#include "light.h"

extern bool showPerformanceCounter; // Toggle state
extern unsigned int fps_c;

static void ShowinfoOverlay();

void RenderMenuBar();

void ShowMyWindow(Scene* scene, unsigned int fps_count);

static void ShowinfoOverlay();

#endif