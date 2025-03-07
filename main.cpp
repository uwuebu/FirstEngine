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

#include "mine_imgui.h"

#include "window.h"
#include "input_handler.h"
#include "renderer.h"

// standart libraries
#include <iostream>
#include <string.h>
#include <vector>

// Initialisations
void renderLightQuad(const glm::vec3& lightPos, const glm::mat4& view, Shader& shader);
void renderQuad();

// Global variable setup
bool debug = 0;
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
unsigned int quadVAO, quadVBO;
int fps;
World_param world;
int debugLayer = 0;

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

  // Setup ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window.GetWindowPTR(), true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
  ImGui_ImplOpenGL3_Init();

  // SOME PARAMETRS
  double previousTime = glfwGetTime();
  int frameCount = 0;

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

  PointLight* pointLight1 = scene.AddPointLight(camera, glm::vec3(2.0f, 2.0f, 2.0f));

  PointLight* pointLight2 = scene.AddPointLight(camera, glm::vec3(1.0f, 2.0f, 2.0f));

  SpotLight* spotLight = scene.AddSpotLight("Flashlight");
  //spotLight->update(&shader, 0);

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

  //skyboxShader.setInt("skybox", 0);

  char path[] = "resources/models/backpack/backpack.obj";
  Model* backpack = scene.AddModel(path);
  backpack->setPosition(glm::vec3(-3.0, 1.0, 1.0));

  unsigned int light_texture = TextureFromFile("lightb.png", "resources/textures/default");

  //// DEBUG QUAD SETUP
  // =----------------------=
  Shader debugDepthQuad("quad.vert", "debug_quad.frag");
  debugDepthQuad.use();
  debugDepthQuad.setInt("depthMap", 0);


  Renderer renderer(&window, &scene);
  //renderer.SetScene(&scene);
  // RENDER LOOP
  // =--------------------------------------------------=
  while (!glfwWindowShouldClose(window.GetWindowPTR())) {
    // Gamma correction
    glEnable(GL_FRAMEBUFFER_SRGB);

    camera->screenHeight = window.GetScreenHeight();
    camera->screenWidth = window.GetScreenWidth();
    //camera->uniform_buffer = uboMatrices;

    // input
    input_handler.processInput(window.GetWindowPTR(), deltaTime);
    input_handler.toggle_flashlight(window.GetWindowPTR(), *spotLight);

    // DeltaTime calculation
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    glClearColor(world.color[0], world.color[1], world.color[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // RENDER_SCENE
    //=------------------------=
    glViewport(0, 0, window.GetScreenWidth(), window.GetScreenHeight());
    glBindFramebuffer(GL_FRAMEBUFFER, renderer.frame_buffer);

    glEnable(GL_DEPTH_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glEnable(GL_CULL_FACE);

    spotLight->setPosition(camera->getPosition());
    spotLight->setDirection(camera->Front);

    glStencilFunc(GL_ALWAYS, 1, 0xFF); // all fragments pass the stencil test
    glStencilMask(0xFF); // enable writing to the stencil buffer

    renderer.RenderScene();

    /*light_shader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, light_texture);
    light_shader.setVec3("lightPos", pointLight->getPosition());
    light_shader.setFloat("radius", 0.5f);
    light_shader.setVec3("lightColor", glm::vec3(1.0, 1.0, 1.0));
    light_shader.setVec3("viewPos", camera->getPosition());
    camera->update_shaders(&light_shader);*/

    //renderLightQuad(pointLight->getPosition(), camera->GetViewMatrix(), light_shader);

    //glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
    //glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT);
    //quadShader.use();
    //glBindVertexArray(fullquadVAO);
    //glDisable(GL_DEPTH_TEST);
    //glBindTexture(GL_TEXTURE_2D, texColorBuffer);
    //glDrawArrays(GL_TRIANGLES, 0, 6);

    // render ImGui
    if (input_handler.DoImGUI()) {
      // Measure framerate
      double currentTime = glfwGetTime();
      frameCount++;
      // If a second has passed.
      if (currentTime - previousTime >= 1.0) {
        // Display the frame count here any way you want.
        fps = frameCount;
        frameCount = 0;
        previousTime = currentTime;
      }

      // Gamma correction
      glDisable(GL_FRAMEBUFFER_SRGB);
      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();
      //ImGui::ShowDemoWindow(); // Show demo window! :)
      ShowMyWindow(&scene, world, fps);
      // Render ImGUI ontop
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    debugDepthQuad.use();
    debugDepthQuad.setInt("layer", debugLayer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, directionLight->depthMap);
    if (debug)
    {
      renderQuad();
    }

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window.GetWindowPTR());
    glfwPollEvents();
  }

  //glDeleteFramebuffers(1, &fbo);

  //clear ImGUI
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  // glfw: terminate, clearing all previously allocated GLFW resources.
  // ------------------------------------------------------------------
  glfwTerminate();
  return 0;
}

void renderLightQuad(const glm::vec3& lightPos, const glm::mat4& view, Shader& shader) {
  if (quadVAO == 0) {
    float quadVertices[] = {
      // Positions       // Texture Coords
      -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, // Top-left
      -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, // Bottom-left
       0.5f, -0.5f, 0.0f,  1.0f, 0.0f, // Bottom-right

      -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, // Top-left
       0.5f, -0.5f, 0.0f,  1.0f, 0.0f, // Bottom-right
       0.5f,  0.5f, 0.0f,  1.0f, 1.0f  // Top-right
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  // Billboard transformation (Quad faces the camera)
  glm::mat4 model = glm::mat4(1.0f);
  model = glm::translate(model, lightPos);  // Move quad to light position


  // Extract camera right and up vectors from the view matrix
  glm::vec3 cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
  glm::vec3 cameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);

  // Billboard rotation
  model[0] = glm::vec4(cameraRight, 0.0f);
  model[1] = glm::vec4(cameraUp, 0.0f);
  model[2] = glm::vec4(glm::cross(cameraRight, cameraUp), 0.0f);  // Forward vector
  model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));

  // Set uniforms
  shader.use();
  shader.setMat4("model", model);
  shader.setMat4("view", view);

  // Render the quad
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
}

//renderQuad() renders a 1x1 XY quad in NDC
//-----------------------------------------
unsigned int debug_quadVAO = 0;
unsigned int debug_quadVBO;
void renderQuad() {
  if (debug_quadVAO == 0)
  {
    float quadVertices[] = {
      // positions        // texture Coords
      -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
       1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
       1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // setup plane VAO
    glGenVertexArrays(1, &debug_quadVAO);
    glGenBuffers(1, &debug_quadVBO);
    glBindVertexArray(debug_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debug_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  }
  glBindVertexArray(debug_quadVAO);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}