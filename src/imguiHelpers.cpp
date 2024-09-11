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
#include "FlashCard.hpp"

static void centerTextConditional(const ImVec2& window_size, const ImVec2& text_size) {
    if (window_size.x >= text_size.x) {
        ImGui::SetCursorPosX((window_size.x - text_size.x) * 0.5f);
    }
}

FlashCardStatus drawFlashCard(const FlashCard& card, bool& reveal_card) {

    ImGui::Begin("Flashcard");

    ImVec2 window_size = ImGui::GetWindowSize();
    std::string checkbox_text = (reveal_card) ? "Hide Card" : "Reveal card";
    std::string card_string = (reveal_card) ? card.a : card.q;
    ImVec2 text_size = ImGui::CalcTextSize(card_string.c_str());

    ImGui::Dummy(ImVec2(0.0f, 20.0f));

    centerTextConditional(window_size, text_size);
    ImGui::TextWrapped("%s", card_string.c_str());

    ImGui::Dummy(ImVec2(0.0f, window_size.y - 100.0f - text_size.y));

    ImGui::Checkbox(checkbox_text.c_str(), &reveal_card);

    FlashCardStatus status = NONE;

    if (reveal_card) {
        ImGui::SameLine(ImGui::GetItemRectSize().x + 20.0f);
        if (ImGui::Button("Easy")) {
            status = FlashCardStatus::EASY;
        }

        ImGui::SameLine();

        if (ImGui::Button("Good")) {
            status = FlashCardStatus::GOOD;
        }

        ImGui::SameLine();

        if (ImGui::Button("Hard")) {
            status = FlashCardStatus::HARD;
        }

        ImGui::SameLine();

        if (ImGui::Button("Again")) {
            status = FlashCardStatus::AGAIN;
        }
    }

    ImGui::End();

    return status;
}
