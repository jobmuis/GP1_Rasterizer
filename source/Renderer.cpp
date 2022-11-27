//External includes
#include "SDL.h"
#include "SDL_surface.h"
#include <iostream>

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

	//Initialize Texture
	m_pTexture = Texture::LoadFromFile("resources/uv_grid_2.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//RENDER LOGIC
	//Render_W1_Part1();
	//Render_W1_Part2();
	//Render_W1_Part3();
	//Render_W1_Part4();
	//Render_W1_Part5();
	//Render_W2_Part1();
	//Render_W2_Part2TriangleList();
	//Render_W2_Part2TriangleStrip();
	Render_W2_UVCoordinates();

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	const float aspectRatio{ float(m_Width) / float(m_Height) };
	
	for (const Vertex& vertexWorldspace : vertices_in)
	{
		//World space to View space
		Vertex vertexViewspace{};
		vertexViewspace.position = m_Camera.viewMatrix.TransformPoint(vertexWorldspace.position);
		//Apply perspective divide
		Vertex vertexProjected{};
		vertexProjected.position.x = vertexViewspace.position.x / vertexViewspace.position.z;
		vertexProjected.position.y = vertexViewspace.position.y / vertexViewspace.position.z;
		vertexProjected.position.z = vertexViewspace.position.z;
		//Relative to screen
		vertexProjected.position.x /= (aspectRatio * m_Camera.fov);
		vertexProjected.position.y /= m_Camera.fov;
		vertexProjected.color = vertexWorldspace.color;
		vertexProjected.position.x = ((vertexProjected.position.x + 1) / 2.f) * m_Width;
		vertexProjected.position.y = ((1 - vertexProjected.position.y) / 2.f) * m_Height;
		vertices_out.emplace_back(vertexProjected);
	}
}

void Renderer::VertexTransformationFunction(const std::vector<Mesh>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	const float aspectRatio{ float(m_Width) / float(m_Height) };

	for (const Mesh& mesh : vertices_in)
	{
		for (const Vertex& vertexWorldspace : mesh.vertices)
		{
			//World space to View space
			Vertex vertexViewspace{};
			vertexViewspace.position = m_Camera.viewMatrix.TransformPoint(vertexWorldspace.position);
			//Apply perspective divide
			Vertex vertexProjected{};
			vertexProjected.position.x = vertexViewspace.position.x / vertexViewspace.position.z;
			vertexProjected.position.y = vertexViewspace.position.y / vertexViewspace.position.z;
			vertexProjected.position.z = vertexViewspace.position.z;
			//Relative to screen
			vertexProjected.position.x /= (aspectRatio * m_Camera.fov);
			vertexProjected.position.y /= m_Camera.fov;
			vertexProjected.position.x = ((vertexProjected.position.x + 1) / 2.f) * m_Width;
			vertexProjected.position.y = ((1 - vertexProjected.position.y) / 2.f) * m_Height;
			//Copy color and uv
			vertexProjected.color = vertexWorldspace.color;
			vertexProjected.uv = vertexWorldspace.uv;
			vertices_out.emplace_back(vertexProjected);
		}
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void Renderer::Render_W2_UVCoordinates()
{
	//Define Mesh
	std::vector<Mesh> vertices_world
	{
		Mesh{
				{
					Vertex{{-3, 3, -2}, {}, {0, 0}},
					Vertex{{0, 3, -2}, {}, {.5f, 0}},
					Vertex{{3, 3, -2}, {},  {1, 0}},
					Vertex{{-3, 0, -2}, {},  {0, .5}},
					Vertex{{0, 0, -2}, {},  {.5, .5}},
					Vertex{{3, 0, -2}, {}, {1, .5}},
					Vertex{{-3, -3, -2}, {}, {0, 1}},
					Vertex{{0, -3, -2}, {}, {.5, 1}},
					Vertex{{3, -3, -2}, {}, {1, 1}}
				},
				{
					3, 0, 4, 1, 5, 2,
					2, 6,
					6, 3, 7, 4, 8, 5
				},
				PrimitiveTopology::TriangleStrip
		}
	};

	//std::cout << vertices_world[0].vertices[1].uv.x << '\n';
	
	std::vector<Vertex> vertices_ScreenSpace;
	vertices_ScreenSpace.reserve(vertices_world[0].vertices.size());

	VertexTransformationFunction(vertices_world, vertices_ScreenSpace);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//for each triangle
	for (int index{}; index < vertices_world[0].indices.size() - 2; ++index)
	{
		Vertex vertex0{}, vertex1{}, vertex2{};

		if (index % 2 == 0)
		{
			vertex0 = vertices_ScreenSpace[vertices_world[0].indices[index]];
			vertex1 = vertices_ScreenSpace[vertices_world[0].indices[index + 1]];
			vertex2 = vertices_ScreenSpace[vertices_world[0].indices[index + 2]];
		}
		else
		{
			vertex0 = vertices_ScreenSpace[vertices_world[0].indices[index]];
			vertex1 = vertices_ScreenSpace[vertices_world[0].indices[index + 2]];
			vertex2 = vertices_ScreenSpace[vertices_world[0].indices[index + 1]];
		}

		//triangle edges
		Vector2 edgeA{ Vector2(vertex0.position.x, vertex0.position.y),
					   Vector2(vertex1.position.x, vertex1.position.y) };
		Vector2 edgeB{ Vector2(vertex1.position.x, vertex1.position.y),
					   Vector2(vertex2.position.x, vertex2.position.y) };
		Vector2 edgeC{ Vector2(vertex2.position.x, vertex2.position.y),
					   Vector2(vertex0.position.x, vertex0.position.y) };

		float smallestX{ vertex0.position.x };
		smallestX = std::min(smallestX, vertex1.position.x);
		smallestX = std::min(smallestX, vertex2.position.x);

		float smallestY{ vertex0.position.y };
		smallestY = std::min(smallestY, vertex1.position.y);
		smallestY = std::min(smallestY, vertex2.position.y);

		float largestX{ vertex0.position.x };
		largestX = std::max(largestX, vertex1.position.x);
		largestX = std::max(largestX, vertex2.position.x);

		float largestY{ vertex0.position.y };
		largestY = std::max(largestY, vertex1.position.y);
		largestY = std::max(largestY, vertex2.position.y);

		Int2 pMin, pMax;
		pMin.x = Clamp(int(smallestX), 0, m_Width);
		pMin.y = Clamp(int(smallestY), 0, m_Height);
		pMax.x = Clamp(int(largestX), 0, m_Width);
		pMax.y = Clamp(int(largestY), 0, m_Height);

		//for every pixel
		for (int px{ pMin.x }; px <= pMax.x; ++px)
		{
			for (int py{ pMin.y }; py <= pMax.y; ++py)
			{
				const Vector2 pixel{ float(px), float(py) };

				Vector2 vertex0ToPixel{ Vector2(vertex0.position.x, vertex0.position.y), pixel };
				float crossA = Vector2::Cross(edgeA, vertex0ToPixel);

				Vector2 vertex1ToPixel{ Vector2(vertex1.position.x, vertex1.position.y), pixel };
				float crossB = Vector2::Cross(edgeB, vertex1ToPixel);

				Vector2 vertex2ToPixel{ Vector2(vertex2.position.x, vertex2.position.y), pixel };
				float crossC = Vector2::Cross(edgeC, vertex2ToPixel);

				ColorRGB finalColor{};

				//if pixel is inside triangle
				if (crossA > 0 && crossB > 0 && crossC > 0)
				{
					const float totalArea = Vector2::Cross(edgeA, edgeB);
					const float W0{ Vector2::Cross(edgeB, vertex1ToPixel) / totalArea };
					const float W1{ Vector2::Cross(edgeC, vertex2ToPixel) / totalArea };
					const float W2{ Vector2::Cross(edgeA, vertex0ToPixel) / totalArea };

					const float pixelDepth = vertex0.position.z * W0 + vertex1.position.z * W1 + vertex2.position.z * W2;

					const float zInterpolated = 1.f / ((1.f / vertex0.position.z) * W0 + (1.f / vertex1.position.z) * W1 + (1.f / vertex2.position.z) * W2);

					if (zInterpolated < m_pDepthBufferPixels[py * m_Width + px])
					{
						m_pDepthBufferPixels[py * m_Width + px] = zInterpolated;

						//finalColor = vertex0.color * W0 + vertex1.color * W1 + vertex2.color * W2;

						//Vector2 interpolatedUV{ vertex0.uv * W0 + vertex1.uv * W1 + vertex2.uv * W2 };
						Vector2 interpolatedUV = ((vertex0.uv / vertex0.position.z * W0) + (vertex1.uv / vertex1.position.z * W1)
							+ (vertex2.uv / vertex2.position.z * W2)) * zInterpolated;

						finalColor = m_pTexture->Sample(interpolatedUV);
						//std::cout << finalColor.r << ' ' << finalColor.g << ' ' << finalColor.b << '\n';

						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}
}

void Renderer::Render_W2_Part2TriangleStrip()
{
	//Define Mesh
	std::vector<Mesh> vertices_world
	{
		Mesh{
				{
					Vertex{{-3, 3, -2}, {0, 0}},
					Vertex{{0, 3, -2}, {.5, 0}},
					Vertex{{3, 3, -2}, {1, 0}},
					Vertex{{-3, 0, -2}, {0, .5}},
					Vertex{{0, 0, -2}, {.5, .5}},
					Vertex{{3, 0, -2}, {1, .5}},
					Vertex{{-3, -3, -2}, {0, 1}},
					Vertex{{0, -3, -2}, {.5, 1}},
					Vertex{{3, -3, -2}, {1, 1}}
				},
				{
					3, 0, 4, 1, 5, 2,
					2, 6,
					6, 3, 7, 4, 8, 5
				},
				PrimitiveTopology::TriangleStrip
		}
	};

	std::vector<Vertex> vertices_ScreenSpace;
	vertices_ScreenSpace.reserve(vertices_world[0].vertices.size());

	VertexTransformationFunction(vertices_world, vertices_ScreenSpace);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//std::cout << vertices_world[0].indices.size() << '\n';

	//for each triangle
	for (int index{}; index < vertices_world[0].indices.size() - 2; ++index)
	{
		Vertex vertex0{}, vertex1{}, vertex2{};

		if (index % 2 == 0)
		{
			vertex0 = vertices_ScreenSpace[vertices_world[0].indices[index]];
			vertex1 = vertices_ScreenSpace[vertices_world[0].indices[index + 1]];
			vertex2 = vertices_ScreenSpace[vertices_world[0].indices[index + 2]];
		}
		else
		{
			vertex0 = vertices_ScreenSpace[vertices_world[0].indices[index]];
			vertex1 = vertices_ScreenSpace[vertices_world[0].indices[index + 2]];
			vertex2 = vertices_ScreenSpace[vertices_world[0].indices[index + 1]];
		}

		//triangle edges
		Vector2 edgeA{ Vector2(vertex0.position.x, vertex0.position.y),
					   Vector2(vertex1.position.x, vertex1.position.y) };
		Vector2 edgeB{ Vector2(vertex1.position.x, vertex1.position.y),
					   Vector2(vertex2.position.x, vertex2.position.y) };
		Vector2 edgeC{ Vector2(vertex2.position.x, vertex2.position.y),
					   Vector2(vertex0.position.x, vertex0.position.y) };

		float smallestX{ vertex0.position.x };
		smallestX = std::min(smallestX, vertex1.position.x);
		smallestX = std::min(smallestX, vertex2.position.x);

		float smallestY{ vertex0.position.y };
		smallestY = std::min(smallestY, vertex1.position.y);
		smallestY = std::min(smallestY, vertex2.position.y);

		float largestX{ vertex0.position.x };
		largestX = std::max(largestX, vertex1.position.x);
		largestX = std::max(largestX, vertex2.position.x);

		float largestY{ vertex0.position.y };
		largestY = std::max(largestY, vertex1.position.y);
		largestY = std::max(largestY, vertex2.position.y);

		Int2 pMin, pMax;
		pMin.x = Clamp(int(smallestX), 0, m_Width);
		pMin.y = Clamp(int(smallestY), 0, m_Height);
		pMax.x = Clamp(int(largestX), 0, m_Width);
		pMax.y = Clamp(int(largestY), 0, m_Height);

		//for every pixel
		for (int px{ pMin.x }; px <= pMax.x; ++px)
		{
			for (int py{ pMin.y }; py <= pMax.y; ++py)
			{
				const Vector2 pixel{ float(px), float(py) };

				Vector2 vertex0ToPixel{ Vector2(vertex0.position.x, vertex0.position.y), pixel };
				float crossA = Vector2::Cross(edgeA, vertex0ToPixel);

				Vector2 vertex1ToPixel{ Vector2(vertex1.position.x, vertex1.position.y), pixel };
				float crossB = Vector2::Cross(edgeB, vertex1ToPixel);

				Vector2 vertex2ToPixel{ Vector2(vertex2.position.x, vertex2.position.y), pixel };
				float crossC = Vector2::Cross(edgeC, vertex2ToPixel);

				ColorRGB finalColor{};

				//if pixel is inside triangle
				if (crossA > 0 && crossB > 0 && crossC > 0)
				{
					const float totalArea = Vector2::Cross(edgeA, edgeB);
					const float W0{ Vector2::Cross(edgeB, vertex1ToPixel) / totalArea };
					const float W1{ Vector2::Cross(edgeC, vertex2ToPixel) / totalArea };
					const float W2{ Vector2::Cross(edgeA, vertex0ToPixel) / totalArea };

					const float pixelDepth = vertex0.position.z * W0 + vertex1.position.z * W1 + vertex2.position.z * W2;

					if (pixelDepth < m_pDepthBufferPixels[py * m_Width + px])
					{
						m_pDepthBufferPixels[py * m_Width + px] = pixelDepth;

						finalColor = vertex0.color * W0 + vertex1.color * W1 + vertex2.color * W2;

						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}
}

void Renderer::Render_W2_Part2TriangleList()
{
	//Define Mesh
	std::vector<Mesh> vertices_world
	{
		Mesh{
				{
					Vertex{{-3, 3, -2}},
					Vertex{{0, 3, -2}},
					Vertex{{3, 3, -2}},
					Vertex{{-3, 0, -2}},
					Vertex{{0, 0, -2}},
					Vertex{{3, 0, -2}},
					Vertex{{-3, -3, -2}},
					Vertex{{0, -3, -2}},
					Vertex{{3, -3, -2}}
				},
				{
					3, 0, 1,	1, 4, 3,	4, 1, 2,
					2, 5, 4,	6, 3, 4,	4, 7, 6,
					7, 4, 5,	5, 8, 7
				},
				PrimitiveTopology::TriangeList
		}
	};

	std::vector<Vertex> vertices_ScreenSpace;
	vertices_ScreenSpace.reserve(vertices_world[0].vertices.size());

	VertexTransformationFunction(vertices_world, vertices_ScreenSpace);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));


	//for each triangle
	for (int index{}; index < vertices_world[0].indices.size(); index += 3)
	{

		Vertex vertex0{ vertices_ScreenSpace[vertices_world[0].indices[index]] };
		Vertex vertex1{ vertices_ScreenSpace[vertices_world[0].indices[index + 1]] };
		Vertex vertex2{ vertices_ScreenSpace[vertices_world[0].indices[index + 2]] };

		//triangle edges
		Vector2 edgeA{ Vector2(vertex0.position.x, vertex0.position.y),
					   Vector2(vertex1.position.x, vertex1.position.y) };
		Vector2 edgeB{ Vector2(vertex1.position.x, vertex1.position.y),
					   Vector2(vertex2.position.x, vertex2.position.y) };
		Vector2 edgeC{ Vector2(vertex2.position.x, vertex2.position.y),
					   Vector2(vertex0.position.x, vertex0.position.y) };

		float smallestX{ vertex0.position.x };
		smallestX = std::min(smallestX, vertex1.position.x);
		smallestX = std::min(smallestX, vertex2.position.x);

		float smallestY{ vertex0.position.y };
		smallestY = std::min(smallestY, vertex1.position.y);
		smallestY = std::min(smallestY, vertex2.position.y);

		float largestX{ vertex0.position.x };
		largestX = std::max(largestX, vertex1.position.x);
		largestX = std::max(largestX, vertex2.position.x);

		float largestY{ vertex0.position.y };
		largestY = std::max(largestY, vertex1.position.y);
		largestY = std::max(largestY, vertex2.position.y);

		Int2 pMin, pMax;
		pMin.x = Clamp(int(smallestX), 0, m_Width);
		pMin.y = Clamp(int(smallestY), 0, m_Height);
		pMax.x = Clamp(int(largestX), 0, m_Width);
		pMax.y = Clamp(int(largestY), 0, m_Height);

		//for every pixel
		for (int px{ pMin.x }; px <= pMax.x; ++px)
		{
			for (int py{ pMin.y }; py <= pMax.y; ++py)
			{
				const Vector2 pixel{ float(px), float(py) };

				Vector2 vertex0ToPixel{ Vector2(vertex0.position.x, vertex0.position.y), pixel };
				float crossA = Vector2::Cross(edgeA, vertex0ToPixel);

				Vector2 vertex1ToPixel{ Vector2(vertex1.position.x, vertex1.position.y), pixel };
				float crossB = Vector2::Cross(edgeB, vertex1ToPixel);

				Vector2 vertex2ToPixel{ Vector2(vertex2.position.x, vertex2.position.y), pixel };
				float crossC = Vector2::Cross(edgeC, vertex2ToPixel);

				ColorRGB finalColor{};

				//if pixel is inside triangle
				if (crossA > 0 && crossB > 0 && crossC > 0)
				{
					const float totalArea = Vector2::Cross(edgeA, edgeB);
					const float W0{ Vector2::Cross(edgeB, vertex1ToPixel) / totalArea };
					const float W1{ Vector2::Cross(edgeC, vertex2ToPixel) / totalArea };
					const float W2{ Vector2::Cross(edgeA, vertex0ToPixel) / totalArea };

					const float pixelDepth = vertex0.position.z * W0 + vertex1.position.z * W1 + vertex2.position.z * W2;

					if (pixelDepth < m_pDepthBufferPixels[py * m_Width + px])
					{
						m_pDepthBufferPixels[py * m_Width + px] = pixelDepth;

						finalColor = vertex0.color * W0 + vertex1.color * W1 + vertex2.color * W2;

						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}
}

void Renderer::Render_W2_Part1()
{
	//Define Quad - Vertices in World space
	std::vector<Vertex> vertices_world
	{
		{{-3.f, 3.f, -2.f}, {1, 1, 1}},
		{{0.f, 3.f, -2.f}, {1, 1, 1}},
		{{3.f, 3.f, -2.f}, {1, 1, 1}},
		{{-3.f, 0.f, -2.f}, {1, 1, 1}},
		{{0.f, 0.f, -2.f}, {1, 1, 1}},
		{{3.f, 0.f, -2.f}, {1, 1, 1}},
		{{-3.f, -3.f, -2.f}, {1, 1, 1}},
		{{0.f, -3.f, -2.f}, {1, 1, 1}},
		{{3.f, -3.f, -2.f}, {1, 1, 1}}
	};

	std::vector<int> indices_quad
	{
		3, 0, 4,
		0, 1, 4,
		4, 1, 5,
		1, 2, 5,
		6, 3, 7,
		3, 4, 7,
		7, 4, 8,
		4, 5, 8
	};

	std::vector<Vertex> vertices_ScreenSpace;
	vertices_ScreenSpace.reserve(vertices_world.size());

	VertexTransformationFunction(vertices_world, vertices_ScreenSpace);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));


	//for each triangle
	for (int index{}; index < indices_quad.size(); index += 3)
	{
		
		Vertex vertex0{ vertices_ScreenSpace[indices_quad[index]] };
		Vertex vertex1{ vertices_ScreenSpace[indices_quad[index + 1]] };
		Vertex vertex2{ vertices_ScreenSpace[indices_quad[index + 2]] };

		//triangle edges
		Vector2 edgeA{ Vector2(vertex0.position.x, vertex0.position.y),
					   Vector2(vertex1.position.x, vertex1.position.y) };
		Vector2 edgeB{ Vector2(vertex1.position.x, vertex1.position.y),
					   Vector2(vertex2.position.x, vertex2.position.y) };
		Vector2 edgeC{ Vector2(vertex2.position.x, vertex2.position.y),
					   Vector2(vertex0.position.x, vertex0.position.y) };

		float smallestX{ vertex0.position.x };
		smallestX = std::min(smallestX, vertex1.position.x);
		smallestX = std::min(smallestX, vertex2.position.x);

		float smallestY{ vertex0.position.y };
		smallestY = std::min(smallestY, vertex1.position.y);
		smallestY = std::min(smallestY, vertex2.position.y);

		float largestX{ vertex0.position.x };
		largestX = std::max(largestX, vertex1.position.x);
		largestX = std::max(largestX, vertex2.position.x);

		float largestY{vertex0.position.y };
		largestY = std::max(largestY,vertex1.position.y);
		largestY = std::max(largestY, vertex2.position.y);

		Int2 pMin, pMax;
		pMin.x = Clamp(int(smallestX), 0, m_Width);
		pMin.y = Clamp(int(smallestY), 0, m_Height);
		pMax.x = Clamp(int(largestX), 0, m_Width);
		pMax.y = Clamp(int(largestY), 0, m_Height);

		//for every pixel
		for (int px{ pMin.x }; px <= pMax.x; ++px)
		{
			for (int py{ pMin.y }; py <= pMax.y; ++py)
			{
				const Vector2 pixel{ float(px), float(py) };

				Vector2 vertex0ToPixel{ Vector2(vertex0.position.x, vertex0.position.y), pixel };
				float crossA = Vector2::Cross(edgeA, vertex0ToPixel);

				Vector2 vertex1ToPixel{ Vector2(vertex1.position.x, vertex1.position.y), pixel };
				float crossB = Vector2::Cross(edgeB, vertex1ToPixel);

				Vector2 vertex2ToPixel{ Vector2(vertex2.position.x, vertex2.position.y), pixel };
				float crossC = Vector2::Cross(edgeC, vertex2ToPixel);

				ColorRGB finalColor{};

				//if pixel is inside triangle
				if (crossA > 0 && crossB > 0 && crossC > 0)
				{
					const float totalArea = Vector2::Cross(edgeA, edgeB);
					const float W0{ Vector2::Cross(edgeB, vertex1ToPixel) / totalArea };
					const float W1{ Vector2::Cross(edgeC, vertex2ToPixel) / totalArea };
					const float W2{ Vector2::Cross(edgeA, vertex0ToPixel) / totalArea };

					const float pixelDepth = vertex0.position.z * W0 + vertex1.position.z * W1 + vertex2.position.z * W2;

					if (pixelDepth < m_pDepthBufferPixels[py * m_Width + px])
					{
						m_pDepthBufferPixels[py * m_Width + px] = pixelDepth;

						finalColor = vertex0.color * W0 + vertex1.color * W1 + vertex2.color * W2;

						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}
}


void Renderer::Render_W1_Part5()
{
	//Define Triangle - Vertices in World space
	std::vector<Vertex> vertices_world
	{
		//Triangle 0
		{{0.f, 2.f, 0.f}, {1, 0, 0}},
		{{1.5f, -1.f, 0.f}, {1, 0, 0}},
		{{-1.5f, -1.f, 0.f}, {1, 0, 0}},

		//Triangle 1
		{{0.f, 4.f, 2.f}, {1, 0, 0}},
		{{3.f, -2.f, 2.f}, {0, 1, 0}},
		{{-3.f, -2.f, 2.f}, {0, 0, 1}}
	};

	std::vector<Vertex> vertices_ScreenSpace;
	vertices_ScreenSpace.reserve(vertices_world.size());

	VertexTransformationFunction(vertices_world, vertices_ScreenSpace);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//for each triangle
	for (int index{}; index < vertices_ScreenSpace.size(); index += 3)
	{
		Vertex vertex0{ vertices_ScreenSpace[index] };
		Vertex vertex1{ vertices_ScreenSpace[index + 1] };
		Vertex vertex2{ vertices_ScreenSpace[index + 2] };

		//check if pixel is inside triangle
		Vector2 edgeA{ Vector2(vertex0.position.x, vertex0.position.y),
					   Vector2(vertex1.position.x, vertex1.position.y) };
		Vector2 edgeB{ Vector2(vertex1.position.x, vertex1.position.y),
					   Vector2(vertex2.position.x, vertex2.position.y) };
		Vector2 edgeC{ Vector2(vertex2.position.x, vertex2.position.y),
					   Vector2(vertex0.position.x, vertex0.position.y) };

		float smallestX{ vertex0.position.x };
		smallestX = std::min(smallestX, vertex1.position.x);
		smallestX = std::min(smallestX, vertex2.position.x);

		float smallestY{ vertex0.position.y };
		smallestY = std::min(smallestY, vertex1.position.y);
		smallestY = std::min(smallestY, vertex2.position.y);

		float largestX{ vertex0.position.x };
		largestX = std::max(largestX, vertex1.position.x);
		largestX = std::max(largestX, vertex2.position.x);

		float largestY{ vertex0.position.y };
		largestY = std::max(largestY, vertex1.position.y);
		largestY = std::max(largestY, vertex2.position.y);

		Int2 pMin, pMax;
		pMin.x = Clamp(int(smallestX), 0, m_Width);
		pMin.y = Clamp(int(smallestY), 0, m_Height);
		pMax.x = Clamp(int(largestX), 0, m_Width);
		pMax.y = Clamp(int(largestY), 0, m_Height);

		//for every pixel
		for (int py{ pMin.y }; py <= pMax.y; ++py)
		{
			for (int px{ pMin.x }; px <= pMax.x; ++px)
			{
				const Vector2 pixel{ float(px), float(py) };

				Vector2 vertex0ToPixel{ Vector2(vertex0.position.x, vertex0.position.y), pixel };
				float crossA = Vector2::Cross(edgeA, vertex0ToPixel);

				Vector2 vertex1ToPixel{ Vector2(vertex1.position.x, vertex1.position.y), pixel };
				float crossB = Vector2::Cross(edgeB, vertex1ToPixel);

				Vector2 vertex2ToPixel{ Vector2(vertex2.position.x, vertex2.position.y), pixel };
				float crossC = Vector2::Cross(edgeC, vertex2ToPixel);

				ColorRGB finalColor{};

				//if pixel is inside triangle
				if (crossA > 0 && crossB > 0 && crossC > 0)
				{
					const float totalArea = Vector2::Cross(edgeA, edgeB);
					const float W0{ Vector2::Cross(edgeB, vertex1ToPixel) / totalArea };
					const float W1{ Vector2::Cross(edgeC, vertex2ToPixel) / totalArea };
					const float W2{ Vector2::Cross(edgeA, vertex0ToPixel) / totalArea };

					const float pixelDepth = vertex0.position.z * W0 + vertex1.position.z * W1 + vertex2.position.z * W2;

					if (pixelDepth < m_pDepthBufferPixels[py * m_Width + px])
					{
						m_pDepthBufferPixels[py * m_Width + px] = pixelDepth;

						finalColor = vertex0.color * W0 + vertex1.color * W1 + vertex2.color * W2;

						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}
}

void Renderer::Render_W1_Part4()
{
	//Define Triangle - Vertices in World space
	std::vector<Vertex> vertices_world
	{
		//Triangle 0
		{{0.f, 2.f, 0.f}, {1, 0, 0}},
		{{1.5f, -1.f, 0.f}, {1, 0, 0}},
		{{-1.5f, -1.f, 0.f}, {1, 0, 0}},
		
		//Triangle 1
		{{0.f, 4.f, 2.f}, {1, 0, 0}},
		{{3.f, -2.f, 2.f}, {0, 1, 0}},
		{{-3.f, -2.f, 2.f}, {0, 0, 1}}
	};

	std::vector<Vertex> vertices_ScreenSpace;
	vertices_ScreenSpace.reserve(vertices_world.size());

	VertexTransformationFunction(vertices_world, vertices_ScreenSpace);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	const int vertexSet{ 3 };
	const int triangleAmount{ int(vertices_ScreenSpace.size()) / 3 };

	//for each triangle
	for (int triangleCount{}; triangleCount < triangleAmount; ++triangleCount)
	{

		const int triangleIndex{ triangleCount * vertexSet };
		//check if pixel is inside triangle
		Vector2 edgeA{ Vector2(vertices_ScreenSpace[triangleIndex + 0].position.x, vertices_ScreenSpace[triangleIndex + 0].position.y),
					   Vector2(vertices_ScreenSpace[triangleIndex + 1].position.x, vertices_ScreenSpace[triangleIndex + 1].position.y) };
		Vector2 edgeB{ Vector2(vertices_ScreenSpace[triangleIndex + 1].position.x, vertices_ScreenSpace[triangleIndex + 1].position.y),
					   Vector2(vertices_ScreenSpace[triangleIndex + 2].position.x, vertices_ScreenSpace[triangleIndex + 2].position.y) };
		Vector2 edgeC{ Vector2(vertices_ScreenSpace[triangleIndex + 2].position.x, vertices_ScreenSpace[triangleIndex + 2].position.y),
					   Vector2(vertices_ScreenSpace[triangleIndex + 0].position.x, vertices_ScreenSpace[triangleIndex + 0].position.y) };

		//for every pixel
		for (int px{}; px < m_Width; ++px)
		{
			for (int py{}; py < m_Height; ++py)
			{
				const Vector2 pixel{ float(px), float(py) };

				Vector2 vertex0ToPixel{ Vector2(vertices_ScreenSpace[triangleIndex + 0].position.x, vertices_ScreenSpace[triangleIndex + 0].position.y), pixel };
				float crossA = Vector2::Cross(edgeA, vertex0ToPixel);

				Vector2 vertex1ToPixel{ Vector2(vertices_ScreenSpace[triangleIndex + 1].position.x, vertices_ScreenSpace[triangleIndex + 1].position.y), pixel };
				float crossB = Vector2::Cross(edgeB, vertex1ToPixel);

				Vector2 vertex2ToPixel{ Vector2(vertices_ScreenSpace[triangleIndex + 2].position.x, vertices_ScreenSpace[triangleIndex + 2].position.y), pixel };
				float crossC = Vector2::Cross(edgeC, vertex2ToPixel);

				ColorRGB finalColor{};

				//if pixel is inside triangle
				if (crossA > 0 && crossB > 0 && crossC > 0)
				{
					const float totalArea = Vector2::Cross(edgeA, edgeB);
					const float W0{ Vector2::Cross(edgeB, vertex1ToPixel) / totalArea };
					const float W1{ Vector2::Cross(edgeC, vertex2ToPixel) / totalArea };
					const float W2{ Vector2::Cross(edgeA, vertex0ToPixel) / totalArea };

					const float pixelDepth = vertices_ScreenSpace[triangleIndex + 0].position.z * W0 + vertices_ScreenSpace[triangleIndex + 1].position.z * W1 
											+ vertices_ScreenSpace[triangleIndex + 2].position.z * W2;

					if (pixelDepth < m_pDepthBufferPixels[py * m_Width + px])
					{
						m_pDepthBufferPixels[py * m_Width + px] = pixelDepth;
						
						finalColor = vertices_ScreenSpace[triangleIndex + 0].color * W0 + vertices_ScreenSpace[triangleIndex + 1].color * W1
							+ vertices_ScreenSpace[triangleIndex + 2].color * W2;

						//Update Color in Buffer
						finalColor.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColor.r * 255),
							static_cast<uint8_t>(finalColor.g * 255),
							static_cast<uint8_t>(finalColor.b * 255));
					}
				}
			}
		}
	}
}

void Renderer::Render_W1_Part3()
{
	//Define Triangle - Vertices in World space
	std::vector<Vertex> vertices_world
	{
		{{0.f, 4.f, 2.f}, {1, 0, 0}},
		{{3.f, -2.f, 2.f}, {0, 1, 0}},
		{{-3.f, -2.f, 2.f}, {0, 0, 1}}
	};

	std::vector<Vertex> vertices_projected;
	vertices_projected.reserve(vertices_world.size());

	VertexTransformationFunction(vertices_world, vertices_projected);

	//fill vector of vertices with screenspace coordinates for x and y
	std::vector<Vertex> vertices_ScreenSpace{};
	for (const Vertex& vertex : vertices_projected)
	{
		float screenSpaceX = ((vertex.position.x + 1) / 2.f) * m_Width;
		float screenSpaceY = ((1 - vertex.position.y) / 2.f) * m_Height;
		vertices_ScreenSpace.push_back({ {screenSpaceX, screenSpaceY, vertex.position.z}, {vertex.color} });
	}

	//for every pixel
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			//check if pixel is inside triangle
			const Vector2 pixel{ float(px), float(py) };
			Vector2 edgeA{ Vector2(vertices_ScreenSpace[0].position.x, vertices_ScreenSpace[0].position.y), 
						   Vector2(vertices_ScreenSpace[1].position.x, vertices_ScreenSpace[1].position.y) };
			Vector2 edgeB{ Vector2(vertices_ScreenSpace[1].position.x, vertices_ScreenSpace[1].position.y), 
						   Vector2(vertices_ScreenSpace[2].position.x, vertices_ScreenSpace[2].position.y) };
			Vector2 edgeC{ Vector2(vertices_ScreenSpace[2].position.x, vertices_ScreenSpace[2].position.y), 
						   Vector2(vertices_ScreenSpace[0].position.x, vertices_ScreenSpace[0].position.y) };

			Vector2 vertex0ToPixel{ Vector2(vertices_ScreenSpace[0].position.x, vertices_ScreenSpace[0].position.y), pixel };
			float crossA = Vector2::Cross(edgeA, vertex0ToPixel);

			Vector2 vertex1ToPixel{ Vector2(vertices_ScreenSpace[1].position.x, vertices_ScreenSpace[1].position.y), pixel };
			float crossB = Vector2::Cross(edgeB, vertex1ToPixel);

			Vector2 vertex2ToPixel{ Vector2(vertices_ScreenSpace[2].position.x, vertices_ScreenSpace[2].position.y), pixel };
			float crossC = Vector2::Cross(edgeC, vertex2ToPixel);

			ColorRGB finalColor{};

			if (crossA > 0 && crossB > 0 && crossC > 0)
			{
				const float totalArea = Vector2::Cross(edgeA, edgeB);
				std::cout << totalArea << '\n';
				const float W0{ Vector2::Cross(edgeB, vertex1ToPixel) / totalArea };
				const float W1{ Vector2::Cross(edgeC, vertex2ToPixel) / totalArea };
				const float W2{ Vector2::Cross(edgeA, vertex0ToPixel) / totalArea };
					
				finalColor = vertices_ScreenSpace[0].color * W0 + vertices_ScreenSpace[1].color * W1 + vertices_ScreenSpace[2].color * W2;
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void Renderer::Render_W1_Part2()
{
	//Define Triangle - Vertices in World space
	std::vector<Vertex> vertices_world
	{
		{{0.f, 2.f, 0.f}},
		{{1.f, .0f, .0f}},
		{{-1.f, 0.f, .0f}}
	};

	std::vector<Vertex> vertices_projected;
	vertices_projected.reserve(vertices_world.size());

	VertexTransformationFunction(vertices_world, vertices_projected);

	//fill vector of three vertices with screenspace coordinates
	std::vector<Vector2> vertices_ScreenSpace{};
	for (const Vertex& vertex : vertices_projected)
	{
		float screenSpaceX = ((vertex.position.x + 1) / 2.f) * m_Width;
		float screenSpaceY = ((1 - vertex.position.y) / 2.f) * m_Height;
		vertices_ScreenSpace.push_back({ screenSpaceX , screenSpaceY });
	}

	//for every pixel
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			//check if pixel is inside triangle
			Vector2 pixel{ float(px), float(py) };
			Vector2 edgeA{ vertices_ScreenSpace[0], vertices_ScreenSpace[1] };
			Vector2 edgeB{ vertices_ScreenSpace[1], vertices_ScreenSpace[2] };
			Vector2 edgeC{ vertices_ScreenSpace[2], vertices_ScreenSpace[0] };

			float crossA = Vector2::Cross(edgeA, Vector2(vertices_ScreenSpace[0], pixel));
			float crossB = Vector2::Cross(edgeB, Vector2(vertices_ScreenSpace[1], pixel));
			float crossC = Vector2::Cross(edgeC, Vector2(vertices_ScreenSpace[2], pixel));

			ColorRGB finalColor{};

			if (crossA > 0 && crossB > 0 && crossC > 0)
			{
				finalColor = { 1.f, 1.f, 1.f };
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

void Renderer::Render_W1_Part1()
{
	//Define Triangle - Vertices in NDC space
	std::vector<Vector3> vertices_ndc
	{
		{0.f, .5f, 1.f},
		{.5f, -.5f, 1.f},
		{-.5f, -.5f, 1.f}
	};
	
	//fill vector of three vertices with screenspace coordinates
	std::vector<Vector2> vertices_ScreenSpace{};
	for (const Vector3& vertex : vertices_ndc)
	{
		float screenSpaceX = ((vertex.x + 1) / 2.f) * m_Width;
		float screenSpaceY = ((1 - vertex.y) / 2.f) * m_Height;
		vertices_ScreenSpace.push_back({ screenSpaceX , screenSpaceY });
	}

	//for every pixel
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			//check if pixel is inside triangle
			Vector2 pixel{ float(px), float(py) };
			Vector2 edgeA{ vertices_ScreenSpace[0], vertices_ScreenSpace[1] };
			Vector2 edgeB{ vertices_ScreenSpace[1], vertices_ScreenSpace[2] };
			Vector2 edgeC{ vertices_ScreenSpace[2], vertices_ScreenSpace[0] };

			float crossA = Vector2::Cross(edgeA, Vector2(vertices_ScreenSpace[0], pixel));
			float crossB = Vector2::Cross(edgeB, Vector2(vertices_ScreenSpace[1], pixel));
			float crossC = Vector2::Cross(edgeC, Vector2(vertices_ScreenSpace[2], pixel));

			ColorRGB finalColor{};

			if (crossA > 0 && crossB > 0 && crossC > 0)
			{
				finalColor = { 1.f, 1.f, 1.f };
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}