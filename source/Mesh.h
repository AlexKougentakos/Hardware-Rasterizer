#pragma once
#include "pch.h"
#include "Effect.h"
#include "Math.h"
#include "Camera.h"
#include "Texture.h"
#include "Utils.h"
#include "DataTypes.h"

using namespace dae;

class Mesh
{
public:

	Mesh(ID3D11Device* pDevice, Effect* pEffect, std::string objPath)
	{
		m_pEffect = pEffect;
		m_pEffect->CreateInputLayout(pDevice);

		Utils::ParseOBJ(objPath, m_Vertices, m_Indices);
		m_SoftwareVertives.resize(m_Vertices.size());
		for (int i{0}; i < m_Vertices.size(); ++i)
		{
			m_SoftwareVertives[i].position = m_Vertices[i].position;
			m_SoftwareVertives[i].normal = m_Vertices[i].normal;
			m_SoftwareVertives[i].tangent = m_Vertices[i].tangent;
			m_SoftwareVertives[i].uv = m_Vertices[i].uv;
		}

		//Create vertex buffer
		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.ByteWidth = sizeof(Vertex_PosTex) * static_cast<uint32_t>(m_Vertices.size());
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = m_Vertices.data();

		HRESULT result = pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
		if (FAILED(result))
			return;

		//Create index buffer
		m_NumIndices = static_cast<uint32_t>(m_Indices.size());
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.ByteWidth = sizeof(uint32_t) * m_NumIndices;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		bd.MiscFlags = 0;
		initData.pSysMem = m_Indices.data();

		result = pDevice->CreateBuffer(&bd, &initData, &m_pIndexBuffer);
		if (FAILED(result))
			return;
	}
	~Mesh()
	{
		delete m_pEffect;
		m_pIndexBuffer->Release();
		m_pVertexBuffer->Release();
		
	}
	
	void Render(ID3D11DeviceContext* pDeviceContext, dae::Camera* pCamera) const
	{
		//1 Set Primitive Topology
		pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//2 Set Input Layout
		pDeviceContext->IASetInputLayout(m_pEffect->GetInputLayout());

		//3 Set VertexBuffer
		constexpr UINT stride = sizeof(Vertex_PosTex);
		constexpr UINT offset = 0;
		pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

		//4 Set IndexBuffer
		pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		//Update the Effect
		m_pEffect->UpdateEffect();

		//5 Draw
		D3DX11_TECHNIQUE_DESC techDesc{};
		m_pEffect->GetTechnique()->GetDesc(&techDesc);
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			m_pEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, pDeviceContext);
			pDeviceContext->DrawIndexed(m_NumIndices, 0, 0);
		}
	}

	void CycleCurrentFilteringTechnique() const
	{
		m_pEffect->CycleCurrentFilteringTechnique();
	}

	void InitializeMeshMatrices(Vector3 position, Vector3 rotation, Vector3 scale)
	{
		m_ScaleMatrix = Matrix::CreateScale(scale);
		m_RotationMatrix = Matrix::CreateRotation(rotation);
		m_TranslationMatrix = Matrix::CreateTranslation(position);
	}

	void UpdateMeshMatrices(const Matrix& viewProjectionMatrix, const Matrix& inverseViewMatrix)
	{
		worldMatrix = { m_ScaleMatrix * m_RotationMatrix * m_TranslationMatrix };
		const Matrix wolrdViewProjMat{ worldMatrix * viewProjectionMatrix };
		const Matrix inverseViewMat{ inverseViewMatrix };

		m_pEffect->SetMatrix(worldMatrix, Matrix::worldMatrix);
		m_pEffect->SetMatrix(wolrdViewProjMat, Matrix::worldViewProjectionMatrix);
		m_pEffect->SetMatrix(inverseViewMat, Matrix::inverseViewMatrix);
	}

	void RotateY(float angle)
	{
		m_RotationMatrix *= Matrix::CreateRotationY(angle);
	}

	//Todo: Make a getter for these
	std::vector<Vertex_PosTex> m_Vertices{};
	std::vector<uint32_t> m_Indices{};

	std::vector<Vertex> m_SoftwareVertives{};
	std::vector<Vertex_Out> m_VerticesOut{};

	Matrix worldMatrix{};

private:
	Effect* m_pEffect{};
	ID3D11Buffer* m_pVertexBuffer{};
	ID3D11Buffer* m_pIndexBuffer{};
	uint32_t m_NumIndices{};

	Matrix m_TranslationMatrix{ Vector3::UnitX, Vector3::UnitY, Vector3::UnitZ, Vector3::Zero };
	Matrix m_RotationMatrix{ Vector3::UnitX, Vector3::UnitY, Vector3::UnitZ, Vector3::Zero };
	Matrix m_ScaleMatrix{ Vector3::UnitX, Vector3::UnitY, Vector3::UnitZ, Vector3::Zero };

};