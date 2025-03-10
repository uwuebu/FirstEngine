#include "mine_imgui.h"

bool showPerformanceCounter = false; // Toggle state
unsigned int fps_c = 0;

void RenderMenuBar() {
  if (ImGui::BeginMainMenuBar()) {
    // "File" menu
    if (ImGui::BeginMenu("File")) {
      ImGui::MenuItem("Scene", NULL, false, false);
      if (ImGui::MenuItem("Save")) {

      }
      if (ImGui::MenuItem("Load")) {

      }
      if (ImGui::BeginMenu("Load recent")) {

        ImGui::EndMenu();
      }

      ImGui::EndMenu();
    }

    // "Edit" menu
    if (ImGui::BeginMenu("Edit")) {

      ImGui::EndMenu();
    }

    // "View" menu
    if (ImGui::BeginMenu("View")) {
      // "Asset Browser" menu item
      if (ImGui::MenuItem("Asset Browser")) {
        // Handle the opening of the Asset Browser (toggle visibility, etc.)
        // Example: showAssetBrowser = true;
      }
      // "Scene Browser" menu item
      if (ImGui::MenuItem("Scene Browser")) {
        // Handle the opening of the Scene Browser (toggle visibility, etc.)
        // Example: showSceneBrowser = true;
      }
      // "FPS counter" menu item
      if (ImGui::MenuItem("Performance counter")) {
        showPerformanceCounter = !showPerformanceCounter;
      }

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar(); // End the menu bar
  }
}

void ShowMyWindow(Scene* scene, unsigned int fps_count) {
  IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing Dear ImGui context. Refer to examples app!");
  IMGUI_CHECKVERSION();
  fps_c = fps_count;
  RenderMenuBar();
  if (showPerformanceCounter) {
    ShowinfoOverlay();
  }

  static Object* selectedObject = nullptr; // Store selected object persistently

  // Get main viewport
  const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

  // Position Scene Browser
  ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 50, main_viewport->WorkPos.y + 50), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(250, 500), ImGuiCond_Once);

  bool show_window = true;
  ImVec2 scene_browser_pos, scene_browser_size;

  if (show_window) {
    if (!ImGui::Begin("Scene Browser", &show_window)) {
      ImGui::End();
      return;
    }

    scene_browser_pos = ImGui::GetWindowPos();
    scene_browser_size = ImGui::GetWindowSize();

    std::vector<Object*>& objects = scene->getObjects();

    for (size_t i = 0; i < objects.size(); ++i) {
      Object* obj = objects[i];

      // Get text size for precise selectable width
      ImVec2 textSize = ImGui::CalcTextSize(obj->name.c_str());
      float selectableWidth = textSize.x + 10.0f; // Add padding

      // Make selection only cover the text width
      bool isSelected = (selectedObject == obj);
      if (ImGui::Selectable(obj->name.c_str(), isSelected, 0, ImVec2(selectableWidth, 0))) {
        selectedObject = obj; // Select the clicked object
      }

      // Move button to the right
      ImGui::SameLine();
      float buttonWidth = 25.0f; // Adjust button width
      float offset = ImGui::GetContentRegionAvail().x - buttonWidth;
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

      // Visibility toggle button (aligned to the right)
      if (ImGui::Button(("V##" + std::to_string(reinterpret_cast<uintptr_t>(obj))).c_str(), ImVec2(buttonWidth, 0))) {
        obj->setVisibility(!obj->getVisibility());
      }
    }

    ImGui::End();
  }

  // Draw Properties Window next to Scene Browser if an object is selected
  if (selectedObject) {
    // Position next to Scene Browser
    ImGui::SetNextWindowPos(ImVec2(scene_browser_pos.x + scene_browser_size.x + 10, scene_browser_pos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(1000, scene_browser_size.y), ImGuiCond_Always);

    selectedObject->draw_menu();
  }
}


static void ShowinfoOverlay() {
  bool open = 1;
  bool* p_open = &open;
  static int location = 0;
  ImGuiIO& io = ImGui::GetIO();
  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
  if (location >= 0)
  {
    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
    ImVec2 work_size = viewport->WorkSize;
    ImVec2 window_pos, window_pos_pivot;
    window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
    window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
    window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
    window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    window_flags |= ImGuiWindowFlags_NoMove;
  }
  else if (location == -2)
  {
    // Center window
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    window_flags |= ImGuiWindowFlags_NoMove;
  }
  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
  if (ImGui::Begin("Overlay", p_open, window_flags))
  {
    ImGui::Text("Overlay\n" "(right-click to change position)");
    ImGui::Separator();
    ImGui::Text("FPS: %d", fps_c);
  }
  ImGui::End();
}