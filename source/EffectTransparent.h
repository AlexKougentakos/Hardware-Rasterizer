#pragma once
#include "Effect.h"

class Texture;

class EffectTransparent final : public Effect
{
public:
	EffectTransparent(ID3D11Device* pDevice, const std::wstring& assertFile)
		:Effect{ pDevice, assertFile }
	{
		// Save the diffuse texture variable of the effect as a member variable
		m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
		if (!m_pDiffuseMapVariable->IsValid()) std::wcout << L"m_pDiffuseMapVariable not valid\n";
	}
	virtual ~EffectTransparent() override
	{
		m_pDiffuseMapVariable->Release();
	}

	virtual void SetTexture(const dae::Texture* pTexture, dae::Texture::TextureType textureType) const override
	{
		if (textureType == dae::Texture::Diffuse)
			m_pDiffuseMapVariable->SetResource(pTexture->GetShaderResourceView());
		else { std::cout << "Effect Transparent only takes Diffuse Texture!"; }
	}

	virtual void SetMatrix(dae::Matrix matrix, dae::Matrix::MatrixType matrixType) override
	{
		if (matrixType == Matrix::worldViewProjectionMatrix)
			m_pMatWorldViewProjVariable->SetMatrix(reinterpret_cast<float*>(&matrix));
	}

	virtual void UpdateEffect() const override {}
	virtual void CycleCurrentFilteringTechnique() override {}

private:
	ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable{};	
};
