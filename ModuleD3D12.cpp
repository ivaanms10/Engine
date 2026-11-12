#include "Globals.h"
#include "ModuleD3D12.h"

//Constructor
ModuleD3D12::ModuleD3D12(HWND hWnd) : hWnd(hWnd)
{
}

//Destructor
ModuleD3D12::~ModuleD3D12()
{
}


bool ModuleD3D12::init()
{
	OutputDebugStringA("ModuleD3D12 init\n");

	#if defined(_DEBUG)
		ComPtr<ID3D12Debug> debugInterface;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
		debugInterface->EnableDebugLayer();
	#endif
		
	//We create the factory.
	#if defined(_DEBUG)
		CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));
	#else
		CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
	#endif

	//We create the adapter.
	factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));

	//We create the device.
	D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));

	#if defined(_DEBUG)
		ComPtr<ID3D12InfoQueue> infoQueue;
		device.As(&infoQueue);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
	#endif

	//We call the methods to create the command queue, swap chain, RTVs, command objects and fence.
	createCommandQueue();
	createSwapChain();
	createRTVs();
	createCommandObjects();
	createFence();

	return true;
}


bool ModuleD3D12::cleanUp()
{
	return true;
}

//Method to create the command queue.
void ModuleD3D12::createCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue));
}