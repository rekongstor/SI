#include "core_Imgui.h"
#include "core_Window.h"
#include "rnd_Dx12.h"
#include <imgui.h>
#include <examples/imgui_impl_dx12.h>
#include <examples/imgui_impl_win32.h>

#include "SceneConstBuf.h"

extern bool procRunning;
extern LRESULT (*imguiProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void core_Imgui::OnInit()
{
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO();
   (void)io;
   io.AddInputCharacter('w');
   io.AddInputCharacter('a');
   io.AddInputCharacter('s');
   io.AddInputCharacter('d');
   io.KeyRepeatDelay = 0.001f;
   io.KeyRepeatRate = 0.001f;
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

   ImGui::StyleColorsDark();
   imguiProc = ImGui_ImplWin32_WndProcHandler;
}

void core_Imgui::InitWindow()
{
   assert(window != NULL && window->window != NULL);

   ImGui_ImplWin32_Init(window->window);
}

void core_Imgui::InitRender()
{
   auto handle = renderer->GetCbvSrvUavHandle();
   ImGui_ImplDX12_Init(renderer->Device(), FRAME_COUNT,
                       renderer->swapChainFormat, renderer->cbvSrvUavHeap.Get(),
                       handle.first,
                       handle.second);
}

void core_Imgui::OnUpdate()
{
   ImGui_ImplDX12_NewFrame();
   ImGui_ImplWin32_NewFrame();
   ImGui::NewFrame();

   //ImGui::ShowDemoWindow();

#pragma region "Debug window"
   ImGui::Begin("Dist"); // , nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
   ImTextureID id = reinterpret_cast<ImTextureID>(renderer->textureMgr.rayTracingOutputDist.srvHandle.second.ptr);
   ImGui::Image(id, { (float)(RAYS_PER_AXIS * RAYS_PER_AXIS), (float)(RAYS_PER_AXIS * TRAINING_SAMPLES)});
   ImGui::End();

   ImGui::Begin("Camera");
   ImGui::DragFloat3("Position", &renderer->camPos.x, 0.01);
   ImGui::DragFloat3("Light Direction", &renderer->lightDirection.x, 0.01);
   ImGui::SliderFloat("Pitch", &renderer->camDir.x, -M_PI_2, M_PI_2);
   ImGui::DragFloat("Yaw", &renderer->camDir.y, 0.01);
   renderer->camDir.y = fmodf(renderer->camDir.y, 2 * M_PI);
   ImGui::SliderFloat("Fov", &renderer->fovAngleY, 1, 180);
   ImGui::End();

   auto& inst = renderer->scene.instances[&renderer->scene.meshes[4]];

   ImGui::Begin("Inst");
   ImGui::DragFloat4("Position1", inst.instanceData.worldMat[0].m128_f32, 0.01);
   ImGui::DragFloat4("Position2", inst.instanceData.worldMat[1].m128_f32, 0.01);
   ImGui::DragFloat4("Position3", inst.instanceData.worldMat[2].m128_f32, 0.01);
   if (ImGui::Button("Generate training samples"))
   {
      renderer->counter = TRAINING_SAMPLES - 1;
   }
   if (ImGui::Button("Save2File"))
   {
      //SaveD
   }
   ImGui::End();
#pragma endregion
}

void core_Imgui::OnRender()
{
   ImGui::Render();
   ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), renderer->CommandList());
}
