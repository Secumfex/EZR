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

#include "VolumetricLighting.h"

////////////////////// PARAMETERS /////////////////////////////
static bool s_isRotating = false;
static bool s_switchCanvas = false;


const glm::vec2 WINDOW_RESOLUTION = glm::vec2(800.0f, 600.0f);

void setupGLFWCallbacks() {

}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MAIN ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main()
{
    DEBUGLOG->setAutoPrint(true);

    //////////////////////////////////////////////////////////////////////////////
    /////////////////////// INIT //////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    // create window and opengl context
    auto window = generateWindow(WINDOW_RESOLUTION.x,WINDOW_RESOLUTION.y);



    //////////////////////////////////////////////////////////////////////////////
    /////////////////////////////// RENDERING  ///////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    /////////////////////     Scene / View Settings     //////////////////////////
    // set matrices
    glm::mat4 model = glm::mat4(1.0f);
    glm::vec4 cameraStartPos(0.0f, 0.0f, 3.0f, 1.0f);
    glm::vec4 cameraPos(0.0f, 0.0f, 3.0f, 1.0f);
    glm::vec4 cameraTarget(0.0f,0.0f,0.0f,1.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(cameraPos), glm::vec3(cameraTarget), glm::vec3(0,1,0));
    glm::mat4 perspective = glm::perspective(glm::radians(65.f), getRatio(window), 0.1f, 100.0f);
    // setup light
    float lightIntensity = 70000000.0;
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
    int numberOfSamples = 7;
    float mediumDensity = 0.027;       // tau
    float scatterProbability = 0.02f;    // albedo
    // create objects
    float object_size = 0.5f;
    std::vector<Renderable* > objects;
    objects.push_back(new Volume(object_size));



    /////////////////////// 	Renderpass     ///////////////////////////

    ///////////////////////    Frambuffers     ///////////////////////////
    FrameBufferObject::s_internalFormat  = GL_RGBA32F; // to allow arbitrary values in G-Buffer
    FrameBufferObject frameBuffer(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
    frameBuffer.addColorAttachments(1);
    FrameBufferObject debugBuffer(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
    debugBuffer.addColorAttachments(1);
    FrameBufferObject shadowMap(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);
    FrameBufferObject::s_internalFormat  = GL_RGBA;	   // restore default

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
    // renderPass.addEnable(GL_BLEND);
    renderPass.setClearColor(0.0,0.0,0.0,0.0);
    renderPass.addClearBit(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    for (auto r : objects){renderPass.addRenderable(r);}
    DEBUGLOG->outdent();



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



    /////////////////////// 	Raymarching     //////////////////////////
    glm::vec4 cameraPosLightSpace = lightView * cameraPos;
    glm::vec4 cameraViewDirInvLightSpace = glm::normalize(lightView * (cameraPos - cameraTarget));

    VolumetricLighting volumetricLighting(WINDOW_RESOLUTION.x, WINDOW_RESOLUTION.y);

    // uniforms for the vertex shader
    volumetricLighting._raymarchingShader->update("model", model);
    volumetricLighting._raymarchingShader->update("view", view);
    volumetricLighting._raymarchingShader->update("projection", perspective);
    volumetricLighting._raymarchingShader->update("lightView", lightView);
    // uniforms for the fragment shader
    volumetricLighting._raymarchingShader->bindTextureOnUse("shadowMapSampler", shadowMap.getDepthTextureHandle());
    volumetricLighting._raymarchingShader->bindTextureOnUse("worldPosMap", fbo.getBuffer("fragWorldPos"));
    volumetricLighting._raymarchingShader->update("phi", lightIntensity);
    volumetricLighting._raymarchingShader->update("lightProjection", lightProjection);
    volumetricLighting._raymarchingShader->update("lightColor", lightColor);
    volumetricLighting._raymarchingShader->update("cameraPosLightSpace", cameraPosLightSpace);



    /////////////////////// 	Compositing     //////////////////////////
    Quad quad;
    ShaderProgram texShader("/screenSpace/fullscreen.vert", "/screenSpace/simpleAlphaTexture.frag");



    //////////////////////////////////////////////////////////////////////////////
    ///////////////////////    GUI / USER INPUT   ////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////


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

    //////////////////////////////////////////////////////////////////////////////
    //////////////////////////////// RENDER LOOP /////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    double elapsedTime = 0.0;
    render(window, [&](double dt)
    {
        // set window title
        elapsedTime += dt;
        std::string window_header = "Volume Lighting Renderer - " + DebugLog::to_string( 1.0 / dt ) + " FPS";
        glfwSetWindowTitle(window, window_header.c_str() );



        ////////////////////////////////     GUI      ////////////////////////////////
        ImGuiIO& io = ImGui::GetIO();
        ImGui_ImplGlfwGL3_NewFrame(); // tell ImGui a new frame is being rendered
        ImGui::PushItemWidth(-100);

        ImGui::SliderInt("Number ff Samples", &numberOfSamples, 1, 10);
        ImGui::SliderFloat("lightIntensity", &lightIntensity, 100.0f, 100000000.0f);
        ImGui::SliderFloat3("lightPos", glm::value_ptr(lightPos), -10.0f, 10.0f);
        ImGui::SliderFloat("lightAngle", &lightViewAngle, 0.1f, 90.0f);
        ImGui::SliderFloat("lightNearPlane", &lightNearPlane, 0.01f, 1.0f);
        ImGui::SliderFloat("lightFarPlane", &lightFarPlane, 10.0f, 1000.0f);
        ImGui::SliderFloat4("lightColor", glm::value_ptr(lightColor), 0.0f, 1.0f);
        ImGui::SliderFloat("tau", &mediumDensity, 0.0f, 1.0f);
        ImGui::SliderFloat("albedo", &scatterProbability, 0.0f, 1.0f);
        ImGui::Checkbox("auto-rotate", &s_isRotating); // enable/disable rotating volume
        ImGui::PopItemWidth();
        //////////////////////////////////////////////////////////////////////////////



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



        ////////////////////////  SHADER / UNIFORM UPDATING //////////////////////////
        // update shadowmap related uniforms
        shadowMapShader.update("lightMVP", lightMVP);

        // update raymarching related uniforms
        volumetricLighting._raymarchingShader->update("numberOfSamples", numberOfSamples);
        volumetricLighting._raymarchingShader->update("phi", lightIntensity);
        volumetricLighting._raymarchingShader->update("model", model);
        volumetricLighting._raymarchingShader->update("view", view);
        volumetricLighting._raymarchingShader->update("projection", perspective);
        volumetricLighting._raymarchingShader->update("lightView", lightView);
        volumetricLighting._raymarchingShader->update("lightProjection", lightProjection);
        volumetricLighting._raymarchingShader->update("lightColor", lightColor);
        volumetricLighting._raymarchingShader->update("cameraPosLightSpace", cameraPosLightSpace);
        //////////////////////////////////////////////////////////////////////////////



        ////////////////////////////////  RENDERING //////////////////////////////////
        shadowMapRenderpass.render();
        volumetricLighting._raymarchingRenderPass->render();

        // draw fbo in viewport
        RenderPass showVolumeLighting( &texShader, 0 );
        showVolumeLighting.addClearBit(GL_COLOR_BUFFER_BIT);
        showVolumeLighting.addRenderable(&quad);
        texShader.bindTextureOnUse("tex", volumetricLighting._raymarchingFBO->getColorAttachmentTextureHandle(GL_COLOR_ATTACHMENT0));
        showVolumeLighting.render();

        ImGui::Render();
        //////////////////////////////////////////////////////////////////////////////

    });

    destroyWindow(window);

    return 0;
}