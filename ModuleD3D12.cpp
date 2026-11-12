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

//Method to create the swap chain.
void ModuleD3D12::createSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	desc.Width = 800;
	desc.Height = 600;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Stereo = FALSE;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = FRAME_COUNT;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = 0;

	ComPtr<IDXGISwapChain1> swapChain1;
	factory->CreateSwapChainForHwnd(commandQueue.Get(), hWnd, &desc, nullptr, nullptr, &swapChain1);
	swapChain1.As(&swapChain);
}

//Method to create the RTVs.
void ModuleD3D12::createRTVs()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = FRAME_COUNT;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&rtvHeap));
	rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	
	for (UINT i = 0; i < FRAME_COUNT; ++i){
		swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), i, rtvSize);
		device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, handle);
	}
}

//Method to create the command objects.
void ModuleD3D12::createCommandObjects()
{
	for (UINT i = 0; i < FRAME_COUNT; ++i){
		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocators[i]));
	}
	device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList));
}

//Method to create the fence.
void ModuleD3D12::createFence()
{
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

//Method to wait for a specific fence value.
void ModuleD3D12::waitForFence(UINT64 value)
{
	if (fence->GetCompletedValue() < value){
		fence->SetEventOnCompletion(value, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

//Method to wait for the GPU to finish its work.
void ModuleD3D12::waitForGPU()
{
	commandQueue->Signal(fence.Get(), ++fenceValue);
	waitForFence(fenceValue);
}

//Method to prepare the command list for rendering.
void ModuleD3D12::preRender()
{
	currentIndex = swapChain->GetCurrentBackBufferIndex();
	waitForFence(fenceValues[currentIndex]);
	allocators[currentIndex]->Reset();
}

//Method to record the commands for rendering.
void ModuleD3D12::render()
{
	commandList->Reset(allocators[currentIndex].Get(), nullptr);

	auto toRT = CD3DX12_RESOURCE_BARRIER::Transition(backBuffers[currentIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &toRT);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentIndex, rtvSize);
	float red[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	commandList->ClearRenderTargetView(rtv, red, 0, nullptr);

	auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(backBuffers[currentIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &toPresent);

	commandList->Close();
	ID3D12CommandList* lists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, lists);

	swapChain->Present(1, 0);
}

//Method to present the rendered frame and signal the fence.
void ModuleD3D12::postRender()
{
	commandQueue->Signal(fence.Get(), ++fenceValue);
	fenceValues[currentIndex] = fenceValue;
}