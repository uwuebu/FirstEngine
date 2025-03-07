#ifndef INPUTHANDLER_H_
#define INPUTHANDLER_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "scene.h"

static class InputHandler {
  Scene* scene_ = nullptr;
  float MovementSpeed = 2.5f;
  float MouseSensitivity = 0.1f;
  bool renderImGUI = 0;

public:
  InputHandler() {};
  void SetScene(Scene* scene) { scene_ = scene; };

  bool DoImGUI() { return renderImGUI; };

  void ProcessMouseMovement(float xoffset,
                         float yoffset,
                         GLboolean constrainPitch = true) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    glm::vec3 camera_rotation = scene_->GetCamera()->getRotation();
    camera_rotation.y += xoffset;
    camera_rotation.x += yoffset;

    // Constrain pitch to avoid flipping
    if (constrainPitch) {
      if (camera_rotation.x > 89.0f)
        camera_rotation.x = 89.0f;
      if (camera_rotation.x < -89.0f)
        camera_rotation.x = -89.0f;
    }

    scene_->GetCamera()->setRotation(camera_rotation);
    scene_->GetCamera()->updateCameraVectors();
  }
  void ProcessMouseScroll(float yoffset) {
    scene_->GetCamera()->Zoom -= (float)yoffset;
    if (scene_->GetCamera()->Zoom < 1.0f) scene_->GetCamera()->Zoom = 1.0f;
    if (scene_->GetCamera()->Zoom > 120.0f) scene_->GetCamera()->Zoom = 120.0f;
  }

  // process all input: 
  // query GLFW whether relevant keys are pressed/released this frame 
  // and react accordingly
  // --------------------------------------------------------------------
  void processInput(GLFWwindow* window, float delta_time) {
    Camera* camera = scene_->GetCamera();
    // Exit program
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    // Toggle ImGUI
    static bool f5PressedLastFrame = false; // Track F5 state
    bool f5CurrentlyPressed = (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS);
    if (f5CurrentlyPressed && !f5PressedLastFrame) {
      renderImGUI = !renderImGUI;
    }
    f5PressedLastFrame = f5CurrentlyPressed;

    //// Toggle Debug
    //static bool EPressedLastFrame = false; // Track F5 state
    //bool ECurrentlyPressed = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);
    //if (ECurrentlyPressed && !EPressedLastFrame) {
    //  //debug = !debug;
    //}
    //EPressedLastFrame = ECurrentlyPressed;

    //// cycle the depthmaps
    //static bool arrowPressedLastFrame = false; // Track F5 state
    //bool arrowCurrentlyPressed = (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
    //if (arrowCurrentlyPressed && !arrowPressedLastFrame) {
    //  if (debugLayer == 3)
    //    debugLayer = 0;
    //  else
    //    debugLayer++;
    //}
    //arrowPressedLastFrame = arrowCurrentlyPressed;

    bool shift = 0;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
      shift = 1;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera->ProcessKeyboard(FORWARD, shift, delta_time);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera->ProcessKeyboard(BACKWARD, shift, delta_time);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera->ProcessKeyboard(LEFT, shift, delta_time);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera->ProcessKeyboard(RIGHT, shift, delta_time);


  }
  
  void toggle_flashlight(GLFWwindow* window, SpotLight& flashlight) {
    static bool fPressedLastFrame = false; // Track F5 state
    bool fCurrentlyPressed = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS);
    if (fCurrentlyPressed && !fPressedLastFrame) {
      if (flashlight.getIntensity() == 1.0f)
        flashlight.setIntensity(0.0f);
      else
        flashlight.setIntensity(1.0f);
    }

    fPressedLastFrame = fCurrentlyPressed;
  }
};

#endif