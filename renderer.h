#ifndef RENDERER_H_
#define RENDERER_H_

// Graphics libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Imgui lib
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

// Custom includes
#include "scene.h"
#include "stb_image.h"
#include <SHADER/shader_c.h>

#include "window.h"
#include "mine_imgui.h"

// standart libraries
#include <iostream>
#include <string.h>
#include <vector>


class Renderer {

public:
  std::vector<Shader*> shaders;
  Scene* scene_;
	Shader* model_shader_;
	Shader* light_shader_;
	Shader* single_color_;
	Shader* skybox_shader_;
	Shader* DLdepth_shader_;
	Shader* PLdepth_shader_;
	Shader* quadShader;
	unsigned int fullquadVAO, fullquadVBO;
	unsigned int texColorBuffer;

	float deltaTime = 0.0f; // Time between current frame and last frame
	float lastFrame = 0.0f; // Time of last frame  
	// SOME PARAMETRS
  double previousTime = glfwGetTime();
  int frameCount = 0;

	int fps = 0;
	Window* window_;

	GLuint depthMapFBO;
	GLuint cubeMapArray;
	GLuint frame_buffer;
	GLuint uboMatrices;

	Renderer(Window* window, Scene* scene);

  void RenderScene(bool render_imgui);

	inline void Terminate() {

		//clear ImGUI
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		// glfw: terminate, clearing all previously allocated GLFW resources.
		// ------------------------------------------------------------------
		glfwTerminate();
	}
};


#endif