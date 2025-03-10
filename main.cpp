// Custom includes
#include "scene.h"
#include "stb_image.h"
#include <SHADER/shader_c.h>

#include "window.h"
#include "input_handler.h"
#include "renderer.h"

// standart libraries
#include <iostream>
#include <string.h>
#include <vector>

Model* createPlaneModel(const std::string& textureFile, const std::string& path) {
  vector<Vertex> vertices = {
    // Positions          // Normals        // Texture Coords
    {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // Bottom-left
    {{0.5f, 0.0f, -0.5f},  {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // Bottom-right
    {{0.5f, 0.0f, 0.5f},   {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, // Top-right
    {{-0.5f, 0.0f, 0.5f},  {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}  // Top-left
  };

  vector<unsigned int> indices = {
      2, 1, 0, // First triangle
      0, 3, 2 // Second triangle
  };

  vector<Texture> textures;
  Texture texture;
  texture.id = TextureFromFile(textureFile.c_str(), path.c_str()); // Load texture
  texture.type = "texture_diffuse";
  texture.path = textureFile;
  textures.push_back(texture);

  return new Model(Mesh(vertices, indices, textures));
}

int main() {
  Window window(4, 5);
  InputHandler input_handler;
  window.SetInputHandler(&input_handler);

  Scene scene;
  input_handler.SetScene(&scene);

  // Adding camera object
  Camera* camera = scene.AddCamera(glm::vec3(0.0f, 0.0f, 3.0f));

  // Adding ambient light object
  AmbientLight* ambientLight = scene.AddAmbientLight(glm::vec3(0.3f, 0.3f, 0.3f));

  // Add directional light to scene
  DirectionalLight* directionLight = scene.AddDirectionalLight(camera, glm::vec3(-0.2f, -0.5f, -0.3f), "Sun");

  // Create point lights and add to scene
  PointLight* pointLight = scene.AddPointLight(camera, glm::vec3(3.0f, 2.0f, 2.0f));
  //pointLight->setIntensity(1.0f);

  //PointLight* pointLight1 = scene.AddPointLight(camera, glm::vec3(2.0f, 2.0f, 2.0f));

  //PointLight* pointLight2 = scene.AddPointLight(camera, glm::vec3(1.0f, 2.0f, 2.0f));

  SpotLight* spotLight = scene.AddSpotLight("Flashlight");

  // Set path to cube object
  char path_cube[] = "resources/models/default/CUBE/default_cube.obj";

  // Add first cube to scene
  Model* default_cube_1 = scene.AddModel(path_cube, "Default_cube1");
  default_cube_1->setSelection(false);
  default_cube_1->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
  default_cube_1->scale_texture = true;

  // Add second cube to scene
  Model* default_cube_2 = scene.AddModel(path_cube, "Default_cube2");
  default_cube_2->setSelection(false);
  default_cube_2->setPosition(glm::vec3(3.0f, 2.0f, 0.0f));
  default_cube_2->scale_texture = true;

  // Add plane to scene
  Model* plane = createPlaneModel("concrete_diffuse.png", "resources/textures/default");
  scene.AddModel(plane, "Plane");
  plane->setSelection(false);
  plane->setPosition(glm::vec3(1.5f, -1.0001f, 0.0f));
  plane->setSize(glm::vec3(20.0f));
  plane->scale_texture = true;

  // Add Skybox to scene
  Skybox* skybox = scene.AddSkybox();

  char path[] = "resources/models/backpack/backpack.obj";
  Model* backpack = scene.AddModel(path);
  backpack->setPosition(glm::vec3(-3.0, 1.0, 1.0));

  unsigned int light_texture = TextureFromFile("lightb.png", "resources/textures/default");

  // Setting up the renderer
  Renderer renderer(&window, &scene);

  camera->screenHeight = window.GetScreenHeight();
  camera->screenWidth = window.GetScreenWidth();
  // RENDER LOOP
  // =--------------------------------------------------=
  while (!glfwWindowShouldClose(window.GetWindowPTR())) {
    // INITIAL PARAMETERS
    //=------------------=
    spotLight->setPosition(camera->getPosition());
    spotLight->setDirection(camera->Front);

    // INPUT HANDLING
    //=-------------=
    input_handler.processInput(window.GetWindowPTR(), renderer.deltaTime);
    input_handler.toggle_flashlight(window.GetWindowPTR(), *spotLight);


    // RENDER_SCENE
    //=------------------------=
    renderer.RenderScene(input_handler.DoImGUI());

    /*light_shader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, light_texture);
    light_shader.setVec3("lightPos", pointLight->getPosition());
    light_shader.setFloat("radius", 0.5f);
    light_shader.setVec3("lightColor", glm::vec3(1.0, 1.0, 1.0));
    light_shader.setVec3("viewPos", camera->getPosition());
    camera->update_shaders(&light_shader);*/

    //renderLightQuad(pointLight->getPosition(), camera->GetViewMatrix(), light_shader);
  }

  renderer.Terminate();

  return 0;
}

//void renderLightQuad(const glm::vec3& lightPos, const glm::mat4& view, Shader& shader) {
//  if (quadVAO == 0) {
//    float quadVertices[] = {
//      // Positions       // Texture Coords
//      -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, // Top-left
//      -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, // Bottom-left
//       0.5f, -0.5f, 0.0f,  1.0f, 0.0f, // Bottom-right
//
//      -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, // Top-left
//       0.5f, -0.5f, 0.0f,  1.0f, 0.0f, // Bottom-right
//       0.5f,  0.5f, 0.0f,  1.0f, 1.0f  // Top-right
//    };
//
//    glGenVertexArrays(1, &quadVAO);
//    glGenBuffers(1, &quadVBO);
//
//    glBindVertexArray(quadVAO);
//    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
//
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
//    glEnableVertexAttribArray(0);
//
//    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
//    glEnableVertexAttribArray(1);
//
//    glBindBuffer(GL_ARRAY_BUFFER, 0);
//    glBindVertexArray(0);
//  }
//
//  // Billboard transformation (Quad faces the camera)
//  glm::mat4 model = glm::mat4(1.0f);
//  model = glm::translate(model, lightPos);  // Move quad to light position
//
//
//  // Extract camera right and up vectors from the view matrix
//  glm::vec3 cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
//  glm::vec3 cameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
//
//  // Billboard rotation
//  model[0] = glm::vec4(cameraRight, 0.0f);
//  model[1] = glm::vec4(cameraUp, 0.0f);
//  model[2] = glm::vec4(glm::cross(cameraRight, cameraUp), 0.0f);  // Forward vector
//  model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
//
//  // Set uniforms
//  shader.use();
//  shader.setMat4("model", model);
//  shader.setMat4("view", view);
//
//  // Render the quad
//  glBindVertexArray(quadVAO);
//  glDrawArrays(GL_TRIANGLES, 0, 6);
//  glBindVertexArray(0);
//}