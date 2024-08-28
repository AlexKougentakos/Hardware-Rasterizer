#include "pch.h"
#include "Texture.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(const std::string& path, ID3D11Device* pDevice, TextureType textureType)
	{
		// Make SDL_Surface, release at the end
		SDL_Surface* pSurface = IMG_Load(path.c_str());

		// Texture description
		const DXGI_FORMAT format{ DXGI_FORMAT_R8G8B8A8_UNORM };
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = pSurface->w;
		desc.Height = pSurface->h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = pSurface->pixels;
		initData.SysMemPitch = static_cast<UINT>(pSurface->pitch);
		initData.SysMemSlicePitch = static_cast<UINT>(pSurface->h * pSurface->pitch);

		HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &m_pResource);

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
		SRVDesc.Format = format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		hr = pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, &m_pShaderResourceView);

		SDL_FreeSurface(pSurface);
	}
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		Texture* texture = new Texture{ IMG_Load(path.c_str()) };
		return texture;
	}
	Texture::~Texture()
	{
		if (m_pResource) m_pResource->Release();
		if (m_pShaderResourceView) m_pShaderResourceView->Release();
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}

	}
	ID3D11Texture2D* Texture::GetResource() const
	{
		return m_pResource;
	}
	ID3D11ShaderResourceView* Texture::GetShaderResourceView() const
	{
		return m_pShaderResourceView;
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		unsigned char r{}, g{}, b{};

		const int x{ std::min(int(uv.x * m_pSurface->w), m_pSurface->w - 1) };
		const int y{ std::min(int(uv.y * m_pSurface->h), m_pSurface->h - 1) };

		unsigned const int pixel{ m_pSurfacePixels[x + y * m_pSurface->w] };

		SDL_GetRGB(pixel, m_pSurface->format, &r, &g, &b);

		constexpr float inverseClampedValue{ 1 / 255.f };

		return { r * inverseClampedValue, g * inverseClampedValue, b * inverseClampedValue };
	}
}