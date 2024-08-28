#include "pch.h"
#include "Camera.h"
#include "Renderer.h"
#include "EffectTransparent.h"
#include "EffectPosTex.h"

namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;
			std::cout << "DirectX is initialized and ready!\n";
		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}

		const float aspectRatio{ float(m_Width) / float(m_Height) };

		m_pCamera = new Camera();
		m_pCamera->Initialize(45, { 0,0,0 }, aspectRatio);

		//Software
		InitializeSoftware();
		InitializeDX();

		//Print Controls
		std::cout << "\033[1;33m[Key Bindings - SHARED]\033[0m" << std::endl;
		std::cout << "   \033[1;33m[F1]  Toggle Rasterizer Mode (HARDWARE/SOFTWARE)\033[0m" << std::endl;
		std::cout << "   \033[1;33m[F2]  Toggle Vehicle Rotation (ON/OFF)\033[0m" << std::endl;
		//std::cout << "   \033[1;33m[F9]  Cycle CullMode (BACK/FRONT/NONE)\033[0m" << std::endl;
		std::cout << "   \033[1;33m[F10] Toggle Uniform ClearColor (ON/OFF)\033[0m" << std::endl;
		std::cout << "   \033[1;33m[F11] Toggle Print FPS (ON/OFF)\033[0m" << std::endl;
		std::cout << std::endl;
		std::cout << "\033[1;32m[Key Bindings - HARDWARE]\033[0m" << std::endl;
		std::cout << "   \033[1;32m[F3] Toggle FireFX (ON/OFF)\033[0m" << std::endl;
		std::cout << "   \033[1;32m[F4] Cycle Sampler State (POINT/LINEAR/ANISOTROPIC)\033[0m" << std::endl;
		std::cout << std::endl;
		std::cout << "\033[1;35m[Key Bindings - SOFTWARE]\033[0m" << std::endl;
		std::cout << "   \033[1;35m[F5] Cycle Shading Mode (COMBINED/OBSERVED_AREA/DIFFUSE/SPECULAR)\033[0m" << std::endl;
		std::cout << "   \033[1;35m[F6] Toggle NormalMap (ON/OFF)\033[0m" << std::endl;
		std::cout << "   \033[1;35m[F7] Toggle DepthBuffer Visualization (ON/OFF)\033[0m" << std::endl;
		std::cout << "   \033[1;35m[F8] Toggle BoundingBox Visualization (ON/OFF)\033[0m" << std::endl;
	}

	Renderer::~Renderer()
	{
		m_pRenderTargetView->Release();
		m_pRenderTargetBuffer->Release();
		m_pDepthStencilView->Release();
		m_pDepthStencilBuffer->Release();
		m_pSwapChain->Release();
		if (m_pDeviceContext)
		{
			m_pDeviceContext->ClearState();
			m_pDeviceContext->Flush();
			m_pDeviceContext->Release();
		}
		m_pDevice->Release();

		delete m_pVehicleMesh;
		delete m_pFireMesh;
		delete m_pCamera;
		delete m_pDiffuseTexture;
		delete m_pGlossTexture;
		delete m_pNormalTexture;
		delete m_pSpecularTexture;

		//Software
		delete m_pTexture;
		delete m_pNormalMap;
		delete m_pGlossinessMap;
		delete m_pSpecularMap;
		delete[] m_pDepthBufferPixels;


	}

	void Renderer::Update(const Timer* pTimer)
	{
		m_pCamera->Update(pTimer);
		m_pVehicleMesh->UpdateMeshMatrices(m_pCamera->viewMatrix * m_pCamera->projectionMatrix, m_pCamera->invViewMatrix);
		m_pFireMesh->UpdateMeshMatrices(m_pCamera->viewMatrix * m_pCamera->projectionMatrix, m_pCamera->invViewMatrix);
		m_pFireMesh->UpdateMeshMatrices(m_pCamera->viewMatrix * m_pCamera->projectionMatrix, m_pCamera->invViewMatrix);
		if (m_DoesRotate)
		{
			constexpr float rotationSpeed{ 45.f };
			m_pVehicleMesh->RotateY(TO_RADIANS * rotationSpeed * pTimer->GetElapsed());
			m_pFireMesh->RotateY(TO_RADIANS * rotationSpeed * pTimer->GetElapsed());
			
		}

	}

	void Renderer::InitializeDX()
	{
		//------------
		//  VEHICLE
		//------------
		const auto pEffect = new EffectPosTex(m_pDevice, L"Effects/effect.fx"); //We don't delete this since it will be copied by the mesh ptr and deleted after
		const auto pSpecularTexture = new Texture("Resources/vehicle_specular.png", m_pDevice, Texture::Specular);
		const auto pGlossTexture = new Texture("Resources/vehicle_gloss.png", m_pDevice, Texture::Gloss);
		const auto pNormalTexture = new Texture("Resources/vehicle_normal.png", m_pDevice, Texture::Normal);
		const auto pDiffuseTexture = new Texture("Resources/vehicle_diffuse.png", m_pDevice, Texture::Diffuse);

		m_pVehicleMesh = new Mesh(m_pDevice, pEffect, "Resources/vehicle.obj");
		m_pVehicleMesh->InitializeMeshMatrices({ 0,0,50 }, { 0, PI_DIV_2,0 }, { 1,1,1 });
		m_pVehicleMesh->UpdateMeshMatrices(m_pCamera->viewMatrix * m_pCamera->projectionMatrix, m_pCamera->invViewMatrix);
		pEffect->SetTexture(pDiffuseTexture, Texture::Diffuse);
		pEffect->SetTexture(pGlossTexture, Texture::Gloss);
		pEffect->SetTexture(pNormalTexture, Texture::Normal);
		pEffect->SetTexture(pSpecularTexture, Texture::Specular);

		//------------
		//   FLAME	  
		//------------ 
		const auto pTransparentEffect = new EffectTransparent(m_pDevice, L"Effects/transparency.fx");
		const auto pFireDiffuseTexture = new Texture("Resources/fireFX_diffuse.png", m_pDevice, Texture::Diffuse);
		pTransparentEffect->SetTexture(pFireDiffuseTexture, Texture::Diffuse);
		m_pFireMesh = new Mesh(m_pDevice, pTransparentEffect, "Resources/fireFX.obj");
		m_pFireMesh->InitializeMeshMatrices({ 0,0,50 }, { 0, PI_DIV_2,0 }, { 1,1,1 });

		//Delete the temporary textures
		delete pFireDiffuseTexture;
		delete pSpecularTexture;
		delete pGlossTexture;
		delete pNormalTexture;
		delete pDiffuseTexture;
	}

	void Renderer::InitializeSoftware()
	{
		SDL_GetWindowSize(m_pWindow, &m_Width, &m_Height);

		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(m_pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		m_pDepthBufferPixels = new float[m_Width * m_Height];

		//Initialize all values to FLT_MAX
		for (int i{ 0 }; i < (m_Width * m_Height); ++i)
			m_pDepthBufferPixels[i] = FLT_MAX;

		m_AspectRatio = float(m_Width) / float(m_Height);

		m_pTexture = Texture::LoadFromFile("Resources/vehicle_diffuse.png");
		m_pNormalMap = Texture::LoadFromFile("Resources/vehicle_normal.png");
		m_pGlossinessMap = Texture::LoadFromFile("Resources/vehicle_gloss.png");
		m_pSpecularMap = Texture::LoadFromFile("Resources/vehicle_specular.png");
	}

	void Renderer::RenderSoftware() const
	{
		SDL_LockSurface(m_pBackBuffer);
		m_pVehicleMesh->m_VerticesOut.clear();

		VertexTransformationFunction(*m_pVehicleMesh);

		std::vector<Vector2> verteciesRaster;
		for (const auto& vertex : m_pVehicleMesh->m_VerticesOut)
			verteciesRaster.push_back({ (vertex.position.x + 1) / 2.0f * m_Width,
					(1.0f - vertex.position.y) / 2.0f * m_Height });

		//Clear depth buffer & background
		for (int i{ 0 }; i < (m_Width * m_Height); ++i)
		{
			m_pDepthBufferPixels[i] = FLT_MAX;
			if (!m_UseUniformBackground)
			m_pBackBufferPixels[i] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(0.39f * 255),
				static_cast<uint8_t>(0.39f * 255),
				static_cast<uint8_t>(0.39f * 255));
			else
				m_pBackBufferPixels[i] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(0.1f * 255),
					static_cast<uint8_t>(0.1f * 255),
					static_cast<uint8_t>(0.1f * 255));
		}

		assert(m_pVehicleMesh->m_VerticesOut.size() % 3 == 0);
		//Check if the number of vertecies is divisible by 3.
		//If not then there is an issue with our triangles

		//Use this for triangle strip.
		//for (int startVertexIndex{ 0 }; startVertexIndex < m_pVehicleMesh->m_Indices.size() - 2; ++startVertexIndex)
			//RenderTriangle(*m_pVehicleMesh, verteciesRaster, startVertexIndex, startVertexIndex % 2);
		for (int vertexIndex{ 0 }; vertexIndex < m_pVehicleMesh->m_Indices.size(); vertexIndex += 3)
			RenderTriangle(*m_pVehicleMesh, verteciesRaster, vertexIndex, false);

		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}

	void Renderer::RenderTriangle(const Mesh& mesh, const std::vector<Vector2>& verteciesRaster,
		int vertexIndex, bool swapVertex) const
	{
		const size_t vertexIndex0{ mesh.m_Indices[vertexIndex + (2 * swapVertex)] };
		const size_t vertexIndex1{ mesh.m_Indices[vertexIndex + 1] };
		const size_t vertexIndex2{ mesh.m_Indices[vertexIndex + (!swapVertex * 2)] };

		// Make sure the triangle doesn't have the same vertex twice. If it does it's got no area so we don't have to render it.
		if (vertexIndex0 == vertexIndex1 || vertexIndex1 == vertexIndex2 || vertexIndex2 == vertexIndex0)
			return;

		const Vector2 vertex0{ verteciesRaster[vertexIndex0] };
		const Vector2 vertex1{ verteciesRaster[vertexIndex1] };
		const Vector2 vertex2{ verteciesRaster[vertexIndex2] };

		const Vector2 vertex0NDC = { mesh.m_VerticesOut[vertexIndex0].position.x, mesh.m_VerticesOut[vertexIndex0].position.y };
		const Vector2 vertex1NDC = { mesh.m_VerticesOut[vertexIndex1].position.x, mesh.m_VerticesOut[vertexIndex1].position.y };
		const Vector2 vertex2NDC = { mesh.m_VerticesOut[vertexIndex2].position.x, mesh.m_VerticesOut[vertexIndex2].position.y };

		if (vertex0NDC.x < -1.f || vertex0NDC.x > 1.f ||
			vertex0NDC.y < -1.f || vertex0NDC.y > 1.f ||
			vertex1NDC.x < -1.f || vertex1NDC.x > 1.f ||
			vertex1NDC.y < -1.f || vertex1NDC.y > 1.f ||
			vertex2NDC.x < -1.f || vertex2NDC.x > 1.f ||
			vertex2NDC.y < -1.f || vertex2NDC.y > 1.f) return;

		// Define the Bounding Box
		Vector2 bottomLeft{ Vector2::SmallestVectorComponents(vertex0,Vector2::SmallestVectorComponents(vertex1,vertex2)) };
		Vector2 topRight{ Vector2::BiggestVectorComponents(vertex0,Vector2::BiggestVectorComponents(vertex1,vertex2)) };

		// Add the margin to fix the black lines between the triangles
		constexpr float margin{ 1.f };
		bottomLeft -= {margin, margin};
		topRight += {margin, margin};

		Utils::Clamp(bottomLeft.x, 0, float(m_Width) - 1);
		Utils::Clamp(topRight.x, 0, float(m_Width) - 1);
		Utils::Clamp(bottomLeft.y, 0, float(m_Height) - 1);
		Utils::Clamp(topRight.y, 0, float(m_Height) - 1);

		ColorRGB finalColor{};
		for (int px{ int(bottomLeft.x) }; px < int(topRight.x); ++px)
		{
			for (int py{ int(bottomLeft.y) }; py < int(topRight.y); ++py)
			{
				const Vector2 currentPixel{ static_cast<float>(px),static_cast<float>(py) };
				const int pixelIdx{ px + py * m_Width };

				if (m_CurrentRenderMode == BOUNDING_BOX)
				{
					finalColor = ColorRGB{ 1, 1, 1 };

					m_pBackBufferPixels[pixelIdx] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));

					continue;
				}

				if (Utils::IsInTriangle(currentPixel, vertex0, vertex1, vertex2))
				{
					float weight0 = Vector2::Cross(currentPixel - vertex1, vertex1 - vertex2);
					float weight1 = Vector2::Cross(currentPixel - vertex2, vertex2 - vertex0);
					float weight2 = Vector2::Cross(currentPixel - vertex0, vertex0 - vertex1);

					const float totalTriangleArea{ Vector2::Cross(vertex1 - vertex0,vertex2 - vertex0) };
					const float invTotalTriangleArea{ 1 / totalTriangleArea };
					weight0 *= invTotalTriangleArea;
					weight1 *= invTotalTriangleArea;
					weight2 *= invTotalTriangleArea;

					const float depth0{ (mesh.m_VerticesOut[vertexIndex0].position.z) };
					const float depth1{ (mesh.m_VerticesOut[vertexIndex1].position.z) };
					const float depth2{ (mesh.m_VerticesOut[vertexIndex2].position.z) };
					const float interpolatedDepth{ 1.f /
							(weight0 * (1.f / depth0) +
							weight1 * (1.f / depth1) +
							weight2 * (1.f / depth2)) };

					if (m_pDepthBufferPixels[pixelIdx] < interpolatedDepth ||
						interpolatedDepth < 0.f || interpolatedDepth > 1.f) continue;

					m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;

					const float wDepth0{ mesh.m_VerticesOut[vertexIndex0].position.w };
					const float wDepth1{ mesh.m_VerticesOut[vertexIndex1].position.w };
					const float wDepth2{ mesh.m_VerticesOut[vertexIndex2].position.w };

					const float wInterpolated{ 1.f /
						(weight0 * (1.f / wDepth0) +
						weight1 * (1.f / wDepth1) +
						weight2 * (1.f / wDepth2)) };

					//UVs
					const Vector2 vertex0UV{ mesh.m_VerticesOut[vertexIndex0].uv / mesh.m_VerticesOut[vertexIndex0].position.w };
					const Vector2 vertex1UV{ mesh.m_VerticesOut[vertexIndex1].uv / mesh.m_VerticesOut[vertexIndex1].position.w };
					const Vector2 vertex2UV{ mesh.m_VerticesOut[vertexIndex2].uv / mesh.m_VerticesOut[vertexIndex2].position.w };
					Vector2 UVInterpolated{ (vertex0UV * weight0 + vertex1UV * weight1 + vertex2UV * weight2) * wInterpolated };
					UVInterpolated.y = std::max(UVInterpolated.y, 0.f);
					UVInterpolated.x = std::max(UVInterpolated.x, 0.f);

					//NORMALS
					const Vector3 vertex0Normal{ mesh.m_VerticesOut[vertexIndex0].normal / mesh.m_VerticesOut[vertexIndex0].position.w };
					const Vector3 vertex1Normal{ mesh.m_VerticesOut[vertexIndex1].normal / mesh.m_VerticesOut[vertexIndex1].position.w };
					const Vector3 vertex2Normal{ mesh.m_VerticesOut[vertexIndex2].normal / mesh.m_VerticesOut[vertexIndex2].position.w };
					Vector3 normalInterpolated{ (vertex0Normal * weight0 + vertex1Normal * weight1 + vertex2Normal * weight2) * wInterpolated };
					normalInterpolated.Normalize();

					//TANGENTS
					const Vector3 vertex0Tangent{ mesh.m_VerticesOut[vertexIndex0].tangent / mesh.m_VerticesOut[vertexIndex0].position.w };
					const Vector3 vertex1Tangent{ mesh.m_VerticesOut[vertexIndex1].tangent / mesh.m_VerticesOut[vertexIndex1].position.w };
					const Vector3 vertex2Tangent{ mesh.m_VerticesOut[vertexIndex2].tangent / mesh.m_VerticesOut[vertexIndex2].position.w };
					Vector3 tangentInterpolated{ (vertex0Tangent * weight0 + vertex1Tangent * weight1 + vertex2Tangent * weight2) * wInterpolated };
					tangentInterpolated.Normalize();

					//VIEW DIRECTION
					const Vector3 vertex0ViewDirection{ mesh.m_VerticesOut[vertexIndex0].viewDirection / mesh.m_VerticesOut[vertexIndex0].position.w };
					const Vector3 vertex1ViewDirection{ mesh.m_VerticesOut[vertexIndex1].viewDirection / mesh.m_VerticesOut[vertexIndex1].position.w };
					const Vector3 vertex2ViewDirection{ mesh.m_VerticesOut[vertexIndex2].viewDirection / mesh.m_VerticesOut[vertexIndex2].position.w };
					Vector3 viewDirectionInterpolated{ (vertex0ViewDirection * weight0 + vertex1ViewDirection * weight1 + vertex2ViewDirection * weight2) * wInterpolated };
					viewDirectionInterpolated.Normalize();

					Vertex_Out pixelOut{};
					pixelOut.uv = UVInterpolated;
					pixelOut.normal = normalInterpolated;
					pixelOut.tangent = tangentInterpolated;
					pixelOut.viewDirection = viewDirectionInterpolated;

					auto remap = [](float value, float min, float max)
					{
						return (value - min) / (max - min);
					};

					const float remappedResult = remap(interpolatedDepth, 0.995f, 1.f);

					//Update Color in Buffer
					switch (m_CurrentRenderMode)
					{
					case TEXTURE:
						//finalColor = m_pTexture->Sample(UVInterpolated);
						finalColor = PixelShading(pixelOut);
						break;
					case BOUNDING_BOX:
						break;
					case DEPTH_VALUES:
						finalColor = { remappedResult, remappedResult,remappedResult };
						break;

					}
					finalColor.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}

	ColorRGB Renderer::PixelShading(const Vertex_Out& vertex) const
	{
		//Parameters
		const Vector3 lightDirection = { .577f, -.577f, .577f };
		constexpr float kd{ 1.f }; //Diffuse Reflection Coefficient
		constexpr float lightIntensity{ 7.f };
		constexpr float shininess{ 25.f };

		const ColorRGB lambert{ (m_pTexture->Sample(vertex.uv) * kd) / PI };;
			if (m_CurrentShadingMode == ShadingMode::DIFFUSE) return lambert;
		const Matrix tangentSpaceMatrix{ vertex.tangent, Vector3::Cross(vertex.normal, vertex.tangent), vertex.normal, Vector3::Zero };
		ColorRGB normalColour = m_pNormalMap->Sample(vertex.uv);

		Vector3 normalMap{ normalColour.r, normalColour.g, normalColour.b };
		normalMap = 2.f * normalMap - Vector3{ 1,1,1 };

		normalMap = tangentSpaceMatrix.TransformVector(normalMap);
			if (!m_UseNormalMap) normalMap = vertex.normal;

		float observedArea = Vector3::Dot(normalMap, -lightDirection);
		observedArea = std::max(observedArea, 0.f);
			if (m_CurrentShadingMode == ShadingMode::OBSERVED_AREA) return ColorRGB{ 1,1,1 } * observedArea;
		float glossiness = m_pGlossinessMap->Sample(vertex.uv).r;
		float specular = m_pSpecularMap->Sample(vertex.uv).r;
			if (m_CurrentShadingMode == ShadingMode::SPECULAR) return ColorRGB{ 1,1,1 } * specular;

		glossiness *= shininess;

		const Vector3 reflect = Vector3::Reflect(-lightDirection, normalMap);
		const float alfa = Vector3::Dot(reflect, vertex.viewDirection);
		float PSR{};
		if (alfa >= 0)
			PSR = specular * (powf(alfa, glossiness));

		const ColorRGB phong{ PSR, PSR, PSR };
		if (m_CurrentShadingMode == ShadingMode::COMBINED)
			return lambert * lightIntensity * observedArea + phong;
	}

	void Renderer::VertexTransformationFunction(Mesh& mesh) const
	{
		const Matrix worldViewMatrix = mesh.worldMatrix * m_pCamera->viewMatrix * m_pCamera->projectionMatrix;

		for (const auto& vertex : mesh.m_SoftwareVertives)
		{
			Vertex_Out vertexOut{ Vector4{}, vertex.color, vertex.uv, vertex.normal, vertex.tangent, vertex.viewDirection };

			vertexOut.position = worldViewMatrix.TransformPoint({ vertex.position, 1.f });

			//Perspetive Divide
			vertexOut.position.x /= vertexOut.position.w;
			vertexOut.position.y /= vertexOut.position.w;
			vertexOut.position.z /= vertexOut.position.w;

			//Transform the normals to world space
			vertexOut.normal = mesh.worldMatrix.TransformVector(vertexOut.normal);
			vertexOut.normal.Normalize();

			vertexOut.tangent = mesh.worldMatrix.TransformVector(vertexOut.tangent);

			vertexOut.viewDirection = mesh.worldMatrix.TransformPoint(vertex.position) - m_pCamera->origin;
			vertexOut.viewDirection.Normalize();

			mesh.m_VerticesOut.push_back(vertexOut);
		}
	}


	void Renderer::RenderDirectX() const
	{
		if (!m_IsInitialized)
			return;

		////1. Clear RTV & DSV
		ColorRGB clearColor{ .39f, .59f, .93f };
		if (m_UseUniformBackground) clearColor = ColorRGB{ .1f, .1f, .1f };

		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, &clearColor.r);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		////2. Set Pipeline
		m_pVehicleMesh->Render(m_pDeviceContext, m_pCamera);
		if (m_IsUsingFireFX)
			m_pFireMesh->Render(m_pDeviceContext, m_pCamera);

		////3. Present backbuffer (swap)
		m_pSwapChain->Present(0, 0);
	}

	HRESULT Renderer::InitializeDirectX()
	{
		//1. Create Device & DeviceContext
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
		uint32_t createDeviceFlags = 0;
#if defined (DEBUG) || defined(_DEBUG)
			createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
			HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel,
				1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);
		if (FAILED(result))
			return result;

		//Create DXGI Factory
		IDXGIFactory1* pDxgiFactory{};
		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**> (&pDxgiFactory));

		if (FAILED(result))
			return result;

		//2. Create Swapchain
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		//Get the handle (HWND) from the SDL Backbuffer
		SDL_SysWMinfo sysWMInfo{};
		SDL_VERSION(&sysWMInfo.version)
		SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
		swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

		//Create SwapChain
		result = pDxgiFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
		if (FAILED(result))
			return result;

		//3. Create DepthStencil (DS) & DepthStencilView (DSV)
		//Resource
		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		//View
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);
		if (FAILED(result))
			return result;

		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
		if (FAILED(result))
			return result;

		//4. Create RenderTarget (RT) & RenderTargetView (RTV)
		//Resource
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
		if (FAILED(result))
			return result;

		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);
		if (FAILED(result))
			return result;

		//5. Bind RTV & DSV to Output Merger Stage
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		//6. Set Viewport
		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(m_Width);
		viewport.Height = static_cast<float>(m_Height);
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		m_pDeviceContext->RSSetViewports(1, &viewport);

		//Release local resource before leaving the function
		pDxgiFactory->Release();

		return result;
	}

	void Renderer::CycleCurrentFilteringTechnique()
	{
		m_pVehicleMesh->CycleCurrentFilteringTechnique();
	}

	void Renderer::CycleShadingMode()
	{
		switch (m_CurrentShadingMode)
		{
		case ShadingMode::COMBINED:
			std::cout << "\033[1;35m(SOFTWARE) Diffuse Shading\033[0m" << std::endl;
			m_CurrentShadingMode = DIFFUSE;
			break;
		case ShadingMode::DIFFUSE:
			std::cout << "\033[1;35m(SOFTWARE) Observed Area Shading\033[0m" << std::endl;
			m_CurrentShadingMode = OBSERVED_AREA;
			break;
		case ShadingMode::OBSERVED_AREA:
			std::cout << "\033[1;35m(SOFTWARE) Specular Shading\033[0m" << std::endl;
			m_CurrentShadingMode = SPECULAR;
			break;
		case ShadingMode::SPECULAR:
			std::cout << "\033[1;35m(SOFTWARE) Combined Shading\033[0m" << std::endl;
			m_CurrentShadingMode = COMBINED;
			break;
		}
	}

	void Renderer::HandleInput(SDL_Event event)
	{
		if (event.type != SDL_KEYUP) return;

		if (event.key.keysym.scancode == SDL_SCANCODE_F2)
		{
			if (m_DoesRotate)
				std::cout << "\033[1;33m(SHARED) Stopping Rotation\033[0m" << std::endl;
			else std::cout << "\033[1;33m(SHARED) Starting Rotation\033[0m" << std::endl;
			m_DoesRotate = !m_DoesRotate;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_F3)
		{
			if (m_IsUsingFireFX)
				std::cout << "\033[1;32m(HARDWARE) Disabled FireFX\033[0m" << std::endl;
			else std::cout << "\033[1;32m(HARDWARE) Enabled FireFX\033[0m" << std::endl;
			m_IsUsingFireFX = !m_IsUsingFireFX;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_F4)
			CycleCurrentFilteringTechnique();
		if (event.key.keysym.scancode == SDL_SCANCODE_F5)
			CycleShadingMode();
		if (event.key.keysym.scancode == SDL_SCANCODE_F6)
		{
			if (m_UseNormalMap)
				std::cout << "\033[1;35m(SOFTWARE) Disabled Normal Map\033[0m" << std::endl;
			else std::cout << "\033[1;35m(SOFTWARE) Enabled Normal Map\033[0m" << std::endl;
			m_UseNormalMap = !m_UseNormalMap;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_F7)
		{
			if (m_CurrentRenderMode == TEXTURE)
			{
				m_CurrentRenderMode = DEPTH_VALUES;
				std::cout << "\033[1;35m(SOFTWARE) Enabled Depth Values\033[0m" << std::endl;
			}
			else if (m_CurrentRenderMode == DEPTH_VALUES)
			{
				m_CurrentRenderMode = TEXTURE;
				std::cout << "\033[1;35m(SOFTWARE) Disabled Depth Values\033[0m" << std::endl;
			}
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_F8)
		{
			if (m_CurrentRenderMode == TEXTURE)
			{
				m_CurrentRenderMode = BOUNDING_BOX;
				std::cout << "\033[1;35m(SOFTWARE) Enabled Bounding Box\033[0m" << std::endl;
			}
			else if (m_CurrentRenderMode == BOUNDING_BOX)
			{
				m_CurrentRenderMode = TEXTURE;
				std::cout << "\033[1;35m(SOFTWARE) Disabled Bounding Box\033[0m" << std::endl;
			}
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_F10)
		{
			if (m_UseUniformBackground)
				std::cout << "\033[1;33m(SHARED) Disabled Uniform Background\033[0m" << std::endl;
			else std::cout << "\033[1;33m(SHARED) Enabled Uniform Background\033[0m" << std::endl;
			m_UseUniformBackground = !m_UseUniformBackground;
		}
	}

	void Renderer::SetRasterizerModel(bool isUsingDX)
	{
		m_IsUsingDX = isUsingDX;
	}
}
