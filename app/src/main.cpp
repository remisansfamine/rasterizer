#include <cstdio>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <rdr/renderer.h>
#include <scn/scene.h>

#include <common/maths.hpp>
#include <common/camera.hpp>

#include "framebuffer.hpp"
#include "gif_recorder.hpp"

// Set to 0 to disable high perf GPU
#if 1
extern "C"
{
    __declspec(dllexport) int NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

static void debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    printf("OpenGL error: '%s'\n", message);
}

static void errorCallback(int errorCode, const char* description)
{
    printf("GLFW error (code=%d): '%s'\n", errorCode, description);
}

static GLFWwindow* initWindow(int width, int height, const char* title)
{
    // Setup glfw
    glfwSetErrorCallback(errorCallback);
    if (!glfwInit())
    {
        printf("glfwInit failed\n");
        return nullptr;
    }

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(width, height, "Software renderer", nullptr, nullptr);
    if (window == nullptr)
    {
        printf("glfwCreateWindow failed\n");
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // V-Sync
    
    // Setup glad
    if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0)
    {
        printf("gladLoaderLoadGL failed\n");
        return nullptr;
    }

    // Setup KHR debug
    if (GLAD_GL_KHR_debug)
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugMessageCallback, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PERFORMANCE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 0, nullptr, GL_FALSE);
    }

    printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
    printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    return window;
}

static void newFrame(bool mouseCaptured)
{
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    if (mouseCaptured)
        ImGui::GetIO().MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    ImGui::NewFrame();
}

static void presentFrame(GLuint transitionFramebuffer, int windowWidth, int windowHeight, int frameWidth, int frameHeight)
{
    const float frameAspect = static_cast<float>(frameWidth) / static_cast<float>(frameHeight);
    const float windowAspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    
    int dstX0, dstY0, dstX1, dstY1;
    if (windowAspect > frameAspect)
    {
        const int dstW = int(windowHeight * frameAspect);
        const int dstH = windowHeight;
        dstX0 = (windowWidth - dstW) / 2.f;
        dstY0 = 0;
        dstX1 = dstX0 + dstW;
        dstY1 = dstH;
    }
    else
    {
        const int dstW = windowWidth;
        const int dstH = int(windowWidth / frameAspect);
        dstX0 = 0;
        dstY0 = (windowHeight - dstH) / 2.f;
        dstX1 = dstW;
        dstY1 = dstY0 + dstH;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, transitionFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glBlitFramebuffer(0, frameHeight, frameWidth, 0,   // src rect with mirrored height to flip the result
                      dstX0, dstY0, dstX1, dstY1,   // dst rect
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

static void endFrame(GLFWwindow* window, GLuint transitionFramebuffer, const Framebuffer& framebufferToPresent)
{
    glClear(GL_COLOR_BUFFER_BIT);

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    // Display framebuffer (renderer output)
    presentFrame(transitionFramebuffer, windowWidth, windowHeight, framebufferToPresent.getWidth(), framebufferToPresent.getHeight());

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

int main(int argc, char* argv[])
{
    // Init window
    GLFWwindow* window = initWindow(1200, 800, "Software renderer tester");
    if (window == nullptr)
        return -1;

    // Create renderer framebuffer (color+depth+opengl texture)
    // We need an OpenGL texture to display the result of the renderer to the screen
    Framebuffer framebuffer(800, 400);

    // Init renderer
    rdrImpl* renderer = rdrInit(
        framebuffer.getColorBufferRef(),
        framebuffer.getDepthBuffer(),
        framebuffer.getWidth(), framebuffer.getHeight());

    GLuint transitionFramebuffer;
    glGenFramebuffers(1, &transitionFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, transitionFramebuffer); // source = READ
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer.getColorTexture(), 0);

    GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("transitionFramebuffer incomplete: 0x%x\n", status);
    }

    rdrSetImGuiContext(renderer, ImGui::GetCurrentContext());

    scnImpl* scene = scnCreate();
    scnSetImGuiContext(scene, ImGui::GetCurrentContext());

    CameraInputs inputs;
    Camera camera(framebuffer.getWidth(), framebuffer.getHeight());

    bool mouseCaptured = false;
    double mouseX = 0.0;
    double mouseY = 0.0;
    float mouseDeltaX = 0.0;
    float mouseDeltaY = 0.0;

    bool captureGif = false;
    GifRecorder gifRecorder(framebuffer.getWidth(), framebuffer.getHeight());
    while (glfwWindowShouldClose(window) == false)
    {
        newFrame(mouseCaptured);

        {
            double newMouseX, newMouseY;
            glfwGetCursorPos(window, &newMouseX, &newMouseY);
            mouseDeltaX = (float)(newMouseX - mouseX);
            mouseDeltaY = (float)(newMouseY - mouseY);
            mouseX = newMouseX;
            mouseY = newMouseY;
        }

        float time = (float)glfwGetTime();
        float deltaTime = ImGui::GetIO().DeltaTime;

        // Update camera
        if (ImGui::IsKeyPressed(GLFW_KEY_ESCAPE))
        {
            mouseCaptured = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (mouseCaptured)
        {
            inputs.deltaX = mouseDeltaX;
            inputs.deltaY = mouseDeltaY;
            inputs.moveForward  = ImGui::IsKeyDown(GLFW_KEY_UP)   || ImGui::IsKeyDown(GLFW_KEY_W);
            inputs.moveBackward = ImGui::IsKeyDown(GLFW_KEY_DOWN) || ImGui::IsKeyDown(GLFW_KEY_S);
            inputs.moveUpward = ImGui::IsKeyDown(GLFW_KEY_SPACE);
            inputs.moveDownward = ImGui::IsKeyDown(GLFW_KEY_LEFT_SHIFT);
            inputs.moveLeft = ImGui::IsKeyDown(GLFW_KEY_A) || ImGui::IsKeyDown(GLFW_KEY_LEFT);
            inputs.moveRight = ImGui::IsKeyDown(GLFW_KEY_D) || ImGui::IsKeyDown(GLFW_KEY_RIGHT);
            inputs.speedUp = ImGui::IsKeyDown(GLFW_KEY_KP_ADD);
            inputs.speedDown = ImGui::IsKeyDown(GLFW_KEY_KP_SUBTRACT);
            camera.update(ImGui::GetIO().DeltaTime, inputs);
        }

        // Clear buffers
        framebuffer.clear();

        // Setup matrices
        rdrSetUniformFloatV(renderer, UT_CAMERA_POS, camera.position.e);
        rdrSetUniformFloatV(renderer, UT_DELTATIME, &deltaTime);
        rdrSetUniformFloatV(renderer, UT_TIME, &time);

        rdrSetProjection(renderer, camera.getProjection().e);
        rdrSetView(renderer, camera.getViewMatrix().e);

        // Render scene
        scnSetCameraPosition(scene, camera.position.e);
        scnUpdate(scene, deltaTime, renderer);

        // Upload texture
        rdrFinish(renderer);
        framebuffer.updateTexture();


        // Display debug controls
        if (ImGui::Begin("Config"))
        {
            if (ImGui::CollapsingHeader("Framebuffer", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::ColorEdit4("clearColor", framebuffer.clearColor.e);

                if (captureGif)
                {
                    if (ImGui::Button("Stop capture"))
                    {
                        captureGif = false;
                        gifRecorder.end("anim.gif");
                    }
                }
                else
                {
                    if (ImGui::Button("Capture gif"))
                    {
                        gifRecorder.begin();
                        captureGif = true;
                    }
                }
            }

            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                camera.showImGuiControls();
            }
            if (ImGui::CollapsingHeader("renderer.dll", ImGuiTreeNodeFlags_DefaultOpen))
            {
                rdrShowImGuiControls(renderer);
            }
            if (ImGui::CollapsingHeader("scene.dll", ImGuiTreeNodeFlags_DefaultOpen))
            {
                scnShowImGuiControls(scene);
            }
        }
        ImGui::End();

        if (captureGif)
            gifRecorder.frame(framebuffer.getColorBuffer());

        ImDrawList* draw = ImGui::GetForegroundDrawList();
        draw->AddText(ImVec2(10, 10), IM_COL32_WHITE, "(Right click to capture mouse, Esc to un-capture)");

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            mouseCaptured = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        //ImGui::ShowDemoWindow();
        ImGui::ShowMetricsWindow();
        endFrame(window, transitionFramebuffer, framebuffer);
    }

    glDeleteBuffers(1, &transitionFramebuffer);

    scnDestroy(scene);
    rdrShutdown(renderer);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}