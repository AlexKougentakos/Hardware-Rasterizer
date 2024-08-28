#pragma once
#include "pch.h"

#include <cassert>

#include "Texture.h"


class Effect
{
public:

	enum FilteringMethod
	{
		POINT,
		LINEAR,
		ANISOTROPIC

	};

	Effect(ID3D11Device* pDevice, const std::wstring& assertFile);
	virtual ~Effect();

	virtual void SetTexture(const dae::Texture* pDiffuseTexture, dae::Texture::TextureType textureType) const = 0;
	virtual void SetMatrix(dae::Matrix matrix, dae::Matrix::MatrixType matrixType) = 0;

	virtual void UpdateEffect() const = 0;
	virtual void CycleCurrentFilteringTechnique() = 0;

	ID3DX11EffectTechnique* GetTechnique() const;
	ID3D11InputLayout* GetInputLayout() const;
	void CreateInputLayout(ID3D11Device* pDevice);
private:
	//Member Variables
	ID3D11InputLayout* m_pInputLayout{};
protected:
	ID3DX11Effect* m_pEffect{};
	ID3DX11EffectTechnique* m_pTechnique{};
	ID3DX11EffectMatrixVariable* m_pMatWorldViewProjVariable{};

	ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assertFile) const;

};