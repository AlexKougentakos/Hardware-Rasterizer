#pragma once
#include <string>

namespace dae
{
	struct Vector2;

	class Texture
	{
	public:
		enum TextureType
		{
			Diffuse,
			Normal,
			Specular,
			Gloss
		};

		Texture(const std::string& path, ID3D11Device* pDevice, TextureType textureType);
		Texture(SDL_Surface* pSurface);
		~Texture();
		static Texture* LoadFromFile(const std::string& path);
		ID3D11Texture2D* GetResource() const;
		ID3D11ShaderResourceView* GetShaderResourceView() const;

		ColorRGB Sample(const Vector2& uv) const;

	private:
		//DirectX
		ID3D11Texture2D* m_pResource{};
		ID3D11ShaderResourceView* m_pShaderResourceView{};

		//Software
		SDL_Surface* m_pSurface{ nullptr };
		uint32_t* m_pSurfacePixels{ nullptr };
	};
}