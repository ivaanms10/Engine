#pragma once

#include "Module.h"

class ModuleD3D12 : public Module
{
public:
	ModuleD3D12(HWND hWnd);
	~ModuleD3D12() override;

	virtual bool init() override;
	virtual bool cleanUp() override;

	//Method for Exercise 1. 
	virtual void preRender() override;
	virtual void render() override;
	virtual void postRender() override;
	void createCommandQueue();
	void createSwapChain();
	void createRTVs();
	void createCommandObjects();
	void createFence();
	void waitForFence(UINT64 value);
	void waitForGPU();

private: 
	ComPtr<IDXGIFactory6> factory;
	ComPtr<IDXGIAdapter4> adapter;
	ComPtr<ID3D12Device5> device;

	static const UINT FRAME_COUNT = 2;
	HWND hWnd = nullptr;
	ComPtr<ID3D12CommandQueue>        commandQueue;
	ComPtr<IDXGISwapChain3>           swapChain;
	ComPtr<ID3D12DescriptorHeap>      rtvHeap;
	ComPtr<ID3D12Resource>            backBuffers[FRAME_COUNT];
	ComPtr<ID3D12CommandAllocator>    allocators[FRAME_COUNT];
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12Fence>               fence;

	HANDLE fenceEvent = nullptr;
	UINT64 fenceValue = 0;
	UINT64 fenceValues[FRAME_COUNT] = {};
	UINT   rtvSize = 0;
	UINT   currentIndex = 0;
};

