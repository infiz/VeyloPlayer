#include "GraphicsDeviceMonitor.h"
#include "GraphicsRecovery.h"

#ifdef Q_OS_WIN
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <algorithm>
#include <tuple>
#include <vector>
#endif

void GraphicsDeviceMonitor::run()
{
#ifdef Q_OS_WIN
    using Microsoft::WRL::ComPtr;
    using AdapterId = std::tuple<UINT, UINT, UINT>;
    std::vector<AdapterId> expectedAdapters;
    std::vector<ComPtr<ID3D11Device>> devices;
    veylo::GraphicsRecovery recovery;
    bool established = false;

    const auto createProbes = [&] {
        ComPtr<IDXGIFactory1> factory;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return false;
        std::vector<AdapterId> found;
        std::vector<ComPtr<ID3D11Device>> candidates;
        for (UINT index = 0; ; ++index) {
            ComPtr<IDXGIAdapter1> adapter;
            const HRESULT result = factory->EnumAdapters1(index, &adapter);
            if (result == DXGI_ERROR_NOT_FOUND) break;
            if (FAILED(result)) return false;
            DXGI_ADAPTER_DESC1 description{};
            if (FAILED(adapter->GetDesc1(&description))) return false;
            if (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
            ComPtr<ID3D11Device> device;
            if (FAILED(D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN,
                    nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                    &device, nullptr, nullptr))) return false;
            found.emplace_back(description.VendorId, description.DeviceId,
                               description.SubSysId);
            candidates.push_back(std::move(device));
        }
        std::sort(found.begin(), found.end());
        // Do not mistake Microsoft's fallback adapter (or the other GPU in a
        // hybrid system) for the card whose driver is still being installed.
        if (found.empty() || (established && found != expectedAdapters)) return false;
        expectedAdapters = std::move(found);
        devices = std::move(candidates);
        return true;
    };

    while (!isInterruptionRequested()) {
        bool healthy = !devices.empty();
        for (const auto &device : devices) {
            if (FAILED(device->GetDeviceRemovedReason())) healthy = false;
        }
        if (!healthy) {
            devices.clear();
            if (established && recovery.sample(false) == veylo::GraphicsRecovery::Lost)
                emit deviceLost();
            healthy = createProbes();
        }
        if (healthy) {
            if (!established) {
                established = true;
            } else if (recovery.sample(true) == veylo::GraphicsRecovery::Restored) {
                emit deviceRestored();
            }
        }
        msleep(500);
    }
#endif
}
