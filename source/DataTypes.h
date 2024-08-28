#pragma once
using namespace dae;
struct Vertex_PosCol
{
	Vector3	position{};
	Vector3 color{};
};

struct Vertex_PosTex
{
	Vector3	position{};
	Vector3 worldPosition{};
	Vector3 normal{};
	Vector3 tangent{};
	Vector2 uv{};
};

struct Vertex
{
	Vector3 position{};
	ColorRGB color{ colors::White };
	Vector2 uv{}; //W3
	Vector3 normal{}; //W4
	Vector3 tangent{}; //W4
	Vector3 viewDirection{}; //W4
};

struct Vertex_Out
{
	Vector4 position{};
	ColorRGB color{ colors::White };
	Vector2 uv{};
	Vector3 normal{};
	Vector3 tangent{};
	Vector3 viewDirection{};
};
