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
    create_surface();
}

void Renderer::deinit()
{
    factory_->Release();
}

void Renderer::draw()
{
    cmd_.list->OMSetRenderTargets(1, &surface_.handle, FALSE, nullptr);

    FLOAT black_bg[4] = { 0.f, 0.f, 0.f, 1.f };
    cmd_.list->ClearRenderTargetView(surface_.handle, black_bg, 0, nullptr);
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

void Renderer::create_surface()
{
    HRESULT err = S_OK;

    surface_.desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    surface_.desc.Width = 1920;
    surface_.desc.Height = 1080;
    surface_.desc.DepthOrArraySize = 1;
    surface_.desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    surface_.desc.SampleDesc.Count = 1;
    surface_.desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_HEAP_PROPERTIES properties = {};
    properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;

    err = device_->CreateCommittedResource(
        &properties, flags,
        &surface_.desc, D3D12_RESOURCE_STATE_RENDER_TARGET,
        nullptr, IID_PPV_ARGS(&surface_.target));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to create render target");

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = 1;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    err = device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&surface_.heap));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to allocate heap");

    surface_.handle = surface_.heap->GetCPUDescriptorHandleForHeapStart();
    device_->CreateRenderTargetView(surface_.target, nullptr, surface_.handle);
}

} // chaos
