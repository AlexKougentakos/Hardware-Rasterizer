#pragma once
#include <map>

#include "Effect.h"
#include "Mesh.h"

struct SDL_Window;
struct SDL_Surface;

class Camera;
class Texture;

namespace dae
{
	class Renderer final
	{
	public:
		enum ShadingMode
		{
			COMBINED,
			OBSERVED_AREA,
			DIFFUSE,
			SPECULAR
		};

		enum RenderingMode
		{
			TEXTURE,
			BOUNDING_BOX,
			DEPTH_VALUES
		};

		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void RenderDirectX() const;
		void RenderSoftware() const;

		void SetRasterizerModel(bool isUsingDX);
		void HandleInput(SDL_Event event);

	private:
		void CycleCurrentFilteringTechnique();
		void CycleShadingMode();

		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		//DIRECTX
		HRESULT InitializeDirectX();
		ID3D11Device* m_pDevice{};
		ID3D11DeviceContext* m_pDeviceContext{};
		IDXGISwapChain* m_pSwapChain{};

		//Resource Views
		ID3D11DepthStencilView* m_pDepthStencilView{};
		ID3D11Texture2D* m_pDepthStencilBuffer{};

		ID3D11Texture2D* m_pRenderTargetBuffer{};
		ID3D11RenderTargetView* m_pRenderTargetView{};

		Mesh* m_pVehicleMesh{};
		Mesh* m_pFireMesh{};
		Camera* m_pCamera{};
		Texture* m_pDiffuseTexture{};
		Texture* m_pNormalTexture{};
		Texture* m_pGlossTexture{};
		Texture* m_pSpecularTexture{};

		void InitializeDX();
		//=============================
		//    Software Rasterizer
		//=============================

		//Variables
		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		float m_AspectRatio{};

		Texture* m_pTexture{};
		Texture* m_pNormalMap{};
		Texture* m_pGlossinessMap{};
		Texture* m_pSpecularMap{};

		//Functions
		void InitializeSoftware();
		void VertexTransformationFunction(Mesh& mesh) const;
		void RenderTriangle(const Mesh& mesh, const std::vector<Vector2>& verteciesRaster,
			int vertexIndex, bool swapVertex) const;
		ColorRGB PixelShading(const Vertex_Out& vertex) const;

		//Settings & Toggles
		bool m_IsUsingDX{ true }; //F1
		bool m_DoesRotate{ true }; //F2
		bool m_IsUsingFireFX{ true }; //F3
		bool m_UseNormalMap{ true }; //F6
		bool m_UseUniformBackground{ false }; //F10

		ShadingMode m_CurrentShadingMode{ShadingMode::COMBINED};
		RenderingMode m_CurrentRenderMode{ RenderingMode::TEXTURE };


	};
}
