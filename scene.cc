#include "scene.h"

template<typename T, typename... Args>
T* Scene::AddObject(Args&&... args) {
	T* obj = new T(std::forward<Args>(args)...);
	obj->SetID(nextID++);
	objects.push_back(obj);
	objectLookup[obj->GetID()] = obj;
	return obj;
}
Model* Scene::AddModel(Model* model, std::string name) {
	objects.push_back(model);
	model->name = "Model" + std::to_string(count_models);
	count_models++;
	return model;
}
Model* Scene::AddModel(char* path, std::string name) {
	Model* obj = AddObject<Model>(path);
	obj->name = name + std::to_string(count_models);
	count_models++;
	return obj;
}

PointLight* Scene::AddPointLight(Camera* camera, glm::vec3 position, 
																 std::string name) {
	PointLight* obj = AddObject<PointLight>(camera, position);
	obj->name = name + std::to_string(count_lights);
	count_lights++;
	return obj;
}
DirectionalLight* Scene::AddDirectionalLight(Camera* camera, 
																						 const glm::vec3& direction, 
																						 std::string name) {
	DirectionalLight* obj = AddObject<DirectionalLight>(camera, direction);
	obj->name = name + std::to_string(count_lights);
	count_lights++;
	return obj;
}
SpotLight* Scene::AddSpotLight(std::string name) {
	SpotLight* obj = AddObject<SpotLight>();
	obj->name = name + std::to_string(count_lights);
	count_lights++;
	return obj;
}
AmbientLight* Scene::AddAmbientLight(const glm::vec3& color, 
																		 std::string name) {
	AmbientLight* obj = AddObject<AmbientLight>(color);
	obj->name = name + std::to_string(count_lights);
	count_lights++;
	return obj;
}
Camera* Scene::AddCamera(const glm::vec3& position, std::string name) {
	if (camera) delete camera;
	camera = new Camera(position);
	camera->SetID(nextID++);
	objectLookup[camera->GetID()] = camera;
	camera->name = name;
	return camera;
}

Skybox* Scene::AddSkybox() {
	if (skybox) delete skybox;
	skybox = new Skybox();
	skybox->SetID(nextID++);
	objectLookup[skybox->GetID()] = skybox;
	return skybox;
}

// Delete an object by ID
void Scene::Delete(unsigned int id) {
	if (objectLookup.find(id) == objectLookup.end()) return;

	Object* obj = objectLookup[id];
	objectLookup.erase(id);

	objects.erase(std::remove(objects.begin(), objects.end(), obj),
														objects.end());
	delete obj;
}

void Scene::Clear() {
	for (auto obj : objects) delete obj;
	objects.clear();
	objectLookup.clear();
	if (skybox) delete skybox;
	skybox = nullptr;
	nextID = 1;
}

void Scene::Draw(unsigned int active_camera_index, unsigned int frame_buffer, unsigned int screenWidth, unsigned int screenHeight, unsigned int cubeMapArray, unsigned int depthMapFBO) {
	if (objects.empty() || active_camera_index >= objects.size()) return;

	unsigned int number_p_lights = 0;

	// Update light !!! WILL BE REMOVED WHEN LIGHTS ARE DONE !!!
	for (const auto& obj : objects) {
		if (auto* light = dynamic_cast<Light*>(obj)) {
			light->update(model_shader, 0);
		}
	}

	// CAMERA
	//=----------------------------------------------------=

	Camera* activeCamera = camera;

	//// Find the active camera
	//for (auto obj : objects) {
	//	if (auto cam = dynamic_cast<Camera*>(obj)) {
	//		activeCamera = cam;
	//		break;  // Assuming only one active camera
	//	}
	//}
	activeCamera->update_shaders(model_shader);
	activeCamera->update_shaders(outline_shader);
	activeCamera->update_shaders(DLdepth_shader);
	activeCamera->update_shaders(skybox_shader);

	// DIRECTIONAL LIGHT SHADOWS
	// =--------------------------------------------------=

	DirectionalLight* sun = nullptr;

	// Find the directional light (sun)
	for (auto obj : objects) {
		if (auto dirLight = dynamic_cast<DirectionalLight*>(obj)) {
			sun = dirLight;
			break;  // Assuming only one directional light
		}
	}

	sun->setCamera(activeCamera);
	if (!activeCamera || !sun) return;

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_ARRAY, sun->depthMap);
	model_shader->use();
	model_shader->setInt("DLshadowMap", 3);

	// configure UBO
	// --------------------
	unsigned int matricesUBO;
	glGenBuffers(1, &matricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 5, nullptr, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, matricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	DLdepth_shader->use();
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

	for (const auto& obj : objects) {
		if (auto model = dynamic_cast<Model*>(obj)) {
			model->Draw(DLdepth_shader);
		}
	}
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	model_shader->use();
	sun->update(model_shader, 0);
	// Setting up directional light cascades
	model_shader->setFloat("DLfarPlane", activeCamera->getFar());
	model_shader->setInt("cascadeCount", sun->shadowCascadeLevels.size());
	for (size_t i = 0; i < sun->shadowCascadeLevels.size(); ++i)
	{
		model_shader->setFloat("cascadePlaneDistances[" + std::to_string(i) + "]", sun->shadowCascadeLevels[i]);
	}

	// POINT LIGHTS SHADOWS
	//=-----------------------------------------------------=
	unsigned int PLmatricesUBO;
	glGenBuffers(1, &PLmatricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, PLmatricesUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 6, nullptr, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 2, PLmatricesUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO); // Use the global FBO
	// Render the scene to the cube map array layer
	glViewport(0, 0, properties.PLShadowResolution, properties.PLShadowResolution); // Use global shadow map dimensions
	//glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubeMapArray, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubeMapArray, 0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	//glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubeMapArray, number_p_lights*6);

	for (auto obj : objects) {
		if (auto pointLight = dynamic_cast<PointLight*>(obj)) {
			pointLight->update(model_shader, number_p_lights);

			model_shader->use();
			model_shader->setFloat("pointLights[" + std::to_string(number_p_lights) + "].PLfarPlane", pointLight->far_plane);
			PLdepth_shader->use();
			PLdepth_shader->setFloat("far_plane", pointLight->far_plane);
			PLdepth_shader->setVec3("lightPos", pointLight->getPosition());
			PLdepth_shader->setInt("lightIndex", number_p_lights);

			// UBO setup
			// Set up the light space matrices

			std::vector<glm::mat4> lsMatrices = pointLight->getLightSpaceMatrix(properties.PLShadowResolution);
			glBindBuffer(GL_UNIFORM_BUFFER, PLmatricesUBO);
			for (size_t i = 0; i < lsMatrices.size(); ++i)
			{
				glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4x4), sizeof(glm::mat4x4), &lsMatrices[i]);
			}
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
			//// Render to each face of the cube map
			//for (unsigned int face = 0; face < 6; ++face) {
			//    // Bind the cube map array layer for this face

			//    glFramebufferTextureLayer(
			//        GL_FRAMEBUFFER,              // Target
			//        GL_DEPTH_ATTACHMENT,         // Attachment
			//        cubeMapArray,                // Texture
			//        0,                           // Mipmap level
			//        (number_p_lights * 6) + face // Layer (6 faces per cube map)
			//    );

			//    // Check if the framebuffer is complete
			//    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			//        std::cerr << "Framebuffer is not complete!" << std::endl;
			//    }

			//    // Set the light space matrix for this face
			//    PLdepth_shader->setMat4("lightSpaceMatrix", lsMatrices[face]);

			glClear(GL_DEPTH_BUFFER_BIT);

			for (const auto& obj : objects) {
				if (auto model = dynamic_cast<Model*>(obj)) {
					model->DrawDepth(PLdepth_shader);
				}
			}
			//}

			// Increment the point light counter
			number_p_lights++;
		}
	}
	// Unbind the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Set the uniform for the cube map array
	model_shader->use();
	model_shader->setInt("PLshadowMapArray", 4); // Use the same texture unit

	// MAIN RENDER
	//=------------------------------------------------------=

	model_shader->setInt("numPointLights", number_p_lights);
	model_shader->setInt("numSpotLights", 1);

	// Restore viewport for main rendering
	glViewport(0, 0, screenWidth, screenHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


	// Render not selected objects without writing to stencil buffer
	for (const auto& obj : objects) {
		if (auto model = dynamic_cast<Model*>(obj)) {
			if (!model->getSelection()) {
				glStencilMask(0x00);
				model->Draw(model_shader);
			}
		}
	}

	// Render selected
	for (const auto& obj : objects) {
		if (auto model = dynamic_cast<Model*>(obj)) {
			if (model->getSelection()) {
				glStencilFunc(GL_ALWAYS, 1, 0xFF);
				glStencilMask(0xFF);
				model->Draw(model_shader);
			}
		}
	}

	// Render selected object with solid color shader
	for (const auto& obj : objects) {
		if (auto model = dynamic_cast<Model*>(obj)) {
			if (model->getSelection()) {
				glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
				glStencilMask(0x00);
				glDisable(GL_DEPTH_TEST);
				model->DrawStencil(outline_shader);
			}
		}
	}

	// Render skybox last
	glStencilMask(0x00);
	if (skybox) skybox->Draw(skybox_shader, activeCamera);

	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glEnable(GL_DEPTH_TEST);
}