// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "misc/cpp/imgui_stdlib.h"
#include <stdio.h>
#include <string>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "db.h"
#include "queries.h"
#include "availability.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

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
int main(int, char **) {

    // --- Connection setup ---
    std::string host, user, password, database;
    int port = 3306;

    // Defaults (override via env vars for security)
    host     = getenv("DB_HOST")     ? getenv("DB_HOST")     : "127.0.0.1";
    user     = getenv("DB_USER")     ? getenv("DB_USER")     : "root";
    password = getenv("DB_PASS")     ? getenv("DB_PASS")     : "";
    database = getenv("DB_NAME")     ? getenv("DB_NAME")     : "FlightDB2";
    if (getenv("DB_PORT")) port = std::stoi(getenv("DB_PORT"));

    DB db;
    if (!db.connect(host, user, password, database, port)) {
        return 1;
    }
    std::cout << "[ok] Connected to MySQL (" << database << ")" << std::endl;

  
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
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
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow* window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "FlightDB GUI", nullptr, nullptr);
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
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    io.Fonts->AddFontFromFileTTF("./imgui/misc/fonts/Cousine-Regular.ttf");

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    bool show_trip_window = false;
    std::string tripSource, tripDest;

    bool show_flight_window = false;
    std::string flightSearchNo;

    bool show_util_window = false;
    std::string utilStartDate, utilEndDate;

    bool show_availability_window = false;
    std::string seatFlightNo, seatLegNo, seatFlightDate;

    bool show_itinerary_window = false;
    std::string passengerName;
    std::vector<std::map<std::string, std::string>> rows;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {          
            ImGui::Begin("Input Query");

            ImGui::Text("Enter a trip source and destination to find all flights");
            ImGui::InputText("Source", &tripSource);
            ImGui::InputText("Destination", &tripDest);

            if (ImGui::Button("Search for trips")) {
                show_trip_window = true;
            }

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::Text("Enter a flight number to see details");
            ImGui::InputText("Flight No.", &flightSearchNo);
            if (ImGui::Button("Search for flights")) {
                show_flight_window = true;
            }

            ImGui::Spacing();
            ImGui::Separator();            

            ImGui::Text("Enter a date range to see aircraft utilization over that period (year-month-day)");
            ImGui::InputText("Start Date (xxxx-xx-xx)", &utilStartDate);
            ImGui::InputText("End Date (xxxx-xx-xx)", &utilEndDate);
            if (ImGui::Button("See aircraft utilization")) {
                show_util_window = true;
            }

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::Text("Enter a flight number and date (year-month-day) to check seat availability");
            ImGui::InputText("Flight", &seatFlightNo);
            ImGui::InputText("Seat Leg No.", &seatLegNo);
            ImGui::InputText("Flight Date", &seatFlightDate);
            if (ImGui::Button("Check seat availability")) {
                show_availability_window = true;
            }

            ImGui::Spacing();
            ImGui::Separator();

            ImGui::Text("Enter your name to see booked flights");
            ImGui::InputText("Passenger Name", &passengerName);
            if (ImGui::Button("Show itinerary")) {
                rows = itinerary(db, passengerName);

                show_itinerary_window = true;
            }                    
            
            ImGui::End();
        }

        if (show_trip_window) {
            ImGui::Begin("Trip", &show_trip_window);

            searchTrip(db, tripSource, tripDest);

            ImGui::End();
        }

        if (show_flight_window) {
            ImGui::Begin("Flight", &show_flight_window);

            searchFlight(db, flightSearchNo);
            
            ImGui::End();
        }

        if (show_util_window) {
            ImGui::Begin("Aircraft Utilization", &show_util_window);

            util(db, utilStartDate, utilEndDate);

            ImGui::End();
        }

        if (show_availability_window) {
            ImGui::Begin("Seat Availability", &show_availability_window);

            checkAvailability(db, seatFlightNo, seatLegNo, seatFlightDate);

            ImGui::End();
        }

        if (show_itinerary_window) {
            ImGui::Begin("Itinerary Check", &show_itinerary_window);

            if (rows.empty()) {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                    "No itinerary for this customer."
                    );
            } else {

                ImGui::TextUnformatted("RESERVATIONS");
                ImGui::Separator();

                if (ImGui::BeginTable("itinerary_table", 9,
                                      ImGuiTableFlags_Borders |
                                      ImGuiTableFlags_RowBg |
                                      ImGuiTableFlags_Resizable |
                                      ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("Name");
                    ImGui::TableSetupColumn("Flight");
                    ImGui::TableSetupColumn("Date");
                    ImGui::TableSetupColumn("Leg");
                    ImGui::TableSetupColumn("From");
                    ImGui::TableSetupColumn("To");
                    ImGui::TableSetupColumn("Dep Time");
                    ImGui::TableSetupColumn("Arr Time");
                    ImGui::TableSetupColumn("Seat");
                    ImGui::TableHeadersRow();

                    for (auto& row : rows) {
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(row["customer_name"].c_str());

                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(row["number"].c_str());

                        ImGui::TableSetColumnIndex(2);
                        ImGui::TextUnformatted(row["date"].c_str());

                        ImGui::TableSetColumnIndex(3);
                        ImGui::TextUnformatted(row["leg_no"].c_str());

                        ImGui::TableSetColumnIndex(4);
                        ImGui::TextUnformatted(row["dep_airport_code"].c_str());

                        ImGui::TableSetColumnIndex(5);
                        ImGui::TextUnformatted(row["arr_airport_code"].c_str());

                        ImGui::TableSetColumnIndex(6);
                        ImGui::TextUnformatted(row["scheduled_dep_time"].c_str());

                        ImGui::TableSetColumnIndex(7);
                        ImGui::TextUnformatted(row["scheduled_arr_time"].c_str());

                        ImGui::TableSetColumnIndex(8);
                        ImGui::TextUnformatted(row["seat_no"].c_str());
                    }

                    ImGui::EndTable();
                }                
            }

            ImGui::End();
        }          

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
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

    return 0;
}
