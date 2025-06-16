#include "gpu.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <comdef.h>

#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <filesystem>
#include <fstream>
#include <string>
#include <exception>

#define DISCARD (void)

// gpu.h
// (c) 2025 Connor J. Link. All Rights Reserved.

namespace
{
	std::string read_file(const char* filepath)
	{
		const auto path = std::filesystem::path(filepath);

		std::ifstream file(path);
		if (!file.good())
			throw std::runtime_error(std::format("the file `{}` could not be opened for reading", filepath));

		const auto filesize = std::filesystem::file_size(path);

		std::string contents(filesize, '\0');
		contents.assign(
			std::istreambuf_iterator<char>{ file },
			std::istreambuf_iterator<char>{});
		return contents;
	}

	inline std::string tchar_to_string(const TCHAR* message)
	{
#ifdef UNICODE
		int size_needed = WideCharToMultiByte(
			CP_UTF8, 0, message, -1, nullptr, 0, nullptr, nullptr);

		if (size_needed <= 0)
			return {};

		std::string result(size_needed - 1, 0);

		WideCharToMultiByte(
			CP_UTF8, 0, message, -1, result.data(), size_needed, nullptr, nullptr);

		return result;
#else
		return std::string(message ? message : "");
#endif
	}

	std::unique_ptr<ID3DBlob> compile_shader(const char* filepath, const char* shader_revision)
	{
		const auto source = ::read_file(filepath);

		std::unique_ptr<ID3DBlob> blob{};
		const auto result = D3DCompile(
			source.c_str(), source.length(), nullptr, nullptr, nullptr, "main", shader_revision, 0, 0, std::out_ptr(blob), nullptr);

		if (FAILED(result))
		{
			_com_error error(result);
			const auto message = tchar_to_string(error.ErrorMessage());

			throw std::runtime_error(
				std::format("failed to create to compile shader `{}` because: `{}`", filepath, message));
		}

		return blob;
	}

	struct Dimensions
	{
		UINT width, height;
	};

	Dimensions get_dimensions(HWND hwnd)
	{
		Dimensions dimensions{};

		RECT rect{};
		// it's okay if this fails since we will just fall back to returning 0
		DISCARD GetClientRect(hwnd, &rect);
		dimensions.width = rect.right - rect.left;
		dimensions.height = rect.bottom - rect.top;

		return dimensions;
	}
}

namespace luma
{
	GPU* _gpu;

	LRESULT CALLBACK GPU::WindowHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch (msg)
		{
		case WM_CLOSE:
			DestroyWindow(hwnd);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_SIZE:
			if (_gpu != nullptr)
				_gpu->refresh_rendertargetview();
			return 0;

		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
	}

	void GPU::create_window(HINSTANCE instance, int width, int height, int cmdshow)
	{
		static constexpr auto WINDOWNAME = L"Luma DirectX Raytracer";
		static constexpr auto CLASSNAME = L"luma-directx-raytracer";

		WNDCLASSEX wcex{};
		wcex.lpfnWndProc = &WindowHandler;
		wcex.lpszClassName = CLASSNAME;
		wcex.hInstance = instance;
		wcex.hCursor = LoadCursor(instance, IDC_ARROW);
		wcex.hIcon = LoadIcon(instance, IDI_APPLICATION);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.cbSize = sizeof(wcex);

		const auto result = RegisterClassEx(&wcex);
		if (result == NULL)
			throw std::runtime_error("failed to register the window class");

		_hwnd = CreateWindowEx(
			NULL, CLASSNAME, WINDOWNAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, instance, NULL);
		if (_hwnd == NULL)
			throw std::runtime_error("failed to create the window");
	
		// try to show the window as requested, but don't care if it fails
		DISCARD ShowWindow(_hwnd, cmdshow);
	}

	void GPU::create_device(void)
	{
		D3D_FEATURE_LEVEL feature_level{};

		HRESULT hr = D3D11CreateDevice(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
			nullptr, 0, D3D11_SDK_VERSION,
			std::out_ptr(_device), &feature_level, std::out_ptr(_context));

		if (FAILED(hr))
			throw std::runtime_error("failed to create the DirectX device");
	}

	void GPU::create_swapchain(void)
	{
		const auto dimensions = ::get_dimensions(_hwnd);

		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferCount = 2; // Doble buffer
		desc.BufferDesc.Width = dimensions.width;
		desc.BufferDesc.Height = dimensions.height;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.RefreshRate.Numerator = 60;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.OutputWindow = _hwnd;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Windowed = TRUE;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.Flags = 0;

		IDXGIDevice* dxgi_device;
		HRESULT hr = _device->QueryInterface(IID_PPV_ARGS(&dxgi_device));
		if (FAILED(hr))
			throw std::runtime_error("failed to get IDXGIDevice from device");

		IDXGIAdapter* dxgi_adapter;
		hr = dxgi_device->GetAdapter(&dxgi_adapter);
		if (FAILED(hr))
			throw std::runtime_error("failed to get IDXGIAdapter from device");

		IDXGIFactory* dxgi_factory;
		hr = dxgi_adapter->GetParent(IID_PPV_ARGS(&dxgi_factory));
		if (FAILED(hr))
			throw std::runtime_error("failed to get IDXGIFactory from adapter");

		hr = dxgi_factory->CreateSwapChain(_device.get(), &desc, std::out_ptr(_swapchain));
		if (FAILED(hr))
			throw std::runtime_error("failed to create swapchain");
	}

	void GPU::create_rendertargetview(void)
	{
		// double buffering
		ID3D11Texture2D* back_buffer = nullptr;
		HRESULT hr = _swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
		if (FAILED(hr))
			throw std::runtime_error("failed to obtain the swapchain back buffer");

		hr = _device->CreateRenderTargetView(
			back_buffer, nullptr, std::out_ptr(_render_target_view));

		if (FAILED(hr))
			throw std::runtime_error("failed to create the render target view");
	}

	void GPU::create_vertexshader(const char* filepath)
	{
		_vs_blob = ::compile_shader(filepath, "vs_5_0");
		_device->CreateVertexShader(
			_vs_blob->GetBufferPointer(), _vs_blob->GetBufferSize(), nullptr, std::out_ptr(_vertex_shader));
	}

	void GPU::create_pixelshader(const char* filepath)
	{
		_ps_blob = ::compile_shader(filepath, "ps_5_0");
		_device->CreatePixelShader(
			_ps_blob->GetBufferPointer(), _ps_blob->GetBufferSize(), nullptr, std::out_ptr(_pixel_shader));
	}

	void GPU::setup_fullscreentriangle(void)
	{
		struct Vertex { float pos[2]; };

		Vertex triangle[] = 
		{
			{ {-1.0f, -1.0f} },
			{ {-1.0f,  3.0f} },
			{ { 3.0f, -1.0f} }
		};

		D3D11_BUFFER_DESC desc{};
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.ByteWidth = sizeof(triangle);
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA data = { triangle, 0, 0 };

		HRESULT hr =_device->CreateBuffer(&desc, &data, std::out_ptr(_vertex_buffer));
		if (FAILED(hr))
			throw std::runtime_error("failed to create the vertex buffer");
	}

	void GPU::setup_inputlayout(void)
	{
		// float2 pos : POSITION
		D3D11_INPUT_ELEMENT_DESC layout[] = 
		{ {
			"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			0, D3D11_INPUT_PER_VERTEX_DATA, 0
		} };

		HRESULT hr = _device->CreateInputLayout(
			layout,
			ARRAYSIZE(layout),
			_vs_blob->GetBufferPointer(),
			_vs_blob->GetBufferSize(),
			std::out_ptr(_input_layout));

		if (FAILED(hr))
			throw std::runtime_error("failed to create the input layout");
	}

	void GPU::setup_viewport(void)
	{
		const auto dimensions = ::get_dimensions(_hwnd);

		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = static_cast<float>(dimensions.width);
		vp.Height = static_cast<float>(dimensions.height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		_context->RSSetViewports(1, &vp);
	}

	// for resizing purposes
	void GPU::refresh_rendertargetview(void)
	{
		_render_target_view.reset();

		const auto dimensions = ::get_dimensions(_hwnd);

		if (_swapchain != nullptr)
		{
			HRESULT hr = _swapchain->ResizeBuffers(0, dimensions.width, dimensions.height, DXGI_FORMAT_UNKNOWN, 0);
			if (FAILED(hr))
				throw std::runtime_error("failed to resize the swapchain buffers");

			ID3D11Texture2D* back_buffer = nullptr;
			hr = _swapchain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
			if (FAILED(hr))
				throw std::runtime_error("failed to fetch the existing back buffer");

			hr = _device->CreateRenderTargetView(back_buffer, nullptr, std::out_ptr(_render_target_view));
			if (FAILED(hr))
				throw std::runtime_error("failed to resize the render target view");
		}
	}

	void GPU::configure_shaders(void)
	{
		const UINT stride = sizeof(float) * 2;
		const UINT offset = 0;

		_context->IASetInputLayout(_input_layout.get());

		ID3D11Buffer* vb = _vertex_buffer.get();
		_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

		_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_context->VSSetShader(_vertex_shader.get(), nullptr, 0);
		_context->PSSetShader(_pixel_shader.get(), nullptr, 0);

		ID3D11RenderTargetView* rtv = _render_target_view.get();
		_context->OMSetRenderTargets(1, &rtv, nullptr);
	}

	void GPU::draw_fullscreentriangle(void)
	{
		_context->Draw(3, 0);
	}

	void GPU::present(void)
	{
		_swapchain->Present(1, 0);
	}


	// primary external interface

	GPU::GPU(HINSTANCE instance, int width, int height)
	{
		if (_gpu != nullptr)
			throw std::runtime_error("failed to instantiate a new gpu: there can exist at most one instance");

		_gpu = this;

		create_window(instance, width, height, SHOW_OPENWINDOW);
		init();
	}

	void GPU::init(void)
	{
		create_device();
		create_swapchain();
		create_rendertargetview();
		create_vertexshader("VertexShader.hlsl");
		create_pixelshader("PixelShader.hlsl");
		setup_fullscreentriangle();
		setup_inputlayout();
		setup_viewport();
	}

	/*void GPU::render(void)
	{
		const float clear_color[] = { 0.f, 0.f, 0.f, 1.f };
		_context->ClearRenderTargetView(_render_target_view.get(), clear_color);

		configure_shaders();
		draw_fullscreentriangle();
		present();
	}*/

	void GPU::render(void)
	{
		D3D11_VIEWPORT vp = {};
		auto dims = ::get_dimensions(_hwnd);
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = static_cast<float>(dims.width);
		vp.Height = static_cast<float>(dims.height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		_context->RSSetViewports(1, &vp);

		const float clear_color[] = { 0.f, 0.f, 0.f, 1.f };
		_context->ClearRenderTargetView(_render_target_view.get(), clear_color);

		configure_shaders();
		draw_fullscreentriangle();
		present();
	}

	void GPU::loop(void)
	{
		MSG msg = {};
		while (true)
		{
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
					return;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			render();
		}
	}

	void GPU::dispose(void)
	{

	}
}
