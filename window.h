#ifndef WINDOW_H_
#define WINDOW_H_

// Graphics libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// standart libraries
#include <iostream>

#include "input_handler.h"

float lastX = 400, lastY = 300;

class Window {
  GLFWwindow* window_;
  int screenWidth;
  int screenHeight;
  InputHandler* input_handler_;

public:
  Window(int gl_version_major, int gl_version_minor) {
    // GLFW SET UP
    // =----------------------------------------------------------=
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gl_version_major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gl_version_minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    screenWidth = mode->width;
    screenHeight = mode->height;

    // glfw window creation
    window_ = glfwCreateWindow(screenWidth, screenHeight, "LearnOpenGL", 
                              NULL, NULL);
    if (window_ == NULL) {
      std::cout << "Failed to create GLFW window" << std::endl;
      glfwTerminate();
    }
    glfwMakeContextCurrent(window_);
    glfwMaximizeWindow(window_);
    glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);

    // Set this Window instance as the user pointer for the GLFW window
    glfwSetWindowUserPointer(window_, this);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      std::cout << "Failed to initialize GLAD" << std::endl;
    }

    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window_, mouse_callback);
    glfwSetScrollCallback(window_, scroll_callback);
    glfwSwapInterval(0);
  }

  void SetInputHandler(InputHandler* input_handler) { input_handler_ = input_handler; }

  GLFWwindow* GetWindowPTR() { return window_; }
  int GetScreenWidth() { return screenWidth; }
  int GetScreenHeight() { return screenHeight; }



private:
  // glfw: whenever the window size changed (by OS or user resize)
  // this callback function executes
  // -----------------------------------------------------------------------
  static void framebuffer_size_callback(GLFWwindow* window, 
                                        int width, int height) {
    glViewport(0, 0, width, height);
  }

  static void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent((float)xpos, (float)ypos);

    if (io.WantCaptureMouse)
      return;

    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->input_handler_) {
      static bool mouseHeld = false;

      if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (!mouseHeld) {
          lastX = xpos;
          lastY = ypos;
          mouseHeld = true;
          return;
        }
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        win->input_handler_->ProcessMouseMovement(xoffset, yoffset, true);
      }
      else {
        mouseHeld = false;
      }

      lastX = xpos;
      lastY = ypos;
    }
  }

  static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent((float)xoffset, (float)yoffset);

    if (io.WantCaptureMouse)
      return;

    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->input_handler_) {
      win->input_handler_->ProcessMouseScroll(yoffset);
    }
  }
};
#endif