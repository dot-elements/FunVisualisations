#include "menu.h"
#include "render/renderer.h"
#include <filesystem>
#include <fmt/format.h>
#include <imgui.h>
#include <iostream>
#include <nfd.h>

#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

namespace ui {

Menu::Menu(const glm::ivec2& baseRenderResolution)
    : m_baseRenderResolution(baseRenderResolution)
{
    m_renderConfig.renderResolution = m_baseRenderResolution;
}

void Menu::setLoadVolumeCallback(LoadVolumeCallback&& callback)
{
    m_optLoadVolumeCallback = std::move(callback);
}

void Menu::setRenderConfigChangedCallback(RenderConfigChangedCallback&& callback)
{
    m_optRenderConfigChangedCallback = std::move(callback);
}

void Menu::setInterpolationModeChangedCallback(InterpolationModeChangedCallback&& callback)
{
    m_optInterpolationModeChangedCallback = std::move(callback);
}

render::RenderConfig Menu::renderConfig() const
{
    return m_renderConfig;
}

volume::InterpolationMode Menu::interpolationMode() const
{
    return m_interpolationMode;
}

void Menu::setBaseRenderResolution(const glm::ivec2& baseRenderResolution)
{
    m_baseRenderResolution = baseRenderResolution;
    m_renderConfig.renderResolution = glm::ivec2(glm::vec2(m_baseRenderResolution) * m_resolutionScale);
    callRenderConfigChangedCallback();
}

// This function handles a part of the volume loading where we create the widget histograms, set some config values
//  and set the menu volume information
void Menu::setLoadedVolume(const volume::Volume& volume, const volume::GradientVolume& gradientVolume)
{
    m_tfWidget = TransferFunctionWidget(volume);
    m_tf2DWidget = TransferFunction2DWidget(volume, gradientVolume);

    m_tfWidget->updateRenderConfig(m_renderConfig);
    m_tf2DWidget->updateRenderConfig(m_renderConfig);

    const glm::ivec3 dim = volume.dims();
    m_volumeInfo = fmt::format("Volume info:\n{}\nDimensions: ({}, {}, {})\nVoxel value range: {} - {}\n",
        volume.fileName(), dim.x, dim.y, dim.z, volume.minimum(), volume.maximum());
    m_volumeMax = int(volume.maximum());
    m_volumeLoaded = true;
}

// This function draws the menu
void Menu::drawMenu(const glm::ivec2& pos, const glm::ivec2& size, std::chrono::duration<double> renderTime)
{
    static bool open = 1;
    ImGui::Begin("VolVis", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::SetWindowPos(ImVec2(float(pos.x), float(pos.y)));
    ImGui::SetWindowSize(ImVec2(float(size.x), float(size.y)));

    ImGui::BeginTabBar("VolVisTabs");
    showLoadVolTab();
    if (m_volumeLoaded) {
        const auto renderConfigBefore = m_renderConfig;
        const auto interpolationModeBefore = m_interpolationMode;

        //showRayCastTab(renderTime);
        showCoolWarmShadingTab();
        //showTransFuncTab();
        //show2DTransFuncTab();
        

        if (m_renderConfig != renderConfigBefore)
            callRenderConfigChangedCallback();
        if (m_interpolationMode != interpolationModeBefore)
            callInterpolationModeChangedCallback();
    }

    ImGui::EndTabBar();
    ImGui::End();
}

// This renders the Load Volume tab, which shows a "Load" button and some volume information
void Menu::showLoadVolTab()
{
    if (ImGui::BeginTabItem("Load")) {

        if (ImGui::Button("Load volume")) {
            nfdchar_t* pOutPath = nullptr;
            nfdresult_t result = NFD_OpenDialog("fld", nullptr, &pOutPath);

            if (result == NFD_OKAY) {
                // Convert from char* to std::filesystem::path
                std::filesystem::path path = pOutPath;
                if (m_optLoadVolumeCallback)
                    (*m_optLoadVolumeCallback)(path);
            }
        }

        if (m_volumeLoaded)
            ImGui::Text("%s", m_volumeInfo.c_str());

        ImGui::EndTabItem();
    }
}

// This renders the RayCast tab, where the user can set the render mode, interpolation mode and other
//  render-related settings
void Menu::showRayCastTab(std::chrono::duration<double> renderTime)
{
    if (ImGui::BeginTabItem("Raycaster")) {
        const std::string renderText = fmt::format("rendering time: {}ms\nrendering resolution: ({}, {})\n",
            std::chrono::duration_cast<std::chrono::milliseconds>(renderTime).count(), m_renderConfig.renderResolution.x, m_renderConfig.renderResolution.y);
        ImGui::Text("%s", renderText.c_str());
        ImGui::NewLine();

        int* pRenderModeInt = reinterpret_cast<int*>(&m_renderConfig.renderMode);
        ImGui::Text("Render Mode:");
        ImGui::RadioButton("Slicer", pRenderModeInt, int(render::RenderMode::RenderSlicer));
        ImGui::RadioButton("MIP", pRenderModeInt, int(render::RenderMode::RenderMIP));
        ImGui::RadioButton("IsoSurface Rendering", pRenderModeInt, int(render::RenderMode::RenderIso));
        ImGui::RadioButton("Compositing", pRenderModeInt, int(render::RenderMode::RenderComposite));
        ImGui::RadioButton("2D Transfer Function", pRenderModeInt, int(render::RenderMode::RenderTF2D));

        ImGui::NewLine();

        ImGui::Checkbox("Volume Shading", &m_renderConfig.volumeShading);

        ImGui::NewLine();

        ImGui::DragFloat("Iso Value", &m_renderConfig.isoValue, 0.1f, 0.0f, float(m_volumeMax));

        ImGui::NewLine();

        ImGui::DragFloat("Resolution scale", &m_resolutionScale, 0.0025f, 0.25f, 2.0f);
        m_renderConfig.renderResolution = glm::ivec2(glm::vec2(m_baseRenderResolution) * m_resolutionScale);

        ImGui::NewLine();

        int* pInterpolationModeInt = reinterpret_cast<int*>(&m_interpolationMode);
        ImGui::Text("Interpolation:");
        ImGui::RadioButton("Nearest Neighbour", pInterpolationModeInt, int(volume::InterpolationMode::NearestNeighbour));
        ImGui::RadioButton("Linear", pInterpolationModeInt, int(volume::InterpolationMode::Linear));
        ImGui::RadioButton("TriCubic", pInterpolationModeInt, int(volume::InterpolationMode::Cubic));

        ImGui::EndTabItem();
    }
}
bool isC = 1;
float beta = .6f, y = .4f, a = .2f, b = .4f;
glm::vec3 col1 = { 0.0f, 0.0f, 0.4f };
glm::vec3 col2 = { 0.4f, 0.4f, 0.0f };
void Menu::showCoolWarmShadingTab()
{
    
    if (ImGui::BeginTabItem("Cool to warm shading")) {
        // glm::vec4 color = glm::vec4(0.0f, 0.8f, 0.6f, 0.3f);
        // ImGui::ColorPicker4("Color", glm::value_ptr(color));
        ImGui::Text("Cool to warm palette:");
        ImGui::NewLine();
        ImGui::Checkbox("Use Blue and Yellow scaling", &isC);
        ImGui::NewLine();

        if (isC) {
            ImGui::DragFloat("Yellow", &y, 0.1f, 0.0f, 1.f);
            ImGui::NewLine();
            ImGui::DragFloat("Blue", &b, 0.1f, 0.0f, 1.f);
            ImGui::NewLine();
            m_renderConfig.coolColor = { 0.0f, 0.0f, b };
            m_renderConfig.warmColor = { y, y, 0.f };
        }
        else {
            ImGui::ColorEdit3("Cool color", value_ptr(col1));
            ImGui::NewLine();
            ImGui::ColorEdit3("Warm color", value_ptr(col2));
            m_renderConfig.coolColor = col1;
            m_renderConfig.warmColor = col2;
        }

        ImGui::NewLine();
        ImGui::ColorEdit3("Base color", glm::value_ptr(m_renderConfig.modelColor));
        int* pRenderModeInt = reinterpret_cast<int*>(&m_renderConfig.renderMode);
        m_renderConfig.renderResolution = glm::ivec2(glm::vec2(m_baseRenderResolution) * m_resolutionScale);
        ImGui::NewLine();

        ImGui::DragFloat("Alpha", &m_renderConfig.alphaColor, 0.1f, 0.0f, 1.f);
        ImGui::NewLine();
        ImGui::DragFloat("Beta", &m_renderConfig.betaColor, 0.1f, 0.0f, 1.f);
        ImGui::NewLine();


        ImGui::DragFloat("Iso Value", &m_renderConfig.isoValue, 0.1f, 0.0f, float(m_volumeMax));
        ImGui::NewLine();

        ImGui::Text("Render Mode:");
        ImGui::RadioButton("Normal Rendering", pRenderModeInt, int(render::RenderMode::RenderIso));
        ImGui::RadioButton("Cool to Warm Rendering", pRenderModeInt, int(render::RenderMode::RenderIsoCartoon));
        ImGui::EndTabItem();
    }
}

// This renders the 1D Transfer Function Widget.
void Menu::showTransFuncTab()
{
    if (ImGui::BeginTabItem("Transfer function")) {
        m_tfWidget->draw();
        m_tfWidget->updateRenderConfig(m_renderConfig);
        ImGui::EndTabItem();
    }
}

// This renders the 2D Transfer Function Widget.
void Menu::show2DTransFuncTab()
{
    if (ImGui::BeginTabItem("2D transfer function")) {
        m_tf2DWidget->draw();
        m_tf2DWidget->updateRenderConfig(m_renderConfig);
        ImGui::EndTabItem();
    }
}

void Menu::callRenderConfigChangedCallback() const
{
    if (m_optRenderConfigChangedCallback)
        (*m_optRenderConfigChangedCallback)(m_renderConfig);
}

void Menu::callInterpolationModeChangedCallback() const
{
    if (m_optInterpolationModeChangedCallback)
        (*m_optInterpolationModeChangedCallback)(m_interpolationMode);
}

}
