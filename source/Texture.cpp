#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//TODO
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)
		
		//SDL_Surface* surface{ IMG_Load(path.c_str()) };
		
		return new Texture(IMG_Load(path.c_str()));
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//TODO
		//Sample the correct texel for the given uv
		
		Uint8 r{}, g{}, b{};

		size_t x{ static_cast<size_t>(uv.x * m_pSurface->w) };
		size_t y{ static_cast<size_t>(uv.y * m_pSurface->h) };

		//m_pBackBufferPixels[px + (py * m_Width)]
		Uint32 pixel{ m_pSurfacePixels[x + (y * m_pSurface->w)] };
		
		SDL_GetRGB(pixel, m_pSurface->format, &r, &g, &b);

		return ColorRGB{ float(r) / 255.f, float(g) / 255.f, float(b) / 255.f };
	}
}