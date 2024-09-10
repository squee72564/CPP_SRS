#include "imguiHelpers.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <string>
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "FSRS.hpp"
#include "models.hpp"

static void centerTextConditional(const ImVec2& window_size, const ImVec2& text_size) {
    if (window_size.x >= text_size.x) {
        ImGui::SetCursorPosX((window_size.x - text_size.x) * 0.5f);
    }
}

void drawFlashCard(const std::string& card_string, bool& reveal_card) {

    ImGui::Begin("Flashcard");

    ImVec2 window_size = ImGui::GetWindowSize();
    ImVec2 text_size = ImGui::CalcTextSize(card_string.c_str());
    std::string checkbox_text = (reveal_card) ? "Hide Card" : "Reveal card";

    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    centerTextConditional(window_size, text_size);
    ImGui::TextWrapped(card_string.c_str());

    ImGui::Dummy(ImVec2(0.0f, window_size.y - 100.0f - text_size.y));

    ImGui::Checkbox(checkbox_text.c_str(), &reveal_card);

    if (reveal_card) {
        ImGui::SameLine();
        ImGui::Button("Easy");
        ImGui::SameLine();
        ImGui::Button("Good");
        ImGui::SameLine();
        ImGui::Button("Hard");
        ImGui::SameLine();
        ImGui::Button("Relearn");
    }

    ImGui::End();
}
