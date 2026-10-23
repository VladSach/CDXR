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

    void draw();

private:
    void create_device();
    void create_cmd_list();
    void create_surface();

private:
    IDXGIFactory6 *factory_ = nullptr;
    IDXGIAdapter1 *adapter_ = nullptr;

    ID3D12Device *device_ = nullptr;

    struct CommandQueue {
        ID3D12CommandQueue *queue = nullptr;
        ID3D12CommandAllocator *allocator = nullptr;
        ID3D12GraphicsCommandList *list = nullptr;
    } cmd_;

    struct Surface {
        D3D12_RESOURCE_DESC desc = {};
        ID3D12Resource *target = nullptr;

        ID3D12DescriptorHeap *heap = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
    } surface_;
};

} // chaos

#endif // CHAOS_RENDERER_H
