#pragma once
#include "Effect.h"

class Texture;

class EffectPosTex final : public Effect
{
public:
	EffectPosTex(ID3D11Device* pDevice, const std::wstring& assertFile)
		:Effect( pDevice, assertFile )
	{
		//Link the texture maps
		m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
		if (!m_pDiffuseMapVariable->IsValid()) { std::wcout << L"m_pDiffuseMapVariable not valid\n"; }

		m_pGlossMapVariable = m_pEffect->GetVariableByName("gGlossinessMap")->AsShaderResource();
		if (!m_pGlossMapVariable->IsValid()) { std::wcout << L"m_pGlossMapVariable not valid\n"; }

		m_pNormalMapVariable = m_pEffect->GetVariableByName("gNormalMap")->AsShaderResource();
		if (!m_pNormalMapVariable->IsValid()) { std::wcout << L"m_pNormalMapVariable not valid\n"; }

		m_pSpecularMapVariable = m_pEffect->GetVariableByName("gSpecularMap")->AsShaderResource();
		if (!m_pSpecularMapVariable->IsValid()) { std::wcout << L"m_pSpecularMapVariable not valid\n"; }

		// Link the matrices
		m_pMatInverseViewVariable = m_pEffect->GetVariableByName("gViewInverseMatrix")->AsMatrix();
		if (!m_pMatInverseViewVariable->IsValid()) { std::wcout << L"m_pMatInverseViewVariable not valid\n"; }

		m_pMatWorldVariable = m_pEffect->GetVariableByName("gWorldMatrix")->AsMatrix();
		if (!m_pMatWorldVariable->IsValid()) { std::wcout << L"m_pMatWorldVariable not valid\n"; }

		// Link the sampler
		m_pEffectSamplerVariable = m_pEffect->GetVariableByName("samPoint")->AsSampler();
		if (!m_pEffectSamplerVariable->IsValid()) { std::wcout << L"m_pEffectSamplerVariable not valid\n"; }

		D3D11_SAMPLER_DESC samplerDesc;
		ZeroMemory(&samplerDesc, sizeof(samplerDesc));
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

		HRESULT hr = pDevice->CreateSamplerState(&samplerDesc, &m_pSamplerStateLinear);
		if (FAILED(hr)) { std::wcout << "Failed to create linear sampler state" << std::endl; }

		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		hr = pDevice->CreateSamplerState(&samplerDesc, &m_pSamplerStateAnisotropic);
		if (FAILED(hr)) { std::wcout << "Failed to create anisotropic sampler state" << std::endl; }

		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		hr = pDevice->CreateSamplerState(&samplerDesc, &m_pSamplerStatePoint);
		if (FAILED(hr)) { std::wcout << "Failed to create point sampler state" << std::endl; }
	}
	virtual ~EffectPosTex() override
	{
		m_pSamplerStateAnisotropic->Release();
		m_pSamplerStatePoint->Release();
		m_pSamplerStateLinear->Release();
		m_pEffectSamplerVariable->Release();
		m_pDiffuseMapVariable->Release();
		m_pNormalMapVariable->Release();
		m_pGlossMapVariable->Release();
		m_pSpecularMapVariable->Release();
	}

	virtual void SetTexture(const dae::Texture* pTexture, dae::Texture::TextureType textureType) const override
	{
		switch (textureType)
		{
		case dae::Texture::Diffuse:
			m_pDiffuseMapVariable->SetResource(pTexture->GetShaderResourceView());
			break;
		case dae::Texture::Gloss:
			m_pGlossMapVariable->SetResource(pTexture->GetShaderResourceView());
			break;
		case dae::Texture::Normal:
			m_pNormalMapVariable->SetResource(pTexture->GetShaderResourceView());
			break;
		case dae::Texture::Specular:
			m_pSpecularMapVariable->SetResource(pTexture->GetShaderResourceView());
			break;
		}
	}

	virtual void SetMatrix(dae::Matrix matrix, dae::Matrix::MatrixType matrixType) override
	{
		switch (matrixType)
		{
		case Matrix::inverseViewMatrix:
			m_pMatInverseViewVariable->SetMatrix({ reinterpret_cast<float*>(&matrix) });
			break;
		case Matrix::worldMatrix:
			m_pMatWorldVariable->SetMatrix({ reinterpret_cast<float*>(&matrix) });
			break;
		case Matrix::worldViewProjectionMatrix:
			m_pMatWorldViewProjVariable->SetMatrix(reinterpret_cast<float*>(&matrix));
			break;
		}
	}

	void UpdateEffect() const override
	{
		switch (m_CurrentFilterMethod)
		{
		case POINT:
			m_pEffectSamplerVariable->SetSampler(0, m_pSamplerStatePoint);
			break;
		case LINEAR:
			m_pEffectSamplerVariable->SetSampler(0, m_pSamplerStateLinear);
			break;
		case ANISOTROPIC:
			m_pEffectSamplerVariable->SetSampler(0, m_pSamplerStateAnisotropic);
			break;
		}
	}

	void CycleCurrentFilteringTechnique() override
	{
		switch (m_CurrentFilterMethod)
		{
		case POINT:
				std::cout << "\033[1;32m(HARDWARE) Linear Sampling\033[0m" << std::endl;
			m_CurrentFilterMethod = LINEAR;
			break;
		case LINEAR:
			std::cout << "\033[1;32m(HARDWARE) Anisotropic Sampling\033[0m" << std::endl;
			m_CurrentFilterMethod = ANISOTROPIC;
			break;
		case ANISOTROPIC:
			std::cout << "\033[1;32m(HARDWARE) Point Sampling\033[0m" << std::endl;
			m_CurrentFilterMethod = POINT;
			break;
		}
	}

private:
	//Shading
	ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable{};
	ID3DX11EffectShaderResourceVariable* m_pNormalMapVariable{};
	ID3DX11EffectShaderResourceVariable* m_pGlossMapVariable{};
	ID3DX11EffectShaderResourceVariable* m_pSpecularMapVariable{};

	//Matrices
	ID3DX11EffectMatrixVariable* m_pMatWorldVariable{};
	ID3DX11EffectMatrixVariable* m_pMatInverseViewVariable{};

	//Sampling
	ID3D11SamplerState* m_pSamplerStateLinear{};
	ID3D11SamplerState* m_pSamplerStatePoint{};
	ID3D11SamplerState* m_pSamplerStateAnisotropic{};

	FilteringMethod m_CurrentFilterMethod{ FilteringMethod::POINT };
	ID3DX11EffectSamplerVariable* m_pEffectSamplerVariable{};
};