#ifndef CHAOS_RENDERER_H
#define CHAOS_RENDERER_H

#include "utility/types.h"

#include <windows.h>
#include <dxgi1_6.h>
#include <d3d12.h>

namespace chaos {

class Renderer {
public:
    void init();
    void deinit();

private:
    void create_device();
    void create_cmd_list();

private:
    IDXGIFactory6 *factory_ = nullptr;
    IDXGIAdapter1 *adapter_ = nullptr;

    ID3D12Device *device_ = nullptr;

    struct CommandQueue {
        ID3D12CommandQueue *queue = nullptr;
        ID3D12CommandAllocator *allocator = nullptr;
        ID3D12GraphicsCommandList *list = nullptr;
    } cmd_;
};

} // chaos

#endif // CHAOS_RENDERER_H
