#include <string>
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include <sqlite3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "FSRS.hpp"
#include "models.hpp"
#include "imguiHelpers.hpp"
#include "FlashCard.hpp"
#include "db.hpp"

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "SRS APP", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initialize Database
    sqlite3 *db;
    char *errMsg;

    if (!openDB(&db, "./database/myDB")) {
        return 1;
    }

    if (!createDBTables(db, &errMsg)) {
        closeDB(db);
        return 1;
    }
    
    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    bool reveal_card = false;
    bool create_card = false;
    
    FSRS f = FSRS();
    FlashCard *flash_card = nullptr;

    char answer_string[255] = {0};
    char question_string[255] = {0};
    
    std::string next_card_time = "";
    if (!getNextCardTime(db, next_card_time)) {
        closeDB(db);
        return 1;
    }

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Get current time
        std::time_t now_t = std::time(nullptr);
        std::tm* now_tm = gmtime(&now_t);
        
        std::stringstream ss;
        ss << std::put_time(now_tm, "%Y-%m-%dT%H:%M:%S");
        std::string curr_time_str = ss.str();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        { // This is the main window for the FSRS algorithm
            ImGui::Begin("FSRS"); 

            ImGui::Text("Time Now: %s (GMT)", curr_time_str.c_str() );
            ImGui::Text("Next Card Available At: %s (GMT)", (next_card_time == "") ? "None" : next_card_time.c_str() );

            if (ImGui::Button("Pull Card") && !flash_card) {
                if (!pullCard(db, &flash_card)) {
                    break;		
                }
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Create New Card") && !create_card) {
                create_card = true;
            }

            ImGui::End();
        }

        if (flash_card) { // This is the window for the flash card if pulled
            ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f), 8);

            FlashCardStatus status = drawFlashCard(*flash_card, reveal_card);

            Rating flash_card_rating = Rating::Again;

            switch (status) {
                case FlashCardStatus::HARD :
                    flash_card_rating = Rating::Hard;
                    break;
                case FlashCardStatus::GOOD :
                    flash_card_rating = Rating::Good;
                    break;
                case FlashCardStatus::EASY :
                    flash_card_rating = Rating::Easy;
                    break;
                default:
                    break;
            }

            if (status != FlashCardStatus::NONE) {
                // Resechedule the card based on the rating
                const auto& [new_card_metadata, review_log] = f.reviewCard(flash_card->card, flash_card_rating); 
            
                // Update the card in database
                if (!updateCard(db, new_card_metadata, flash_card)) {
                    break;
                }

                freeFlashCard(flash_card);
                flash_card = nullptr;
                reveal_card = false;

                // Update next card time after rescheduling card
                if (!getNextCardTime(db, next_card_time)) {
                    break;
                }

            }
        }

        if (create_card) { // This is the window to create a new card
            ImGui::Begin("Create new card");
            
            ImGui::InputText("Question", question_string, IM_ARRAYSIZE(question_string));
            ImGui::InputText("Answer", answer_string, IM_ARRAYSIZE(answer_string));

            if (ImGui::Button("Submit")) {
                if (!createCard(db, &errMsg, question_string, answer_string)) {
                    break;
                }
                create_card = false;
                memset(question_string, 0, IM_ARRAYSIZE(question_string));
                memset(answer_string, 0, IM_ARRAYSIZE(answer_string));

                // Update next card time after adding card
                if (!getNextCardTime(db, next_card_time)) {
                    break;
                }

            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel")) {
                create_card = false;
                memset(question_string, 0, IM_ARRAYSIZE(question_string));
                memset(answer_string, 0, IM_ARRAYSIZE(answer_string));
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w
        );
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    closeDB(db);

    return 0;
}
