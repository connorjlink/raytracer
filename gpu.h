#ifndef LUMA_GPU_H
#define LUMA_GPU_H

// gpu.h
// (c) 2025 Connor J. Link. All Rights Reserved.

#include <memory>
#include <d3d11.h>

namespace luma
{
	class GPU
	{
	private:
		HWND _hwnd;

	private:
		std::unique_ptr<ID3D11Device> _device;
		std::unique_ptr<ID3D11DeviceContext> _context;
		std::unique_ptr<IDXGISwapChain> _swapchain;
		std::unique_ptr<ID3D11RenderTargetView> _render_target_view;
		std::unique_ptr<ID3D11VertexShader> _vertex_shader;
		std::unique_ptr<ID3D11PixelShader> _pixel_shader;
		std::unique_ptr<ID3DBlob> _vs_blob;
		std::unique_ptr<ID3DBlob> _ps_blob;
		std::unique_ptr<ID3D11InputLayout> _input_layout;
		std::unique_ptr<ID3D11Buffer> _vertex_buffer;

	private:
		static LRESULT CALLBACK WindowHandler(HWND, UINT, WPARAM, LPARAM);

	private:
		void create_window(HINSTANCE, int, int, int);
		void create_device(void);
		void create_swapchain(void);
		void create_rendertargetview(void);
		
	private:
		void create_vertexshader(const char*);
		void create_pixelshader(const char*);

	private:
		void setup_fullscreentriangle(void);
		void setup_inputlayout(void);
		void setup_viewport(void);

	// render commands
	private:
		void refresh_rendertargetview(void);
		void configure_shaders(void);
		void draw_fullscreentriangle(void);
		void present(void);

	public:
		GPU(HINSTANCE, int, int);

	public:
		void init(void);
		void render(void);
		void loop(void);
		void dispose(void);
	};

	extern GPU* _gpu;
}

#endif
