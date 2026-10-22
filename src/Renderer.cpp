#include "Renderer.h"

#include "utility/assert.h"
#include <d3d12.h>

namespace chaos {

void Renderer::init()
{
    HRESULT err = S_OK;

    u32 factory_flags = 0;
#ifdef _DEBUG
    factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    err = CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&factory_));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to create IDXGI Factory");

    create_device();
    create_cmd_list();
}

void Renderer::deinit()
{
    factory_->Release();
}

void Renderer::create_device()
{
    HRESULT err = S_OK;

    IDXGIAdapter1 *dummy = nullptr;
    DXGI_ADAPTER_DESC1 desc = {};

    u64 max_memory = 0;
    for (u32 i = 0; factory_->EnumAdapters1(i, &dummy) != DXGI_ERROR_NOT_FOUND; ++i) {
        err = dummy->GetDesc1(&desc);

        if (FAILED(err)) { continue; }

        // skip integrated graphics cards
        if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) { continue; }

        err = D3D12CreateDevice(dummy, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device_));

        // choose the one that supports d3d12 and has the most video memory
        if (SUCCEEDED(err) && max_memory < desc.DedicatedVideoMemory) {
            max_memory = desc.DedicatedVideoMemory;
            adapter_ = dummy;
        }
    }
}

void Renderer::create_cmd_list()
{
    HRESULT err = S_OK;
    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.Type = type;

    err = device_->CreateCommandQueue(&desc, IID_PPV_ARGS(&cmd_.queue));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to create command queue");

    err = device_->CreateCommandAllocator(type, IID_PPV_ARGS(&cmd_.allocator));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to create command allocator");

    err = device_->CreateCommandList(0, type, cmd_.allocator, 0, IID_PPV_ARGS(&cmd_.list));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to create command list");
}

} // chaos
