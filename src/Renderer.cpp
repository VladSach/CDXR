#include "Renderer.h"

#include "utility/assert.h"
#include <d3d12.h>

#include <string>

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
    create_fence();
}

void Renderer::deinit()
{
    CloseHandle(fence_.handle);
    fence_.fence->Release();
    readback_->Release();
    surface_.heap->Release();
    surface_.target->Release();
    cmd_.queue->Release();
    cmd_.allocator->Release();
    cmd_.list->Release();
    device_->Release();
    adapter_->Release();
    factory_->Release();
}

void Renderer::draw()
{
    HRESULT err = S_OK;

    cmd_.list->OMSetRenderTargets(1, &surface_.handle, FALSE, nullptr);

    FLOAT bg[4] = { clear_color_, clear_color_, clear_color_, 1.f };
    cmd_.list->ClearRenderTargetView(surface_.handle, bg, 0, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Transition.pResource = surface_.target;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

    cmd_.list->ResourceBarrier(1, &barrier);

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = surface_.target;
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = readback_;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dst.PlacedFootprint = rtfootprint_;

    cmd_.list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    cmd_.list->ResourceBarrier(1, &barrier);

    cmd_.list->Close();

    ID3D12CommandList *cmd_lists[] = { cmd_.list };
    cmd_.queue->ExecuteCommandLists(1, cmd_lists);
    cmd_.queue->Signal(fence_.fence, fence_.value);

    if (fence_.fence->GetCompletedValue() < fence_.value) {
        err = fence_.fence->SetEventOnCompletion(fence_.value, fence_.handle);
        ALWAYS_ASSERT(SUCCEEDED(err), "Failed to create fence signal");

        WaitForSingleObject(fence_.handle, INFINITE);
    }

    write_to_file();

    err = cmd_.allocator->Reset();
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to reset cmd allocator");

    err = cmd_.list->Reset(cmd_.allocator, nullptr);
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to reset cmd list");
}

void Renderer::set_clear_color(f32 color)
{
    clear_color_ = color;
}

void Renderer::write_to_file()
{
    HRESULT err = S_OK;

    void *data = nullptr;
    err = readback_->Map(0, nullptr, &data);
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to map readback");

    std::string ppm = {};

    // Header
    ppm = "P3\n"
        + std::to_string(surface_.desc.Width)  + ' '
        + std::to_string(surface_.desc.Height) + '\n'
        + "255" + '\n';

    for (u32 i = 0; i < surface_.desc.Height; ++i) {
        u8 *row = reinterpret_cast<u8*>(data) + i * rtfootprint_.Footprint.RowPitch;
        for (u32 j = 0; j < surface_.desc.Width; ++j) {
            u8 *pixel = row + j * 4;

            ppm += std::to_string(pixel[0]) + ' '
                +  std::to_string(pixel[1]) + ' '
                +  std::to_string(pixel[2]) + '\t';
        }
        ppm += '\n';
    }

    readback_->Unmap(0, nullptr);

    // write file
    static u32 frame = 0;
    std::string filename = "frame#" + std::to_string(frame++) + ".ppm";
    std::FILE *f = nullptr;
    fopen_s(&f, filename.c_str(), "wb");
    ALWAYS_ASSERT(f, "Couldn't create/open:", filename);

    fwrite(ppm.data(), sizeof(char), ppm.size(), f);

    std::fclose(f);
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

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = 1;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    err = device_->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&surface_.heap));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to allocate heap");

    surface_.handle = surface_.heap->GetCPUDescriptorHandleForHeapStart();
    device_->CreateRenderTargetView(surface_.target, nullptr, surface_.handle);

    // create readback
    UINT64 readback_size = 0;
    device_->GetCopyableFootprints(
        &surface_.desc, 0, 1, 0, &rtfootprint_,
        nullptr, nullptr, &readback_size);

    D3D12_HEAP_PROPERTIES heap_props = {};
    heap_props.Type = D3D12_HEAP_TYPE_READBACK;

    D3D12_RESOURCE_DESC readback_desc = {};
    readback_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    readback_desc.Width = readback_size;
    readback_desc.Height = 1;
    readback_desc.DepthOrArraySize = 1;
    readback_desc.MipLevels = 1;
    readback_desc.SampleDesc.Count = 1;
    readback_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    err = device_->CreateCommittedResource(
        &heap_props, D3D12_HEAP_FLAG_NONE,
        &readback_desc, D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS(&readback_));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to create readback buffer");
}

void Renderer::create_fence()
{
    HRESULT err = S_OK;

    err = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_.fence));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to create fence");

    fence_.handle = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    ALWAYS_ASSERT(fence_.handle, "Failed to create fence event");
}

} // chaos
