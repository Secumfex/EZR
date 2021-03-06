/*******************************************
 * **** DESCRIPTION ****
 ****************************************/

#include <iostream>

#include <Rendering/GLTools.h>
#include <Rendering/VertexArrayObjects.h>
#include <Rendering/RenderPass.h>

#include "UI/imgui/imgui.h"
#include <UI/imguiTools.h>
#include <UI/Turntable.h>

#include <Importing/TextureTools.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Rendering/PostProcessing.h>

#include "VolumetricLighting.h"

////////////////////// PARAMETERS /////////////////////////////
static bool s_isRotating = false;
static bool s_switchCanvas = false;


const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);
const glm::vec2 SHADOWMAP_RESOLUTION = glm::vec2(800.0f, 600.0f);
//const glm::vec2 SHADOWMAP_RESOLUTION = glm::vec2(1024.0f, 768.0f);

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main()
{
    DEBUGLOG->setAutoPrint(true);

    //////////////////////////////////////////////////////////////////////////////
    /////////////////////// INIT //////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    //<editor-fold desc="init glfw window">
    // create window and opengl context
    auto window = generateWindow(WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);
    //</editor-fold>



    //////////////////////////////////////////////////////////////////////////////
    /////////////////////////////// RENDERING  ///////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    //<editor-fold desc="init rendering">
    //<editor-fold desc="setup variables">
    /////////////////////     Scene / View Settings     //////////////////////////
    // set matrices
    glm::mat4 model = glm::mat4(1.0f);
    glm::vec4 cameraStartPos(0.0f, 0.0f, 3.0f, 1.0f);
    glm::vec4 cameraPos(0.0f, 0.0f, 3.0f, 1.0f);
    glm::vec4 cameraTarget(0.0f,0.0f,0.0f,1.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(cameraPos), glm::vec3(cameraTarget), glm::vec3(0,1,0));
    glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 100.0f);

    // setup light
    float lightIntensity = 10000.0;
    glm::vec4 lightColor(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos(0.0f, 0.0f, 3.0f);
    glm::vec3 lightTarget(0,0,0);
    glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0,1,0));
    float lightViewAngle = 30.0f;
    float lightNearPlane = 1.0f;
    float lightFarPlane = 10.0f;
    glm::mat4 lightProjection = glm::perspective(glm::radians(lightViewAngle), getRatio(window), lightNearPlane, lightFarPlane);
    glm::mat4 lightMVP = lightProjection * lightView * model;

    // setup variables for raymarching
    float scatterProbability = 0.02f;    // albedo

    // create objects
    float object_size = 0.5f;
    std::vector<Renderable* > objects;
    objects.push_back(new Volume(object_size));
    objects.push_back(new Sphere(20, 40, 80));

    // mip mapping
    int maxLevel = 4;

    //</editor-fold>


    /////////////////////// 	Renderpass     ///////////////////////////

    //<editor-fold desc="setup framebuffer">
    ///////////////////////    Frambuffers     ///////////////////////////
    checkGLError(true);
    FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
    FrameBufferObject shadowMap(SHADOWMAP_RESOLUTION.x, SHADOWMAP_RESOLUTION.y);
    FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default
    //</editor-fold>

    //<editor-fold desc="setup gbuffer">
    // regular GBuffer
    DEBUGLOG->log("Shader Compilation: GBuffer"); DEBUGLOG->indent();
    ShaderProgram shaderProgram("/modelSpace/GBuffer.vert", "/modelSpace/GBuffer.frag"); DEBUGLOG->outdent();
    shaderProgram.update("model", model);
    shaderProgram.update("view", view);
    shaderProgram.update("projection", perspective);
    DEBUGLOG->outdent();

    DEBUGLOG->log("FrameBufferObject Creation: GBuffer"); DEBUGLOG->indent();
    FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
    FrameBufferObject fbo(shaderProgram.getOutputInfoMap(), getResolution(window).x, getResolution(window).y);
    FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default
    DEBUGLOG->outdent();

    DEBUGLOG->log("RenderPass Creation: GBuffer"); DEBUGLOG->indent();
    RenderPass renderPass(&shaderProgram, &fbo);
    renderPass.addEnable(GL_DEPTH_TEST);
    renderPass.setClearColor(0.0,0.0,0.0,0.0);
    renderPass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    for (auto r : objects){renderPass.addRenderable(r);}
    DEBUGLOG->outdent();
    //</editor-fold>

    //<editor-fold desc="setup shadowmapping">
    ///////////////////////    Shadowmapping   ///////////////////////////
    // load and update shadowmap shader
    DEBUGLOG->log("Shader Compilation: shadowmap shader"); DEBUGLOG->indent();
    ShaderProgram shadowMapShader("/vml/shadowmap.vert", "/vml/shadowmap.frag");
    shadowMapShader.update("lightMVP", lightMVP);
    DEBUGLOG->outdent();
    // setup shadowmap renderpass
    DEBUGLOG->log("Renderpass Creation: simple renderpass"); DEBUGLOG->indent();
    RenderPass shadowMapRenderpass(&shadowMapShader, &shadowMap);
    shadowMapRenderpass.setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    shadowMapRenderpass.addClearBit(GL_DEPTH_BUFFER_BIT);
    shadowMapRenderpass.addEnable(GL_DEPTH_TEST);
    DEBUGLOG->outdent();
    // add objects to shadowMap render pass
    for (auto r : objects )
    {
        shadowMapRenderpass.addRenderable(r);
    }
    //</editor-fold>

    //<editor-fold desc="setup raymarching">
    /////////////////////// 	Raymarching     //////////////////////////
    glm::vec4 cameraPosLightSpace = lightView * cameraPos;
    glm::vec4 cameraViewDirInvLightSpace = glm::normalize(lightView * (cameraPos - cameraTarget));

    VolumetricLighting volumetricLighting(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
    volumetricLighting._raymarchingShader->bindTextureOnUse("shadowMapSampler", shadowMap.getDepthTextureHandle());
    volumetricLighting._raymarchingShader->bindTextureOnUse("worldPosMap", fbo.getBuffer("fragWorldPos"));
    volumetricLighting.setupNoiseTexture();
    //</editor-fold>


    /////////////////////// 	Compositing     //////////////////////////

    //<editor-fold desc="tex shader">
    Quad quad;
    ShaderProgram texShader("/screenSpace/fullscreen.vert", "/screenSpace/simpleAlphaTexture.frag");
    //</editor-fold>
    //</editor-fold>



    //////////////////////////////////////////////////////////////////////////////
    ///////////////////////    GUI / USER INPUT   ////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    //<editor-fold desc="setup glfw callbacks">
    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, true);

    Turntable turntable;
    double old_x;
    double old_y;
    glfwGetCursorPos(window, &old_x, &old_y);

    auto cursorPosCB = [&](double x, double y)
    {
        ImGuiIO& io = ImGui::GetIO();
        if ( io.WantCaptureMouse )
        { return; } // ImGUI is handling this

        double d_x = x - old_x;
        double d_y = y - old_y;

        if ( turntable.getDragActive() )
        {
            turntable.dragBy(d_x, d_y, view);
        }

        old_x = x;
        old_y = y;
    };

    auto mouseButtonCB = [&](int b, int a, int m)
    {
        if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_PRESS)
        {
            turntable.setDragActive(true);
        }
        if (b == GLFW_MOUSE_BUTTON_LEFT && a == GLFW_RELEASE)
        {
            turntable.setDragActive(false);
        }

        ImGui_ImplGlfwGL3_MouseButtonCallback(window, b, a, m);
    };

    auto mouseScrollCB = [&](double xOffset, double yOffset)
    {
        cameraStartPos.z += (float) yOffset;

        //ImGui_ImplGlfwGL3_ScrollCallback(window, xOffset, yOffset);
    };

    auto keyboardCB = [&](int k, int s, int a, int m)
    {
        if (a == GLFW_RELEASE) {return;}
        switch (k)
        {
            case GLFW_KEY_W:
                cameraPos += glm::inverse(view)    * glm::vec4(0.0f,0.0f,-0.1f,0.0f);
                cameraTarget += glm::inverse(view) * glm::vec4(0.0f,0.0f,-0.1f,0.0f);
                break;
            case GLFW_KEY_A:
                cameraPos += glm::inverse(view)	 * glm::vec4(-0.1f,0.0f,0.0f,0.0f);
                cameraTarget += glm::inverse(view) * glm::vec4(-0.1f,0.0f,0.0f,0.0f);
                break;
            case GLFW_KEY_S:
                cameraPos += glm::inverse(view)    * glm::vec4(0.0f,0.0f,0.1f,0.0f);
                cameraTarget += glm::inverse(view) * glm::vec4(0.0f,0.0f,0.1f,0.0f);
                break;
            case GLFW_KEY_D:
                cameraPos += glm::inverse(view)    * glm::vec4(0.1f,0.0f,0.0f,0.0f);
                cameraTarget += glm::inverse(view) * glm::vec4(0.1f,0.0f,0.0f,0.0f);
                break;
            case GLFW_KEY_C:
                s_switchCanvas = !s_switchCanvas;
                break;
            default:
                break;
        }
        ImGui_ImplGlfwGL3_KeyCallback(window,k,s,a,m);
    };

    setScrollCallback(window, mouseScrollCB);
    setCursorPosCallback(window, cursorPosCB);
    setMouseButtonCallback(window, mouseButtonCB);
    setKeyCallback(window, keyboardCB);
    //</editor-fold>



    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////// RENDER LOOP /////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    //<editor-fold desc="render loop">
    double elapsedTime = 0.0;
    render(window, [&](double dt)
    {
        //<editor-fold desc="update window title">
        // set window title
        elapsedTime += dt;
        std::string window_header = "Volume Lighting Renderer - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
        glfwSetWindowTitle(window, window_header.c_str() );
        //</editor-fold>

        //<editor-fold desc="imgui update">
        ////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
        ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
        ImGui::PushItemWidth(-100);

        ImGui::SliderFloat3("lightPos", glm::value_ptr(lightPos), -10.0f, 10.0f);
        ImGui::SliderFloat("lightAngle", &lightViewAngle, 0.1f, 90.0f);
        ImGui::SliderFloat("lightNearPlane", &lightNearPlane, 0.01f, 1.0f);
        ImGui::SliderFloat("lightFarPlane", &lightFarPlane, 10.0f, 1000.0f);
        ImGui::SliderFloat4("lightColor", glm::value_ptr(lightColor), 0.0f, 1.0f);
        ImGui::SliderFloat("albedo", &scatterProbability, 0.0f, 4.0f);
        ImGui::Checkbox("auto-rotate", &s_isRotating); // enable/disable rotating volume
        ImGui::PopItemWidth();
        //////////////////////////////////////////////////////////////////////////////
        //</editor-fold>

        //<editor-fold desc="update variables">
        ///////////////////////////// MATRIX UPDATING ///////////////////////////////
        if (s_isRotating) // update view matrix
        {
            model = glm::rotate(glm::mat4(1.0f), (float) dt, glm::vec3(0.0f, 1.0f, 0.0f) ) * model;
        }

        // update camera
        cameraPos = turntable.getRotationMatrix() * cameraStartPos;
        view = glm::lookAt(glm::vec3(cameraPos), glm::vec3(cameraTarget), glm::vec3(0.0f, 1.0f, 0.0f));

        // update the lightsource
        lightView = glm::lookAt(glm::vec3(lightPos), glm::vec3(lightTarget), glm::vec3(0,1,0));
        lightProjection = glm::perspective(glm::radians(lightViewAngle), getRatio(window), lightNearPlane, lightFarPlane);
        lightMVP = lightProjection * lightView * model;
        cameraPosLightSpace = lightView * cameraPos;
        cameraViewDirInvLightSpace = glm::normalize(lightView * (cameraPos - cameraTarget));

        //////////////////////////////////////////////////////////////////////////////
        //</editor-fold>

        //<editor-fold desc="update shader uniforms">
        ////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
        // update shadowmap related uniforms
        shaderProgram.update("model", model);
        shaderProgram.update("view", view);
        shaderProgram.update("projection", perspective);
        shadowMapShader.update("lightMVP", lightMVP);

        // update raymarching related uniforms
        volumetricLighting._raymarchingShader->update("lightView", lightView);
        volumetricLighting._raymarchingShader->update("lightProjection", lightProjection);
        volumetricLighting._raymarchingShader->update("lightColor", lightColor);
        volumetricLighting._raymarchingShader->update("cameraPosLightSpace", cameraPosLightSpace);
        volumetricLighting._raymarchingShader->update("albedo", scatterProbability);
        //////////////////////////////////////////////////////////////////////////////
        //</editor-fold>

        //<editor-fold desc="rendering">
        ////////////////////////////////  RENDERING //////////////////////////////////
        renderPass.render();
        shadowMapRenderpass.render();
        volumetricLighting._raymarchingRenderPass->render();

        // draw fbo in viewport
        RenderPass showVolumeLighting( &texShader, 0 );
        showVolumeLighting.addClearBit(GL_COLOR_BUFFER_BIT);
        showVolumeLighting.addRenderable(&quad);
        texShader.bindTextureOnUse("tex", volumetricLighting._raymarchingFBO->getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
        showVolumeLighting.render();

        // draw depthmap
        RenderPass showDepth( &texShader, 0 );
        showDepth.setViewport(0, 0, 150, 150);
        showDepth.addRenderable(&quad);
        texShader.bindTextureOnUse("tex", shadowMap.getDepthTextureHandle());
        showDepth.render();

        OPENGLCONTEXT->setViewport(0,0,WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
        ImGui::Render();
        //////////////////////////////////////////////////////////////////////////////
        //</editor-fold>

    });
    //</editor-fold>

    destroyWindow(window);

    return 0;
}