#include "RendererAO.h"
#include "../Task5_1_DirectX12/Window.h"

void RendererAO::onInit()
{
   device->OnCreate("SI_AO", "SI_AO", true, window->getWindow());
}

void RendererAO::onUpdate()
{

}

void RendererAO::onDestroy()
{
}

RendererAO::RendererAO(Window* window): window(window)
{
}
