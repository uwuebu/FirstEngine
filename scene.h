#ifndef SCENE_H_
#define SCENE_H_

#include <vector>
#include <unordered_map>
#include <string>
#include "object.h"
#include "model.h"
#include "light.h"
#include <custom/camera.h>
#include "skybox.h"

struct Properties {
	unsigned int DLShadowResolution = 4096;
	unsigned int PLShadowResolution = 2048;
};

// SCENE CLASS
//=----------------------------------=
class Scene {
private:
	std::vector<Object*> objects;  // Single container for all objects
	Camera* camera;
	Skybox* skybox = nullptr;

	unsigned int count_models = 0;
	unsigned int count_lights = 0;

	Shader* model_shader;
	Shader* outline_shader;
	Shader* skybox_shader;
	Shader* DLdepth_shader;
	Shader* PLdepth_shader;

	int active_camera;

	// Maps IDs to objects
	std::unordered_map<unsigned int, Object*> objectLookup; 
	unsigned int nextID = 1; // ID counter for objects

public:
	Properties properties;
	Scene() = default;
	~Scene() { Clear(); }

	std::vector<Object*>& getObjects() { return objects; }
	Camera* GetCamera() { return camera; }
	Skybox* GetSkybox() { return skybox; }


	// Generic add function
	template<typename T, typename... Args>
	T* AddObject(Args&&... args);
	
	Model* AddModel(Model* model, std::string name = "Model");
	
	Model* AddModel(char* path, std::string name = "Model");
	
	PointLight* AddPointLight(Camera* camera, 
														glm::vec3 position, 
														std::string name = "PointLight");

	DirectionalLight* AddDirectionalLight(Camera* camera, 
																				const glm::vec3& direction, 
																				std::string name = "DirectionalLight");
	
	SpotLight* AddSpotLight(std::string name = "SpotLight");
	
	AmbientLight* AddAmbientLight(const glm::vec3& color, 
																std::string name = "AmbientLight");
	
	Camera* AddCamera(const glm::vec3& position, std::string name = "Camera");

	Skybox* AddSkybox();

	// SETTING SHADERS
	//=-------------------------------------=
	void setModelShader(Shader& shad) { model_shader = &shad; }
	void setOutlineShader(Shader& shad) { outline_shader = &shad; }
	void setSkyboxShader(Shader& shad) { skybox_shader = &shad; }
	void setDLDepthShader(Shader& shad) { DLdepth_shader = &shad; }
	void setPLDepthShader(Shader& shad) { PLdepth_shader = &shad; }


	// Delete an object by ID
	void Delete(unsigned int id);

	// Clear the scene
	void Clear();

	void Draw(unsigned int active_camera_index, unsigned int frame_buffer, 
						unsigned int screenWidth, unsigned int screenHeight, 
						unsigned int cubeMapArray, unsigned int depthMapFBO);
};

#endif
