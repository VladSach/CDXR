#include "utility/types.h"
#include "utility/assert.h"

#include <windows.h>
#include <dxgi1_6.h>
#include <iostream>

static struct Context {
    IDXGIFactory6 *factory = nullptr;
    IDXGIAdapter1 *adapter = nullptr;
} ctx;

i32 main()
// INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    HRESULT err = S_OK;

    err = CreateDXGIFactory2(0, IID_PPV_ARGS(&ctx.factory));
    ALWAYS_ASSERT(SUCCEEDED(err), "Failed to create IDXGI Factory");

    DXGI_ADAPTER_DESC1 desc = {};
    for (u32 i = 0; ctx.factory->EnumAdapters1(i, &ctx.adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        err = ctx.adapter->GetDesc1(&desc);

        if (FAILED(err)) { continue; }

        std::wcout << L"GPU #" << i + 1 << L":" << L'\n';

        std::wcout << L"Name: "      << desc.Description          << L'\n';
        std::wcout << L"Video Mem: " << desc.DedicatedVideoMemory << L'\n';
        std::wcout << L"Revision: "  << desc.Revision             << L'\n';

        std::wcout << L"===========" << L'\n';

        ctx.adapter->Release();
    }

    // ctx.factory->Release();

    return 0;
}
