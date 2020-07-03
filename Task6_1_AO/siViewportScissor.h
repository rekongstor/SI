#pragma once
class siViewportScissor
{
   CD3DX12_VIEWPORT viewport;
   CD3DX12_RECT scissorRect;
public:
   siViewportScissor(uint32_t width, uint32_t height);

   [[nodiscard]] const CD3DX12_VIEWPORT& getViewport() const;
   [[nodiscard]] const CD3DX12_RECT& getScissorRect() const;
};
