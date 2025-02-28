#include "imgui.h"

void ShowMyWindow(float& f1) {
    // Exceptionally add an extra assert here for people confused about initial Dear ImGui setup
    // Most functions would normally just assert/crash if the context is missing.
    IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing Dear ImGui context. Refer to examples app!");

    // Verify ABI compatibility between caller code and compiled version of Dear ImGui. This helps detects some build issues.
    IMGUI_CHECKVERSION();

    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

    // Main body of the window
    if (!ImGui::Begin("My window")){
        ImGui::End();
        return;
    }
    ImGui::PushItemWidth(ImGui::GetFontSize() * -12);

    ImGui::SliderFloat("slider float", &f1, 0.0f, 120.0f, "ratio = %1.0f");
}