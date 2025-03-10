#include "renderer.h"

void SetupImGUI(Window* window) {
	// Setup ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window->GetWindowPTR(), true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
  ImGui_ImplOpenGL3_Init();
}

Renderer::Renderer(Window* window, Scene* scene) {
	SetupImGUI(window);

  window_ = window;
  scene_ = scene;
  // OPENGL PARAMETRS
  // =------------------------------------------------------=
  // Set OPENGL parametrs
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_STENCIL_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // Gamma correction
  glEnable(GL_FRAMEBUFFER_SRGB);

  model_shader_ = new Shader("shader.vert", "shader.frag");
  light_shader_ = new Shader("lightsource.vert", "lightsource.frag");
  single_color_ = new Shader("shader.vert", "single_color.frag");
  skybox_shader_ = new Shader("skybox.vert", "skybox.frag");
  DLdepth_shader_ = new Shader("depthShader.vert", "DLightDepthShader.frag", "DLightDepthShader.geom");
  PLdepth_shader_ = new Shader("PlightDepthShader.vert", "PLightDepthShader.frag", "PLightDepthShader.geom");

  //GLuint cubeMapArray;
  glGenTextures(1, &cubeMapArray);
  glBindTexture(GL_TEXTURE_2D_ARRAY, cubeMapArray);
  glTexImage3D(
    GL_TEXTURE_2D_ARRAY,
    0,
    GL_DEPTH_COMPONENT16,
    scene_->properties.PLShadowResolution,
    scene_->properties.PLShadowResolution,
    20 * 6,
    0,
    GL_DEPTH_COMPONENT,
    GL_FLOAT,
    nullptr);

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

  //GLuint depthMapFBO;
  glGenFramebuffers(1, &depthMapFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubeMapArray, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!";
    throw 0;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  //// Create a cube map array
	//GLuint cubeMapArray;
	//glGenTextures(1, &cubeMapArray);
	//GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, cubeMapArray));

	//// Allocate storage for the cube map array
	//GL_CHECK(glTexStorage3D(
	//    GL_TEXTURE_CUBE_MAP_ARRAY,
	//    1,                         
	//    GL_DEPTH_COMPONENT16,   
	//    scene.properties.PLShadowResolution, scene.properties.PLShadowResolution,
	//    20 * 6       
	//));

	//// Set texture parameters
	//glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	//GLuint depthMapFBO;
	//glGenFramebuffers(1, &depthMapFBO);
	//glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

	//glDrawBuffer(GL_NONE); // No color buffer is drawn to
	//glReadBuffer(GL_NONE); // No color buffer is read from

	//glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind the FBO

	//// Bind the cube map array to a texture unit
	//glActiveTexture(GL_TEXTURE4);
	//GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, cubeMapArray));

  unsigned int shader_ub = glGetUniformBlockIndex(model_shader_->ID, "Matrices");
  unsigned int light_shader_ub = glGetUniformBlockIndex(light_shader_->ID, "Matrices");
  unsigned int single_color_ub = glGetUniformBlockIndex(single_color_->ID, "Matrices");
  unsigned int skyboxShader_ub = glGetUniformBlockIndex(skybox_shader_->ID, "Matrices");

  glUniformBlockBinding(model_shader_->ID, shader_ub, 0);
  glUniformBlockBinding(light_shader_->ID, light_shader_ub, 0);
  glUniformBlockBinding(single_color_->ID, single_color_ub, 0);
  glUniformBlockBinding(skybox_shader_->ID, skyboxShader_ub, 0);

  //unsigned int uboMatrices;
  glGenBuffers(1, &uboMatrices);
  glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
  glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
  glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));

  model_shader_->use();
  model_shader_->setInt("PLshadowMapArray", 4); // Use the same texture unit
  skybox_shader_->setInt("skybox", 0);
  // Creating framebuffer
  //unsigned int fbo;
  glGenFramebuffers(1, &frame_buffer);
  glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

  // generate texture
  //unsigned int texColorBuffer;
  glGenTextures(1, &texColorBuffer);
  glBindTexture(GL_TEXTURE_2D, texColorBuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window->GetScreenWidth(), window->GetScreenHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  glBindTexture(GL_TEXTURE_2D, 0);
  // attach it to currently bound framebuffer object
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);

  // Creating renderbuffer
  unsigned int rbo;
  glGenRenderbuffers(1, &rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window->GetScreenWidth(), window->GetScreenHeight());
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" <<
    std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  float quadVertices[] = {
    // positions // texCoords
    -1.0f, 1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
    1.0f, -1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 1.0f
  };

  quadShader = new Shader("quad.vert", "quad.frag");

  //unsigned int fullquadVAO, fullquadVBO;
  glGenVertexArrays(1, &fullquadVAO);
  glGenBuffers(1, &fullquadVBO);
  glBindVertexArray(fullquadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, fullquadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

  // Position Attribute (x, y)
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // Texture Coordinate Attribute (u, v)
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);


  model_shader_->use();
  model_shader_->setInt("DLshadowMap", 3);
  //shader.setInt("PLshadowMap", 4);
  // DEBUG QUAD SETUP
  // =----------------------=
  Shader debugDepthQuad("quad.vert", "debug_quad.frag");
  debugDepthQuad.use();
  debugDepthQuad.setInt("depthMap", 0);
}

void Renderer::RenderScene(bool render_imgui) {
	// Gamma correction
	glEnable(GL_FRAMEBUFFER_SRGB);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glViewport(0, 0, window_->GetScreenWidth(), window_->GetScreenHeight());
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

	glEnable(GL_DEPTH_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glEnable(GL_CULL_FACE);

	glStencilFunc(GL_ALWAYS, 1, 0xFF); // all fragments pass the stencil test
	glStencilMask(0xFF); // enable writing to the stencil buffer

	//if (scene_->getObjects().empty() || active_camera_index >= objects.size()) return;

	unsigned int number_p_lights = 0;

	// Update light !!! WILL BE REMOVED WHEN LIGHTS ARE DONE !!!
	for (const auto& obj : scene_->getObjects()) {
		if (auto* light = dynamic_cast<Light*>(obj)) {
			light->update(model_shader_, 0);
		}
	}

	// CAMERA
	//=----------------------------------------------------=

	Camera* activeCamera = scene_->GetCamera();
	activeCamera->uniform_buffer = uboMatrices;

	//// Find the active camera
	//for (auto obj : objects) {
	//	if (auto cam = dynamic_cast<Camera*>(obj)) {
	//		activeCamera = cam;
	//		break;  // Assuming only one active camera
	//	}
	//}

	activeCamera->update_shaders(model_shader_);
	activeCamera->update_shaders(single_color_);
	activeCamera->update_shaders(DLdepth_shader_);
	activeCamera->update_shaders(skybox_shader_);


	// DIRECTIONAL LIGHT SHADOWS
	// =--------------------------------------------------=

	DirectionalLight* sun = nullptr;

	// Find the directional light (sun)
	for (auto obj : scene_->getObjects()) {
		if (auto dirLight = dynamic_cast<DirectionalLight*>(obj)) {
			sun = dirLight;
			break;  // Assuming only one directional light
		}
	}

	sun->setCamera(activeCamera);
	if (!activeCamera || !sun) return;

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_ARRAY, sun->depthMap);
	model_shader_->use();
	model_shader_->setInt("DLshadowMap", 3);

	// configure UBO
	// --------------------
	unsigned int matricesUBO;
	glGenBuffers(1, &matricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 5, nullptr, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, matricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	DLdepth_shader_->use();
	// UBO setup
	const auto lightMatrices = sun->getLightSpaceMatrices();
	glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
	for (size_t i = 0; i < lightMatrices.size(); ++i)
	{
		glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4x4), sizeof(glm::mat4x4), &lightMatrices[i]);
	}
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, sun->depthMapFBO);
	glViewport(0, 0, sun->SHADOW_WIDTH, sun->SHADOW_HEIGHT);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);  // peter panning

	for (const auto& obj : scene_->getObjects()) {
		if (auto model = dynamic_cast<Model*>(obj)) {
			model->Draw(DLdepth_shader_);
		}
	}
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	model_shader_->use();
	sun->update(model_shader_, 0);
	// Setting up directional light cascades
	model_shader_->setFloat("DLfarPlane", activeCamera->getFar());
	model_shader_->setInt("cascadeCount", sun->shadowCascadeLevels.size());
	for (size_t i = 0; i < sun->shadowCascadeLevels.size(); ++i)
	{
		model_shader_->setFloat("cascadePlaneDistances[" + std::to_string(i) + "]", sun->shadowCascadeLevels[i]);
	}

	//// POINT LIGHTS SHADOWS
	////=-----------------------------------------------------=
	//unsigned int PLmatricesUBO;
	//glGenBuffers(1, &PLmatricesUBO);
	//glBindBuffer(GL_UNIFORM_BUFFER, PLmatricesUBO);
	//glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 6, nullptr, GL_STATIC_DRAW);
	//glBindBufferBase(GL_UNIFORM_BUFFER, 2, PLmatricesUBO);
	//glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO); // Use the global FBO
	//// Render the scene to the cube map array layer
	//glViewport(0, 0, scene_->properties.PLShadowResolution, scene_->properties.PLShadowResolution); // Use global shadow map dimensions
	////glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubeMapArray, 0);
	//glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	//glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubeMapArray, 0);

	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);

	////glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubeMapArray, number_p_lights*6);

	//for (auto obj : scene_->getObjects()) {
	//	if (auto pointLight = dynamic_cast<PointLight*>(obj)) {
	//		pointLight->update(model_shader_, number_p_lights);

	//		model_shader_->use();
	//		model_shader_->setFloat("pointLights[" + std::to_string(number_p_lights) + "].PLfarPlane", pointLight->far_plane);
	//		PLdepth_shader_->use();
	//		PLdepth_shader_->setFloat("far_plane", pointLight->far_plane);
	//		PLdepth_shader_->setVec3("lightPos", pointLight->getPosition());
	//		PLdepth_shader_->setInt("lightIndex", number_p_lights);

	//		// UBO setup
	//		// Set up the light space matrices

	//		std::vector<glm::mat4> lsMatrices = pointLight->getLightSpaceMatrix(scene_->properties.PLShadowResolution);
	//		glBindBuffer(GL_UNIFORM_BUFFER, PLmatricesUBO);
	//		for (size_t i = 0; i < lsMatrices.size(); ++i)
	//		{
	//			glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4x4), sizeof(glm::mat4x4), &lsMatrices[i]);
	//		}
	//		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	//		//// Render to each face of the cube map
	//		//for (unsigned int face = 0; face < 6; ++face) {
	//		//    // Bind the cube map array layer for this face

	//		//    glFramebufferTextureLayer(
	//		//        GL_FRAMEBUFFER,              // Target
	//		//        GL_DEPTH_ATTACHMENT,         // Attachment
	//		//        cubeMapArray,                // Texture
	//		//        0,                           // Mipmap level
	//		//        (number_p_lights * 6) + face // Layer (6 faces per cube map)
	//		//    );

	//		//    // Check if the framebuffer is complete
	//		//    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
	//		//        std::cerr << "Framebuffer is not complete!" << std::endl;
	//		//    }

	//		//    // Set the light space matrix for this face
	//		//    PLdepth_shader->setMat4("lightSpaceMatrix", lsMatrices[face]);

	//		glClear(GL_DEPTH_BUFFER_BIT);

	//		for (const auto& obj : scene_->getObjects()) {
	//			if (auto model = dynamic_cast<Model*>(obj)) {
	//				model->DrawDepth(PLdepth_shader_);
	//			}
	//		}
	//		//}

	//		// Increment the point light counter
	//		number_p_lights++;
	//	}
	//}
	//// Unbind the framebuffer
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//// Set the uniform for the cube map array
	//model_shader_->use();
	//model_shader_->setInt("PLshadowMapArray", 4); // Use the same texture unit

	// MAIN RENDER
	//=------------------------------------------------------=

	model_shader_->setInt("numPointLights", number_p_lights);
	model_shader_->setInt("numSpotLights", 1);

	// Restore viewport for main rendering
	glViewport(0, 0, window_->GetScreenWidth(), window_->GetScreenHeight());
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


	// Render not selected objects without writing to stencil buffer
	for (const auto& obj : scene_->getObjects()) {
		if (auto model = dynamic_cast<Model*>(obj)) {
			if (!model->getSelection()) {
				glStencilMask(0x00);
				model->Draw(model_shader_);
			}
		}
	}

	// Render selected
	for (const auto& obj : scene_->getObjects()) {
		if (auto model = dynamic_cast<Model*>(obj)) {
			if (model->getSelection()) {
				glStencilFunc(GL_ALWAYS, 1, 0xFF);
				glStencilMask(0xFF);
				model->Draw(model_shader_);
			}
		}
	}

	// Render selected object with solid color shader
	for (const auto& obj : scene_->getObjects()) {
		if (auto model = dynamic_cast<Model*>(obj)) {
			if (model->getSelection()) {
				glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
				glStencilMask(0x00);
				glDisable(GL_DEPTH_TEST);
				model->DrawStencil(single_color_);
			}
		}
	}

	// Render skybox last
	glStencilMask(0x00);
	if (scene_->GetSkybox()) scene_->GetSkybox()->Draw(skybox_shader_, activeCamera);

	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	quadShader->use();
	glBindVertexArray(fullquadVAO);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, texColorBuffer);
	glDrawArrays(GL_TRIANGLES, 0, 6);

  // DeltaTime calculation
  float currentFrame = glfwGetTime();
  deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;

  // Measure framerate
  double currentTime = glfwGetTime();
  frameCount++;
  // If a second has passed.
  if (currentTime - previousTime >= 1.0) {
    // Display the frame count here any way you want
    fps = frameCount;
    frameCount = 0;
    previousTime = currentTime;
  }

	// render ImGui
  if (render_imgui) {
    // Gamma correction
		glDisable(GL_FRAMEBUFFER_SRGB);
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    //ImGui::ShowDemoWindow(); // Show demo window! :)
    ShowMyWindow(scene_, fps);
    // Render ImGUI ontop
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }

  // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
  // -------------------------------------------------------------------------------
  glfwSwapBuffers(window_->GetWindowPTR());
  glfwPollEvents();
}