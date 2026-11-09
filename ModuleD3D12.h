#pragma once

#include "Module.h"

class ModuleD3D12 : public Module
{
public:
	virtual bool init() override;
	virtual bool cleanUp() override;

private: 
	ComPtr<IDXGIFactory6> factory;
	ComPtr<IDXGIAdapter4> adapter;
	ComPtr<ID3D12Device5> device;
};

