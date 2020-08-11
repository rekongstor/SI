#include "siImgui.h"
#include "siWindow.h"
#include "siRenderer.h"
#include <imgui.h>
#include <examples/imgui_impl_dx12.h>
#include <examples/imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

siImgui::siImgui()
{
   siWindow::imguiProc = ImGui_ImplWin32_WndProcHandler;
}

void siImgui::onInit()
{
   std::cout << "Initializing ImGUI context" << std::endl;

   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO();
   (void)io;

   ImGui::StyleColorsDark();
}

void siImgui::onInitWindow(HWND window)
{
   std::cout << "Initializing ImGUI window" << std::endl;

   ImGui_ImplWin32_Init(window);
}

void siImgui::onInitRenderer(ID3D12Device* device, uint32_t bufferCount, ID3D12DescriptorHeap* heap,
                             std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> handles,
                             siRenderer* renderer)
{
   std::cout << "Initializing ImGUI renderer" << std::endl;

   ImGui_ImplDX12_Init(device, bufferCount,
                       DXGI_FORMAT_R8G8B8A8_UNORM, heap,
                       handles.first,
                       handles.second);
   this->renderer = renderer;
}

void siImgui::onUpdate()
{
   const char* targets[] = {
      "Color",
      "Diffuse",
      "Position",
      "Normal",
      "Ssao",
      "Metalness",
      "Roughness",
      "Specular",
      "Alt"
   };

   ImGui_ImplDX12_NewFrame();
   ImGui_ImplWin32_NewFrame();
   ImGui::NewFrame();
   {
      ImGui::Begin("Imgui Debug");
      ImGui::DragFloat3("Camera position", &renderer->camera.position.x, 0.01f);
      ImGui::DragFloat3("Camera target", &renderer->camera.target.x, 0.01f);
      ImGui::DragFloat3("Light Color", &renderer->defRenderConstBuffer.get().lightColor.x, 0.1f);
      ImGui::DragFloat3("Ambient Color", &renderer->defRenderConstBuffer.get().ambientColor.x, 0.01f);
      ImGui::DragFloat("Radius", &renderer->defaultSsaoConstBuffer.get().radius, 0.001f);
      ImGui::DragFloat("Bias", &renderer->defaultSsaoConstBuffer.get().bias, 0.001f);
      ImGui::Combo("Target output", &renderer->targetOutput, targets, _countof(targets));
      ImGui::InputInt("Target array", &renderer->targetArray);
      ImGui::InputInt("Target mip", &renderer->targetMip);
      ImGui::InputInt("Target ssao", &renderer->cacaoSsao);
      ImGui::End();
   }
   {
      ImGui::Begin("SI Ssao");
      ImGui::DragFloat4("PS_REG_SSAO_PARAMS", &renderer->siSsaoBuffer.get().PS_REG_SSAO_PARAMS.x, 0.001f);
      ImGui::DragFloat4("SSAO_FRUSTUM_SCALE", &renderer->siSsaoBuffer.get().SSAO_FRUSTUM_SCALE.x, 0.001f);
      ImGui::DragFloat4("SSAO_FRUSTUM_SCALE_FPMODEL", &renderer->siSsaoBuffer.get().SSAO_FRUSTUM_SCALE_FPMODEL.x, 0.001f);
      ImGui::DragFloat4("PS_REG_SSAO_COMMON_PARAMS", &renderer->siSsaoBuffer.get().PS_REG_SSAO_COMMON_PARAMS.x, 0.001f);
      ImGui::InputInt("Blur", &renderer->siSsaoBuffer.get().blur);
      ImGui::End();
   }
   {
      ImGui::Begin("Timings");

      FfxCacaoDetailedTiming timings;
      uint64_t gpuTicksPerMicrosecond;
      renderer->getTimings(&timings, &gpuTicksPerMicrosecond);
      gpuTicksPerMicrosecond /= 1000000;
      for (uint32_t i = 0; i < timings.numTimestamps; ++i)
      {
         FfxCacaoTimestamp* t = &timings.timestamps[i];
         ImGui::Text("%-50s: %7.1f us", t->label, ((double)t->ticks) / ((double)gpuTicksPerMicrosecond));
      }
      ImGui::End();
   }
   {
      ImGui::Begin("Cacao");
      ImGui::DragFloat("radius", &renderer->cacaoSettings.radius, 0.01);
      ImGui::DragFloat("adaptiveQualityLimit", &renderer->cacaoSettings.adaptiveQualityLimit, 0.01);
      ImGui::DragFloat("detailShadowStrength", &renderer->cacaoSettings.detailShadowStrength, 0.01);
      ImGui::DragFloat("fadeOutFrom", &renderer->cacaoSettings.fadeOutFrom, 0.1);
      ImGui::DragFloat("fadeOutTo", &renderer->cacaoSettings.fadeOutTo, 0.1);
      ImGui::DragFloat("horizonAngleThreshold", &renderer->cacaoSettings.horizonAngleThreshold, 0.001);
      ImGui::DragFloat("shadowClamp", &renderer->cacaoSettings.shadowClamp, 0.001);
      ImGui::DragFloat("shadowPower", &renderer->cacaoSettings.shadowPower, 0.01);
      ImGui::DragFloat("shadowMultiplier", &renderer->cacaoSettings.shadowMultiplier, 0.01);
      ImGui::DragFloat("sharpness", &renderer->cacaoSettings.sharpness, 0.01);
      ImGui::DragInt("blurPassCount", reinterpret_cast<int*>(&renderer->cacaoSettings.blurPassCount), 0.1f, 0, 8);
      if (ImGui::Button("Reset"))
      {
         renderer->cacaoSettings = FFX_CACAO_DEFAULT_SETTINGS;
      }
      ImGui::End();
   }
}

void siImgui::onRender(ID3D12GraphicsCommandList* commandList)
{
   ImGui::Render();
   ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}
