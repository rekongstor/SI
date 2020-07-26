#include "siImgui.h"
#include "siWindow.h"
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
                             std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> handles)
{
   std::cout << "Initializing ImGUI renderer" << std::endl;

   ImGui_ImplDX12_Init(device, bufferCount,
                       DXGI_FORMAT_R8G8B8A8_UNORM, heap,
                       handles.first,
                       handles.second);
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
      "Specular"
   };

   ImGui_ImplDX12_NewFrame();
   ImGui_ImplWin32_NewFrame();
   ImGui::NewFrame();
   {
      ImGui::Begin("Imgui Debug");
      ImGui::DragFloat3("Camera position", &camPos->x, 0.1f);
      ImGui::DragFloat3("Camera target", &camTarget->x, 0.1f);
      ImGui::DragFloat3("Light Color", &lightColor->x, 0.1f);
      ImGui::DragFloat3("Ambient Color", &ambientColor->x, 0.01f);
      ImGui::Combo("Target output", targetOutput, targets, _countof(targets));
      ImGui::End();
   }
   {
      ImGui::Begin("Cacao");
      ImGui::DragFloat("radius", &settings->radius, 0.01);
      ImGui::DragFloat("adaptiveQualityLimit", &settings->adaptiveQualityLimit, 0.01);
      ImGui::DragFloat("detailShadowStrength", &settings->detailShadowStrength, 0.01);
      ImGui::DragFloat("fadeOutFrom", &settings->fadeOutFrom, 0.1);
      ImGui::DragFloat("fadeOutTo", &settings->fadeOutTo, 0.1);
      ImGui::DragFloat("horizonAngleThreshold", &settings->horizonAngleThreshold, 0.001);
      ImGui::DragFloat("shadowClamp", &settings->shadowClamp, 0.001);
      ImGui::DragFloat("shadowPower", &settings->shadowPower, 0.01);
      ImGui::DragFloat("shadowMultiplier", &settings->shadowMultiplier, 0.01);
      ImGui::DragFloat("sharpness", &settings->sharpness, 0.01);
      ImGui::End();
   }
}

void siImgui::onRender(ID3D12GraphicsCommandList* commandList)
{
   ImGui::Render();
   ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

void siImgui::bindVariables(float4* cameraPos, float4* cameraTarget, int* targetOutput, float4* lightColor,
                            float4* ambientColor, FfxCacaoSettings* settings)
{
   camPos = cameraPos;
   camTarget = cameraTarget;
   this->targetOutput = targetOutput;
   this->lightColor = lightColor;
   this->ambientColor = ambientColor;
   this->settings = settings;
}
