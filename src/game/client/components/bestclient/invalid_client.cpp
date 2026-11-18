/* InvalidClient: high quality procedural cosmetics and effects.
 * All images are generated in memory - no external PNG assets are needed.
 */
#include "invalid_client.h"

#include <base/color.h>
#include <base/math.h>
#include <base/str.h>
#include <base/time.h>
#include <base/vmath.h>

#include <engine/client.h>
#include <engine/client/enums.h>
#include <engine/graphics.h>
#include <engine/image.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <game/client/gameclient.h>
#include <game/client/prediction/entities/character.h>
#include <game/collision.h>
#include <game/mapitems.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>

static float Clamp01(float V) { return std::clamp(V, 0.0f, 1.0f); }

static float Smooth(float A, float B, float X)
{
	const float T = Clamp01((X - A) / (B - A));
	return T * T * (3.0f - 2.0f * T);
}

static vec3 Mix(const vec3 &A, const vec3 &B, float T) { return A + (B - A) * T; }
static float Mix(float A, float B, float T) { return A + (B - A) * T; }

static float Hash2(int X, int Y)
{
	unsigned int H = (unsigned int)(X * 374761393 + Y * 668265263);
	H = (H ^ (H >> 13)) * 1274126177u;
	return (float)((H ^ (H >> 16)) & 0xffff) / 65535.0f;
}

static float SdfCapsule(vec2 P, vec2 A, vec2 B, float R)
{
	const vec2 PA = P - A;
	const vec2 BA = B - A;
	const float D = dot(BA, BA);
	const float H = D > 0.0f ? Clamp01(dot(PA, BA) / D) : 0.0f;
	return length(PA - BA * H) - R;
}

static float SdfEllipse(vec2 P, vec2 C, vec2 R)
{
	return (length((P - C) / R) - 1.0f) * std::min(R.x, R.y);
}

static float SdfRoundedBox(vec2 P, vec2 C, vec2 B, float R)
{
	const vec2 Q = abs(P - C) - B + vec2(R, R);
	return length(vec2(std::max(Q.x, 0.0f), std::max(Q.y, 0.0f))) + std::min(std::max(Q.x, Q.y), 0.0f) - R;
}

static CImageInfo NewImage(int W, int H)
{
	CImageInfo Image;
	Image.m_Width = W;
	Image.m_Height = H;
	Image.m_Format = CImageInfo::FORMAT_RGBA;
	Image.AllocateFillZero();
	return Image;
}

static void PutPixel(CImageInfo &Image, int X, int Y, const vec3 &Color, float Alpha)
{
	if(X < 0 || Y < 0 || X >= (int)Image.m_Width || Y >= (int)Image.m_Height)
		return;
	uint8_t *pPixel = Image.m_pData + (Y * (int)Image.m_Width + X) * 4;
	pPixel[0] = (uint8_t)std::clamp(Color.r * 255.0f, 0.0f, 255.0f);
	pPixel[1] = (uint8_t)std::clamp(Color.g * 255.0f, 0.0f, 255.0f);
	pPixel[2] = (uint8_t)std::clamp(Color.b * 255.0f, 0.0f, 255.0f);
	pPixel[3] = (uint8_t)std::clamp(Alpha * 255.0f, 0.0f, 255.0f);
}

CInvalidClient::CInvalidClient()
{
	OnReset();
}

void CInvalidClient::OnReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aLastEmote[i] = -1;
	m_PeeTime = 0.0f;
	m_LastPeeEmit = 0.0f;
	m_LastEmit = 0.0f;
	m_LastSmokeEmit = 0.0f;
	for(CParticle &P : m_aParticles)
		P = CParticle();
	m_NextParticle = 0;
}

void CInvalidClient::OnInit()
{
	GenerateTextures();
}

IGraphics::CTextureHandle CInvalidClient::MakeWingTexture(int Style)
{
	const int Size = 144;
	CImageInfo Image = NewImage(Size, Size);

	struct SWingStyle
	{
		int m_Count;
		float m_A0, m_A1;
		float m_Len0, m_Len1;
		float m_Width;
		vec3 m_Base, m_Tip, m_Glow;
	};
	static const SWingStyle s_aStyles[6] = {
		{13, 0.10f, 1.42f, 0.50f, 0.76f, 0.040f, vec3(0.72f, 0.78f, 0.98f), vec3(1.00f, 0.96f, 0.78f), vec3(1.00f, 0.88f, 0.55f)},
		{6, 0.04f, 1.50f, 0.44f, 0.72f, 0.080f, vec3(0.16f, 0.02f, 0.05f), vec3(0.88f, 0.10f, 0.14f), vec3(1.00f, 0.20f, 0.12f)},
		{9, 0.12f, 1.38f, 0.46f, 0.74f, 0.052f, vec3(0.08f, 0.38f, 0.72f), vec3(0.80f, 0.96f, 1.00f), vec3(0.40f, 0.90f, 1.00f)},
		{11, 0.08f, 1.48f, 0.42f, 0.70f, 0.044f, vec3(0.86f, 0.12f, 0.02f), vec3(1.00f, 0.88f, 0.16f), vec3(1.00f, 0.48f, 0.05f)},
		{12, 0.10f, 1.44f, 0.48f, 0.75f, 0.042f, vec3(0.12f, 0.04f, 0.36f), vec3(0.58f, 0.38f, 0.98f), vec3(0.82f, 0.40f, 1.00f)},
		{10, 0.14f, 1.34f, 0.44f, 0.70f, 0.056f, vec3(0.04f, 0.32f, 0.10f), vec3(0.62f, 0.96f, 0.34f), vec3(0.42f, 1.00f, 0.48f)},
	};
	const int StyleIdx = std::clamp(Style, 0, 5);
	const SWingStyle &Ws = s_aStyles[StyleIdx];
	const vec2 Root(0.14f, 0.16f);

	for(int Y = 0; Y < Size; Y++)
	{
		for(int X = 0; X < Size; X++)
		{
			const vec2 P((float)(X + 0.5) / Size, 1.0f - (float)(Y + 0.5) / Size);
			float BestDist = 1000.0f;
			float BestT = 0.0f;
			for(int i = 0; i < Ws.m_Count; i++)
			{
				const float T = (float)i / (float)(Ws.m_Count - 1);
				const float A = Ws.m_A0 + (Ws.m_A1 - Ws.m_A0) * T;
				const float Len = Mix(Ws.m_Len0, Ws.m_Len1, std::sin(pi * T));
				const vec2 End = Root + vec2(std::cos(A), std::sin(A)) * Len;
				const float Radius = Ws.m_Width * (1.0f - 0.45f * T);
				const float D = SdfCapsule(P, Root, End, Radius);
				if(D < BestDist)
				{
					BestDist = D;
					BestT = T;
				}
			}
			const float Body = 1.0f - Smooth(-0.007f, 0.006f, BestDist);
			const float Halo = (1.0f - Smooth(0.0f, 0.055f, BestDist)) * (1.0f - Body);
			vec3 Color = Mix(Ws.m_Base, Ws.m_Tip, Clamp01(BestT * 0.55f + P.y * 0.55f));
			Color += Ws.m_Glow * Halo * 0.55f;
			const float Sheen = 1.0f - Smooth(0.0f, Ws.m_Width * 0.55f, -BestDist);
			Color += vec3(1.0f, 1.0f, 1.0f) * Sheen * 0.16f;
			if(StyleIdx == 4 && Hash2(X / 3, Y / 3) > 0.985f)
				Color += vec3(1.0f, 1.0f, 1.0f) * 0.8f;
			PutPixel(Image, X, Y, Color, Clamp01(Body + Halo * 0.42f));
		}
	}
	return Graphics()->LoadTextureRawMove(Image, 0, "invalid_wing");
}

IGraphics::CTextureHandle CInvalidClient::MakeGlassesTexture(int Style)
{
	const int W = 160;
	const int H = 80;
	CImageInfo Image = NewImage(W, H);

	const vec3 Lenses[8] = {
		vec3(0.04f, 0.05f, 0.09f), vec3(0.02f, 0.02f, 0.03f), vec3(0.00f, 0.55f, 0.70f),
		vec3(0.95f, 0.35f, 0.62f), vec3(0.01f, 0.01f, 0.01f), vec3(0.85f, 0.06f, 0.06f),
		vec3(0.10f, 0.22f, 0.18f), vec3(0.55f, 0.25f, 0.95f)};
	const vec3 Frames[8] = {
		vec3(0.95f, 0.78f, 0.25f), vec3(0.12f, 0.12f, 0.15f), vec3(0.10f, 1.00f, 0.85f),
		vec3(1.00f, 0.55f, 0.75f), vec3(0.20f, 0.20f, 0.22f), vec3(0.20f, 0.55f, 1.00f),
		vec3(0.75f, 0.55f, 0.20f), vec3(1.00f, 0.95f, 0.45f)};
	const int Idx = std::clamp(Style, 0, 7);

	for(int Y = 0; Y < H; Y++)
	{
		for(int X = 0; X < W; X++)
		{
			const vec2 P((float)(X + 0.5) / W, 1.0f - (float)(Y + 0.5) / H);
			float LensMask = 0.0f;
			float FrameMask = 0.0f;
			vec3 Color = vec3(0.0f, 0.0f, 0.0f);

			if(Idx == 2) // visor
			{
				const float D = SdfEllipse(P, vec2(0.5f, 0.5f), vec2(0.47f, 0.13f));
				FrameMask = 1.0f - Smooth(-0.012f, 0.004f, D);
				LensMask = 1.0f - Smooth(-0.05f, -0.035f, D);
				Color = vec3(0.02f, 0.35f, 0.40f) + vec3(0.0f, 0.8f, 0.7f) * Hash2(X / 5, Y);
			}
			else if(Idx == 4) // ninja
			{
				const float Band = 1.0f - Smooth(0.0f, 0.02f, std::abs(P.y - 0.5f) - 0.20f);
				const float Slit = 1.0f - Smooth(0.0f, 0.015f, std::abs(P.y - 0.48f) - 0.075f);
				FrameMask = Band;
				Color = vec3(0.03f, 0.03f, 0.03f) * Band;
				if(Slit > 0.0f && P.x > 0.12f && P.x < 0.88f)
				{
					Color = vec3(0.95f, 0.78f, 0.55f);
					LensMask = Slit;
				}
			}
			else
			{
				for(int Side = 0; Side < 2; Side++)
				{
					const vec2 C(0.5f + (Side == 0 ? -0.235f : 0.235f), 0.5f);
					vec2 R(0.185f, 0.30f);
					if(Idx == 1)
						R = vec2(0.205f, 0.245f);
					else if(Idx == 3)
						R = vec2(0.175f, 0.175f);
					else if(Idx == 6)
						R = vec2(0.150f, 0.150f);
					const float D = SdfEllipse(P, C, R);
					const float Outer = 1.0f - Smooth(-0.018f, 0.006f, D);
					const float Inner = 1.0f - Smooth(-0.045f, -0.028f, D);
					FrameMask = std::max(FrameMask, Outer);
					LensMask = std::max(LensMask, Inner);
					Color = Mix(Color, Lenses[Idx], Inner);
					if(Outer - Inner > 0.0f)
						Color = Frames[Idx];
				}
				const float Bridge = 1.0f - Smooth(0.0f, 0.012f, std::abs(P.y - 0.52f) - 0.022f);
				if(P.x > 0.40f && P.x < 0.60f)
				{
					FrameMask = std::max(FrameMask, Bridge);
					if(Bridge > 0.0f)
						Color = Frames[Idx];
				}
			}
			const float Alpha = std::max(FrameMask, LensMask * 0.92f);
			vec3 Final = Color;
			if(LensMask > FrameMask)
				Final += vec3(0.35f, 0.40f, 0.55f) * (1.0f - Clamp01((float)Y / H)) * 0.35f;
			PutPixel(Image, X, Y, Final, Clamp01(Alpha));
		}
	}
	return Graphics()->LoadTextureRawMove(Image, 0, "invalid_glasses");
}

IGraphics::CTextureHandle CInvalidClient::MakeHatTexture(int Style)
{
	const int W = 128;
	const int H = 96;
	CImageInfo Image = NewImage(W, H);
	const int Idx = std::clamp(Style, 0, 5);

	for(int Y = 0; Y < H; Y++)
	{
		for(int X = 0; X < W; X++)
		{
			const vec2 P((float)(X + 0.5) / W, 1.0f - (float)(Y + 0.5) / H);
			float Mask = 0.0f;
			vec3 Color = vec3(0.0f, 0.0f, 0.0f);
			switch(Idx)
			{
			case 0: // crown
			{
				const float Body = (1.0f - Smooth(0.0f, 0.008f, std::abs(P.x - 0.5f) - 0.24f)) * (1.0f - Smooth(0.0f, 0.008f, std::abs(P.y - 0.30f) - 0.14f));
				float Spikes = 0.0f;
				for(int i = -2; i <= 2; i++)
				{
					const vec2 C(0.5f + i * 0.105f, 0.47f);
					Spikes = std::max(Spikes, 1.0f - Smooth(0.0f, 0.008f, length((P - C) / vec2(0.045f, 0.24f)) - 1.0f));
				}
				Mask = Body;
				Color = Mix(vec3(0.85f, 0.60f, 0.08f), vec3(1.0f, 0.92f, 0.35f), P.y);
				if(Spikes > 0.0f && P.y > 0.40f)
				{
					Mask = Spikes;
					Color = vec3(1.0f, 0.88f, 0.20f);
				}
				if(Body > 0.0f && P.x > 0.28f && P.x < 0.72f && P.y > 0.25f && P.y < 0.34f)
				{
					const float Gem = 1.0f - Smooth(0.0f, 0.008f, length((P - vec2(0.5f, 0.295f)) / vec2(0.045f, 0.045f)) - 1.0f);
					if(Gem > 0.0f) { Mask = Gem; Color = vec3(0.20f, 0.85f, 1.0f); }
				}
				break;
			}
			case 1: // wizard
			{
				const float Cone = 1.0f - Smooth(0.0f, 0.010f, std::abs(P.x - 0.5f) - (0.05f + (0.40f - P.y) * 0.42f));
				const float InCone = Cone * (1.0f - Smooth(0.0f, 0.01f, 0.22f - P.y)) * Smooth(0.0f, 0.02f, P.y - 0.62f);
				const float Brim = (1.0f - Smooth(0.0f, 0.008f, std::abs(P.y - 0.22f) - 0.035f)) * (1.0f - Smooth(0.0f, 0.01f, std::abs(P.x - 0.5f) - 0.44f));
				Mask = std::max(InCone, Brim);
				Color = Mix(vec3(0.10f, 0.08f, 0.35f), vec3(0.42f, 0.30f, 0.85f), P.y);
				if(Hash2(X / 6, Y / 6) > 0.90f)
					Color += vec3(0.9f, 0.85f, 0.4f) * (float)(Hash2(X, Y) > 0.5f);
				break;
			}
			case 2: // cap
			{
				const float Dome = (1.0f - Smooth(0.0f, 0.010f, length((P - vec2(0.5f, 0.34f)) / vec2(0.30f, 0.22f)) - 1.0f)) * Smooth(0.0f, 0.01f, P.y - 0.34f);
				const float Visor = (1.0f - Smooth(0.0f, 0.008f, length((P - vec2(0.5f, 0.30f)) / vec2(0.42f, 0.09f)) - 1.0f)) * (1.0f - Smooth(0.0f, 0.01f, P.y - 0.32f));
				Mask = std::max(Dome, Visor);
				Color = Mix(vec3(0.15f, 0.45f, 0.85f), vec3(0.35f, 0.75f, 1.0f), P.y);
				break;
			}
			case 3: // halo
			{
				const float D = length((P - vec2(0.5f, 0.55f)) / vec2(0.34f, 0.075f));
				Mask = (1.0f - Smooth(0.0f, 0.06f, std::abs(D - 1.0f)));
				Color = vec3(1.0f, 0.92f, 0.45f) + vec3(0.4f, 0.3f, 0.0f) * (1.0f - std::abs(D - 1.0f) * 4.0f);
				break;
			}
			case 4: // cat ears
			{
				for(int Side = -1; Side <= 1; Side += 2)
				{
					const vec2 C(0.5f + Side * 0.26f, 0.42f);
					const float Tri = 1.0f - Smooth(0.0f, 0.010f, std::abs(P.x - C.x) - (0.20f - (P.y - 0.22f) * 0.55f));
					const float Ear = Tri * (1.0f - Smooth(0.0f, 0.01f, 0.22f - P.y)) * Smooth(0.0f, 0.01f, P.y - 0.72f);
					if(Ear > Mask)
					{
						Mask = Ear;
						Color = Mix(vec3(0.85f, 0.45f, 0.65f), vec3(1.0f, 0.75f, 0.85f), P.y);
						const float Inner = 1.0f - Smooth(0.0f, 0.01f, std::abs(P.x - C.x) - (0.10f - (P.y - 0.30f) * 0.45f));
						if(Inner > 0.5f && P.y < 0.68f)
							Color = vec3(1.0f, 0.55f, 0.70f);
					}
				}
				break;
			}
			default: // top hat
			{
				const float Top = (1.0f - Smooth(0.0f, 0.008f, std::abs(P.x - 0.5f) - 0.21f)) * (1.0f - Smooth(0.0f, 0.008f, std::abs(P.y - 0.45f) - 0.24f));
				const float Brim = (1.0f - Smooth(0.0f, 0.008f, std::abs(P.y - 0.20f) - 0.030f)) * (1.0f - Smooth(0.0f, 0.01f, std::abs(P.x - 0.5f) - 0.43f));
				Mask = std::max(Top, Brim);
				Color = vec3(0.06f, 0.06f, 0.08f) + vec3(0.12f, 0.12f, 0.12f) * P.y;
				if(Top > 0.0f && P.y > 0.38f && P.y < 0.45f)
					Color = vec3(0.75f, 0.10f, 0.12f);
				break;
			}
			}
			PutPixel(Image, X, Y, Color, Clamp01(Mask));
		}
	}
	return Graphics()->LoadTextureRawMove(Image, 0, "invalid_hat");
}

IGraphics::CTextureHandle CInvalidClient::MakeSoftCircle(const char *pName, float R, float G, float B, float Core)
{
	const int S = 64;
	CImageInfo Image = NewImage(S, S);
	for(int Y = 0; Y < S; Y++)
	{
		for(int X = 0; X < S; X++)
		{
			const vec2 P(((X + 0.5f) / S) * 2.0f - 1.0f, ((Y + 0.5f) / S) * 2.0f - 1.0f);
			const float D = length(P);
			const float A = (1.0f - Smooth(Core, 1.0f, D)) * (1.0f - Smooth(0.96f, 1.0f, D));
			PutPixel(Image, X, Y, vec3(R, G, B), Clamp01(A));
		}
	}
	return Graphics()->LoadTextureRawMove(Image, 0, pName);
}

IGraphics::CTextureHandle CInvalidClient::MakeSadTexture()
{
	const int S = 64;
	CImageInfo Image = NewImage(S, S);
	for(int Y = 0; Y < S; Y++)
	{
		for(int X = 0; X < S; X++)
		{
			const vec2 P(((X + 0.5f) / S) * 2.0f - 1.0f, ((Y + 0.5f) / S) * 2.0f - 1.0f);
			const float D = length(P);
			float Mask = 0.0f;
			vec3 Color = vec3(0.0f, 0.0f, 0.0f);
			const float Body = 1.0f - Smooth(0.0f, 0.04f, D - 0.86f);
			if(Body > 0.0f)
			{
				Mask = Body;
				Color = vec3(1.0f, 0.92f, 0.25f);
				const float Eye = std::min(1.0f - Smooth(0.0f, 0.03f, length((P - vec2(-0.32f, 0.25f)) / 0.17f) - 1.0f),
					1.0f - Smooth(0.0f, 0.03f, length((P - vec2(0.32f, 0.25f)) / 0.17f) - 1.0f));
				const float Mouth = (1.0f - Smooth(0.0f, 0.04f, std::abs(length(P - vec2(0.0f, -1.15f)) - 0.85f) - 0.05f)) * Smooth(0.0f, 0.02f, -P.y - 0.20f);
				if(Eye > 0.0f || Mouth > 0.0f) { Mask = std::max(Eye, Mouth); Color = vec3(0.15f, 0.10f, 0.05f); }
			}
			PutPixel(Image, X, Y, Color, Clamp01(Mask));
		}
	}
	return Graphics()->LoadTextureRawMove(Image, 0, "invalid_sad");
}

IGraphics::CTextureHandle CInvalidClient::MakeCigaretteTexture()
{
	const int W = 192;
	const int H = 64;
	CImageInfo Image = NewImage(W, H);
	for(int Y = 0; Y < H; Y++)
	{
		for(int X = 0; X < W; X++)
		{
			const vec2 P((float)(X + 0.5f) / W, 1.0f - (float)(Y + 0.5f) / H);
			vec3 Color(0.0f, 0.0f, 0.0f);
			float Alpha = 0.0f;

			// filter (left)
			const float FilterD = SdfRoundedBox(P, vec2(0.085f, 0.5f), vec2(0.075f, 0.135f), 0.045f);
			const float FilterMask = 1.0f - Smooth(-0.010f, 0.004f, FilterD);
			if(FilterMask > 0.0f)
			{
				vec3 Filter = vec3(0.78f, 0.58f, 0.33f);
				// fine cork speckles and a lighter band where paper meets filter
				const float Speck = Hash2(X / 2, Y / 2) * 0.16f - 0.08f;
				Filter += vec3(Speck, Speck * 0.8f, Speck * 0.5f);
				Filter *= 0.86f + 0.22f * (1.0f - std::abs(P.y - 0.5f) * 2.0f);
				if(P.x > 0.145f)
					Filter = Mix(Filter, vec3(0.97f, 0.95f, 0.90f), Smooth(0.145f, 0.165f, P.x));
				Color = Filter;
				Alpha = FilterMask;
			}

			// paper body
			const float PaperD = SdfRoundedBox(P, vec2(0.50f, 0.5f), vec2(0.355f, 0.115f), 0.040f);
			const float PaperMask = 1.0f - Smooth(-0.008f, 0.004f, PaperD);
			if(PaperMask > 0.0f)
			{
				// cylindrical shading: bright band slightly above the middle
				const float Shade = 1.0f - std::abs(P.y - 0.60f) * 1.15f;
				vec3 Paper = vec3(0.97f, 0.95f, 0.90f) * (0.80f + 0.26f * Clamp01(Shade));
				// faint paper fibres
				Paper += vec3(0.02f) * (Hash2(X, Y / 3) - 0.5f);
				Color = Mix(Color, Paper, PaperMask);
				Alpha = std::max(Alpha, PaperMask);
			}

			// charred tip and glowing ember (right)
			const float TipD = SdfRoundedBox(P, vec2(0.845f, 0.5f), vec2(0.030f, 0.105f), 0.030f);
			const float TipMask = 1.0f - Smooth(-0.010f, 0.004f, TipD);
			if(TipMask > 0.0f)
			{
				vec3 Ash = Mix(vec3(0.16f, 0.14f, 0.13f), vec3(0.72f, 0.34f, 0.10f), Smooth(0.83f, 0.875f, P.x));
				Ash = Mix(Ash, vec3(1.00f, 0.72f, 0.30f), Smooth(0.855f, 0.880f, P.x) * 0.75f);
				Color = Mix(Color, Ash, TipMask);
				Alpha = std::max(Alpha, TipMask);
			}

			// thin burnt seam between paper and ember
			const float Seam = 1.0f - Smooth(0.0f, 0.010f, std::abs(P.x - 0.815f) - 0.004f);
			if(Seam > 0.0f && P.y > 0.36f && P.y < 0.64f)
			{
				Color = Mix(Color, vec3(0.24f, 0.18f, 0.14f), Seam * 0.8f);
				Alpha = std::max(Alpha, Seam * 0.9f);
			}

			PutPixel(Image, X, Y, Color, Clamp01(Alpha));
		}
	}
	return Graphics()->LoadTextureRawMove(Image, 0, "invalid_cigarette");
}

IGraphics::CTextureHandle CInvalidClient::MakeArrowTexture()
{
	const int S = 96;
	CImageInfo Image = NewImage(S, S);
	for(int Y = 0; Y < S; Y++)
	{
		for(int X = 0; X < S; X++)
		{
			const vec2 P((float)(X + 0.5f) / S, 1.0f - (float)(Y + 0.5f) / S);
			// soft chevron pointing right
			const float D1 = SdfCapsule(P, vec2(0.26f, 0.20f), vec2(0.70f, 0.50f), 0.105f);
			const float D2 = SdfCapsule(P, vec2(0.26f, 0.80f), vec2(0.70f, 0.50f), 0.105f);
			const float D = std::min(D1, D2);
			const float Body = 1.0f - Smooth(-0.012f, 0.012f, D);
			const float Glow = (1.0f - Smooth(0.0f, 0.16f, D)) * (1.0f - Body);
			const vec3 Color = vec3(1.0f, 0.97f, 0.95f);
			PutPixel(Image, X, Y, Color, Clamp01(Body * 0.95f + Glow * 0.45f));
		}
	}
	return Graphics()->LoadTextureRawMove(Image, 0, "invalid_arrow");
}

IGraphics::CTextureHandle CInvalidClient::MakeRingTexture()
{
	const int S = 128;
	CImageInfo Image = NewImage(S, S);
	for(int Y = 0; Y < S; Y++)
	{
		for(int X = 0; X < S; X++)
		{
			const vec2 P((float)(X + 0.5f) / S, 1.0f - (float)(Y + 0.5f) / S);
			const float R = length(P - vec2(0.5f, 0.5f));
			const float RingD = std::abs(R - 0.335f) - 0.016f;
			const float Ring = 1.0f - Smooth(-0.010f, 0.010f, RingD);
			// soft bloom both inside and outside the ring
			const float Halo = std::max(0.0f, 1.0f - Smooth(0.0f, 0.13f, RingD)) * (1.0f - Ring);
			// subtle dashes so the ring reads as a target reticle
			const float A = std::atan2(P.y - 0.5f, P.x - 0.5f);
			const float Dash = 0.55f + 0.45f * std::sin(A * 9.0f);
			vec3 Color = vec3(1.0f, 1.0f, 1.0f);
			PutPixel(Image, X, Y, Color, Clamp01(Ring * (0.55f + 0.45f * Dash) + Halo * 0.40f));
		}
	}
	return Graphics()->LoadTextureRawMove(Image, 0, "invalid_ring");
}

IGraphics::CTextureHandle CInvalidClient::MakeVignetteTexture()
{
	const int S = 128;
	CImageInfo Image = NewImage(S, S);
	for(int Y = 0; Y < S; Y++)
	{
		for(int X = 0; X < S; X++)
		{
			const vec2 P((float)(X + 0.5f) / S, (float)(Y + 0.5f) / S);
			const vec2 Edge = vec2(std::min(P.x, 1.0f - P.x), std::min(P.y, 1.0f - P.y));
			const float Dist = std::min(Edge.x, Edge.y) * 2.0f;
			// smooth, slightly noisy red falloff so the border never looks like a rectangle
			const float Noise = Hash2(X / 4, Y / 4) * 0.10f - 0.05f;
			const float Falloff = std::pow(1.0f - Clamp01(Dist), 2.4f);
			const float Alpha = Clamp01((Falloff + Noise * Falloff) * 1.15f);
			const vec3 Color = Mix(vec3(0.95f, 0.06f, 0.08f), vec3(1.0f, 0.22f, 0.16f), Falloff);
			PutPixel(Image, X, Y, Color, Alpha);
		}
	}
	return Graphics()->LoadTextureRawMove(Image, 0, "invalid_vignette");
}

IGraphics::CTextureHandle CInvalidClient::MakeBlockGlowTexture()
{
	const int S = 64;
	CImageInfo Image = NewImage(S, S);
	for(int Y = 0; Y < S; Y++)
	{
		for(int X = 0; X < S; X++)
		{
			const vec2 P((float)(X + 0.5f) / S, 1.0f - (float)(Y + 0.5f) / S);
			const float Box = SdfRoundedBox(P, vec2(0.5f, 0.5f), vec2(0.34f, 0.34f), 0.09f);
			const float Fill = 1.0f - Smooth(-0.012f, 0.012f, Box);
			const float Edge = (1.0f - Smooth(0.0f, 0.016f, std::abs(Box) - 0.012f)) * (1.0f - Smooth(0.012f, 0.055f, Box));
			const float Bloom = (1.0f - Smooth(0.0f, 0.32f, Box)) * (1.0f - Fill);
			const vec3 Color = Mix(vec3(1.0f, 0.10f, 0.08f), vec3(1.0f, 0.74f, 0.42f), Edge * 0.85f);
			PutPixel(Image, X, Y, Color, Clamp01(Fill * 0.24f + Edge * 0.88f + Bloom * 0.36f));
		}
	}
	return Graphics()->LoadTextureRawMove(Image, 0, "invalid_block_glow");
}

void CInvalidClient::GenerateTextures()
{
	if(m_bTexturesReady)
		return;
	for(int i = 0; i < 6; i++)
		m_aWingTextures[i] = MakeWingTexture(i);
	for(int i = 0; i < 8; i++)
		m_aGlassesTextures[i] = MakeGlassesTexture(i);
	for(int i = 0; i < 6; i++)
		m_aHatTextures[i] = MakeHatTexture(i);
	m_SparkTexture = MakeSoftCircle("invalid_spark", 1.0f, 1.0f, 1.0f, 0.05f);
	m_SmokeTexture = MakeSoftCircle("invalid_smoke", 0.75f, 0.75f, 0.78f, 0.02f);
	m_TearTexture = MakeSoftCircle("invalid_tear", 0.45f, 0.80f, 1.0f, 0.20f);
	m_EmberTexture = MakeSoftCircle("invalid_ember", 1.0f, 0.45f, 0.10f, 0.10f);
	m_SadTexture = MakeSadTexture();
	m_CigaretteTexture = MakeCigaretteTexture();
	m_ArrowTexture = MakeArrowTexture();
	m_RingTexture = MakeRingTexture();
	m_VignetteTexture = MakeVignetteTexture();
	m_BlockGlowTexture = MakeBlockGlowTexture();
	m_bTexturesReady = true;
}

void CInvalidClient::RenderWings(vec2 BodyPos, vec2 Direction, float Alpha, float BaseSize, int ClientId)
{
	if(!m_bTexturesReady || !g_Config.m_ClInvalidWings)
		return;
	const int Style = std::clamp(g_Config.m_ClInvalidWingStyle, 0, 5);
	const float Time = Client()->GlobalTime();
	const float Speed = 3.0f + g_Config.m_ClInvalidWingFlap * 0.09f;
	const float Flap = std::sin(Time * Speed + ClientId * 0.9f);
	const float Facing = angle(Direction);

	Graphics()->TextureSet(m_aWingTextures[Style]);
	Graphics()->QuadsBegin();
	Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
	for(int Side = -1; Side <= 1; Side += 2)
	{
		Graphics()->QuadsSetRotation(Facing + Side * (0.18f + Flap * 0.20f));
		const vec2 Offset = vec2(Side * BaseSize * 0.22f, -BaseSize * 0.06f);
		const float Size = BaseSize * (2.15f + Flap * 0.12f);
		IGraphics::CQuadItem Item(BodyPos.x + Offset.x, BodyPos.y + Offset.y, Size, Size);
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha);
		Graphics()->QuadsDraw(&Item, 1);
	}
	Graphics()->QuadsEnd();

	if(g_Config.m_ClInvalidWingGlow > 0)
	{
		Graphics()->BlendAdditive();
		Graphics()->TextureSet(m_aWingTextures[Style]);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
		const float Glow = g_Config.m_ClInvalidWingGlow / 100.0f;
		for(int Side = -1; Side <= 1; Side += 2)
		{
			Graphics()->QuadsSetRotation(Facing + Side * (0.18f + Flap * 0.20f));
			const vec2 Offset = vec2(Side * BaseSize * 0.22f, -BaseSize * 0.06f);
			const float Size = BaseSize * 2.45f;
			IGraphics::CQuadItem Item(BodyPos.x + Offset.x, BodyPos.y + Offset.y, Size, Size);
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha * Glow * 0.14f);
			Graphics()->QuadsDraw(&Item, 1);
		}
		Graphics()->QuadsEnd();
		Graphics()->BlendNormal();
	}
	Graphics()->QuadsSetRotation(0.0f);
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CInvalidClient::RenderAccessories(vec2 BodyPos, vec2 Direction, float Alpha, float BaseSize, int ClientId, int Emote)
{
	if(!m_bTexturesReady || Alpha <= 0.0f || BaseSize <= 0.0f)
		return;
	const float Time = Client()->GlobalTime();
	const bool Local = GameClient()->m_Snap.m_LocalClientId == ClientId;

	// glasses
	if(g_Config.m_ClInvalidGlasses)
	{
		const int Style = std::clamp(g_Config.m_ClInvalidGlassesStyle, 0, 7);
		const float W = BaseSize * (Style == 2 ? 0.72f : 0.60f);
		const float H = W * 0.5f;
		const vec2 Offset = vec2(Direction.x * 0.10f, -0.06f + Direction.y * 0.08f) * BaseSize;
		Graphics()->TextureSet(m_aGlassesTextures[Style]);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
		Graphics()->QuadsSetRotation(0.0f);
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha);
		IGraphics::CQuadItem Item(BodyPos.x + Offset.x, BodyPos.y + Offset.y, W, H);
		Graphics()->QuadsDraw(&Item, 1);
		Graphics()->QuadsEnd();
	}

	// hat
	if(g_Config.m_ClInvalidHats && ClientId >= 0)
	{
		const int Style = std::clamp(g_Config.m_ClInvalidHatStyle, 0, 5);
		const float W = BaseSize * 0.85f;
		const float H = W * 0.75f;
		const float Bob = std::sin(Time * 3.0f + ClientId) * BaseSize * 0.012f;
		Graphics()->TextureSet(m_aHatTextures[Style]);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha);
		IGraphics::CQuadItem Item(BodyPos.x, BodyPos.y - BaseSize * 0.42f + Bob, W, H);
		Graphics()->QuadsDraw(&Item, 1);
		Graphics()->QuadsEnd();
	}

	// tears and sad smileys when a tee is hurt
	if(g_Config.m_ClInvalidTears && ClientId >= 0 && Emote == EMOTE_PAIN && m_aLastEmote[ClientId] != EMOTE_PAIN)
	{
		for(int i = 0; i < 4; i++)
			Emit(3, BodyPos + vec2((i % 2 == 0 ? -1 : 1) * BaseSize * 0.14f, -BaseSize * 0.05f),
				vec2((i % 2 == 0 ? -1 : 1) * 12.0f + (i - 1.5f) * 6.0f, -20.0f - i * 8.0f), 0.85f, BaseSize * 0.13f);
		Emit(4, BodyPos + vec2(0.0f, -BaseSize * 0.75f), vec2(0.0f, -28.0f), 1.4f, BaseSize * 0.55f);
	}
	if(ClientId >= 0)
		m_aLastEmote[ClientId] = Emote;

	// particle emitters (local tee keeps the effect tidy)
	if(Local && ClientId >= 0)
	{
		if(g_Config.m_ClInvalidCigarette)
			RenderCigarette(BodyPos, Direction, Alpha, BaseSize);
		if(m_PeeTime > 0.0f)
			RenderPee(BodyPos, Direction, Alpha, BaseSize);
		if(g_Config.m_ClInvalidSpray)
		{
			Emit(2, BodyPos + normalize(Direction) * BaseSize * 0.28f + vec2(0.0f, BaseSize * 0.10f),
				normalize(Direction) * 150.0f + vec2(0.0f, 40.0f), 0.55f, BaseSize * 0.10f);
		}
	}

	if(g_Config.m_ClInvalidTrail > 0 && ClientId >= 0 && Time - m_LastEmit > 0.025f)
	{
		m_LastEmit = Time;
		const int Style = std::clamp(g_Config.m_ClInvalidTrailStyle, 0, NUM_TRAIL_STYLES - 1);
		Emit(0, BodyPos + vec2(0.0f, BaseSize * 0.25f), vec2(0.0f, 8.0f), 0.55f, BaseSize * 0.18f);
		(void)Style;
	}
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CInvalidClient::RenderCigarette(vec2 BodyPos, vec2 Direction, float Alpha, float BaseSize)
{
	if(!m_bTexturesReady)
		return;
	const float Time = Client()->GlobalTime();
	const vec2 Dir = normalize(Direction);
	const float Angle = angle(Dir) - 0.55f;
	const vec2 TipDir(std::cos(Angle), std::sin(Angle));

	// The cigarette is one soft textured sprite (filter, paper and ember are baked
	// into the texture), so there are no visible rectangles at any zoom level.
	const float Length = BaseSize * 0.54f;
	const float Height = Length * (64.0f / 192.0f);
	const vec2 FilterEnd = BodyPos + Dir * BaseSize * 0.20f + vec2(0.0f, -BaseSize * 0.07f);
	const vec2 Center = FilterEnd + TipDir * (Length * 0.5f);
	const vec2 Tip = FilterEnd + TipDir * (Length * 0.845f);

	Graphics()->TextureSet(m_CigaretteTexture);
	Graphics()->QuadsBegin();
	Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
	Graphics()->QuadsSetRotation(Angle);
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha);
	IGraphics::CQuadItem Body(Center.x, Center.y, Length, Height);
	Graphics()->QuadsDraw(&Body, 1);
	Graphics()->QuadsSetRotation(0.0f);
	Graphics()->QuadsEnd();

	// glowing ember with a soft flicker
	Graphics()->BlendAdditive();
	Graphics()->TextureSet(m_EmberTexture);
	Graphics()->QuadsBegin();
	Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
	const float Flicker = 0.78f + 0.22f * std::sin(Time * 17.0f) * std::sin(Time * 5.1f);
	const float EmberSize = BaseSize * 0.16f;
	Graphics()->SetColor(1.0f, 0.55f, 0.18f, 0.85f * Flicker * Alpha);
	IGraphics::CQuadItem Ember(Tip.x - EmberSize * 0.5f, Tip.y - EmberSize * 0.5f, EmberSize, EmberSize);
	Graphics()->QuadsDraw(&Ember, 1);
	const float HaloSize = BaseSize * 0.46f;
	Graphics()->SetColor(1.0f, 0.20f, 0.05f, 0.20f * Alpha);
	IGraphics::CQuadItem Halo(Tip.x - HaloSize * 0.5f, Tip.y - HaloSize * 0.5f, HaloSize, HaloSize);
	Graphics()->QuadsDraw(&Halo, 1);
	Graphics()->QuadsEnd();
	Graphics()->BlendNormal();

	// drifting smoke
	if(Time - m_LastSmokeEmit > 0.055f)
	{
		m_LastSmokeEmit = Time;
		const float Jitter = Hash2((int)(Time * 700.0f), 3) - 0.5f;
		Emit(1, Tip, vec2(Jitter * 14.0f + Dir.x * 4.0f, -30.0f - Hash2((int)(Time * 500.0f), 7) * 10.0f),
			2.2f + Hash2((int)(Time * 300.0f), 11) * 0.7f, BaseSize * 0.20f);
		if(Hash2((int)(Time * 900.0f), 13) > 0.72f)
			Emit(0, Tip, vec2(Jitter * 40.0f, -55.0f), 0.5f, BaseSize * 0.06f);
	}
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CInvalidClient::RenderPee(vec2 BodyPos, vec2 Direction, float Alpha, float BaseSize)
{
	if(!g_Config.m_ClInvalidPee)
	{
		m_PeeTime = 0.0f;
		return;
	}
	const float Time = Client()->GlobalTime();
	const vec2 Dir = normalize(Direction);
	const vec2 Origin = BodyPos + vec2(Dir.x * BaseSize * 0.10f, BaseSize * 0.30f);
	const float Amount = std::clamp(g_Config.m_ClInvalidPeeAmount, 25, 300) / 100.0f;
	const vec2 StreamDir = normalize(vec2(Dir.x * 0.55f, 1.0f));

	// short translucent stream so the effect reads instantly even between droplets
	Graphics()->BlendAdditive();
	Graphics()->TextureSet(m_SmokeTexture);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.98f, 0.88f, 0.28f, 0.30f * Alpha);
	Graphics()->QuadsSetRotation(angle(StreamDir));
	IGraphics::CQuadItem Stream(Origin.x, Origin.y, BaseSize * 0.10f, BaseSize * 0.55f);
	Graphics()->QuadsDraw(&Stream, 1);
	Graphics()->QuadsSetRotation(0.0f);
	Graphics()->QuadsEnd();
	Graphics()->BlendNormal();

	if(Time - m_LastPeeEmit < 0.018f / std::max(0.4f, Amount))
		return;
	m_LastPeeEmit = Time;
	for(int i = 0; i < 2; i++)
	{
		const float Jitter = Hash2((int)(Time * 1300.0f), i * 7 + 1) - 0.5f;
		const float Spread = (Hash2((int)(Time * 900.0f), i * 3 + 5) - 0.5f) * 46.0f;
		Emit(5, Origin, vec2(Dir.x * 110.0f + Spread + Jitter * 18.0f, 150.0f + Jitter * 30.0f),
			0.55f + Hash2((int)(Time * 400.0f), i) * 0.25f, BaseSize * 0.085f);
	}
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CInvalidClient::Emit(int Style, vec2 Pos, vec2 Vel, float Life, float Size)
{
	CParticle &P = m_aParticles[m_NextParticle];
	m_NextParticle = (m_NextParticle + 1) % (int)std::size(m_aParticles);
	P.m_Pos = Pos;
	P.m_Vel = Vel;
	P.m_Life = Life;
	P.m_MaxLife = Life;
	P.m_Size = Size;
	P.m_Style = Style;
}

void CInvalidClient::ManageParticles(float Delta)
{
	for(CParticle &P : m_aParticles)
	{
		if(P.m_Life <= 0.0f)
			continue;
		P.m_Life -= Delta;
		P.m_Pos += P.m_Vel * Delta;
		if(P.m_Style == 1) // smoke rises and expands
		{
			P.m_Vel.y -= 6.0f * Delta;
			P.m_Vel.x += std::sin(P.m_Pos.x * 0.05f + P.m_Pos.y * 0.02f) * 10.0f * Delta;
			P.m_Size += 14.0f * Delta;
		}
		else
			P.m_Vel.y += 40.0f * Delta;
	}
}

void CInvalidClient::RenderFreezeWarning()
{
	if(!g_Config.m_ClInvalidFreezeGlow || Client()->State() != IClient::STATE_ONLINE)
		return;
	const int LocalId = GameClient()->m_Snap.m_LocalClientId;
	if(LocalId < 0)
		return;
	CCharacter *pChar = GameClient()->m_PredictedWorld.GetCharacterById(LocalId);
	if(!pChar)
		return;

	const CCharacterCore *pCore = pChar->Core();
	if(!pCore)
		return;
	const vec2 Pos = pCore->m_Pos;
	const vec2 Vel = pCore->m_Vel;
	const float Range = (float)g_Config.m_ClInvalidFreezeGlowRange;
	CCollision *pCollision = GameClient()->Collision();
	if(!pCollision)
		return;

	auto IsFreeze = [](int Tile) {
		return Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE;
	};
	auto IsBlocking = [&](int Tile) {
		return Tile == TILE_SOLID || Tile == TILE_NOHOOK || IsFreeze(Tile);
	};

	// Only the tiles that the tee would really touch light up - not the whole
	// freeze area around it. Up to three tiles (left foot, centre, right foot).
	vec2 aThreats[4];
	int NumThreats = 0;
	auto AddThreat = [&](float X, float Y) {
		const vec2 Center(std::floor(X / 32.0f) * 32.0f + 16.0f, std::floor(Y / 32.0f) * 32.0f + 16.0f);
		for(int i = 0; i < NumThreats; i++)
			if(distance(aThreats[i], Center) < 10.0f)
				return;
		if(NumThreats < (int)std::size(aThreats))
			aThreats[NumThreats++] = Center;
	};

	// Feet probes: the tee collision radius is ~14 units, so three samples
	// approximate the actual footprint instead of a single centre point.
	// Returns 2 when a freeze tile is touched, 1 when any other solid tile is
	// touched and 0 when the path is still clear.
	static const float s_aFeetOffsets[] = {-9.0f, 0.0f, 9.0f};
	auto SampleFeet = [&](vec2 Body) {
		int Result = 0;
		for(float OffX : s_aFeetOffsets)
		{
			const float Fx = Body.x + OffX;
			const float Fy = Body.y - 13.0f;
			const int Tile = pCollision->GetTile(round_to_int(Fx), round_to_int(Fy));
			if(IsFreeze(Tile))
			{
				AddThreat(Fx, Fy);
				Result = 2;
			}
			else if(IsBlocking(Tile))
			{
				Result = std::max(Result, 1);
			}
		}
		return Result;
	};

	// Standing on (or hovering just above) a freeze tile.
	float Immediacy = 0.0f;
	if(SampleFeet(Pos) == 2)
		Immediacy = 1.0f;
	if(NumThreats == 0 && SampleFeet(Pos + vec2(0.0f, -12.0f)) == 2)
		Immediacy = 0.9f;

	// Look ahead along the actual ballistic trajectory (velocity + gravity).
	// This is purely visual: it never changes input or movement.
	const vec2 Gravity(0.0f, 0.5f);
	if(NumThreats == 0)
	{
		for(int Step = 1; Step <= 48; Step++)
		{
			const float S = (float)Step;
			const vec2 P = Pos + Vel * S + Gravity * (0.5f * S * S);
			if(distance(P, Pos) > Range)
				break;
			const int Hit = SampleFeet(P);
			if(Hit == 0)
				continue;
			if(Hit == 2)
				Immediacy = std::max(Immediacy, Clamp01(1.0f - S / 30.0f));
			break;
		}
	}

	if(NumThreats == 0)
		return;

	const float T = Client()->GlobalTime();
	const float Pulse = 0.5f + 0.5f * std::sin(T * 8.0f);
	const float Strength = 0.55f + 0.45f * Immediacy;

	// Soft rounded-square glow that follows the block, so exactly the tiles the
	// tee would land on are burning red.
	Graphics()->BlendAdditive();
	Graphics()->TextureSet(m_BlockGlowTexture);
	Graphics()->QuadsBegin();
	Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
	for(int i = 0; i < NumThreats; i++)
	{
		const float Size = 62.0f + Pulse * 10.0f;
		Graphics()->SetColor(1.0f, 0.16f, 0.12f, (0.22f + Pulse * 0.24f) * Strength);
		IGraphics::CQuadItem Glow(aThreats[i].x - Size * 0.5f, aThreats[i].y - Size * 0.5f, Size, Size);
		Graphics()->QuadsDraw(&Glow, 1);

		const float CoreSize = 36.0f + Pulse * 4.0f;
		Graphics()->SetColor(1.0f, 0.48f, 0.22f, (0.34f + Pulse * 0.30f) * Strength);
		IGraphics::CQuadItem Core(aThreats[i].x - CoreSize * 0.5f, aThreats[i].y - CoreSize * 0.5f, CoreSize, CoreSize);
		Graphics()->QuadsDraw(&Core, 1);
	}
	Graphics()->QuadsEnd();
	Graphics()->BlendNormal();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

	if(Immediacy > 0.35f)
		RenderFreezeVignette(Immediacy);
}

void CInvalidClient::RenderFreezeVignette(float Proximity)
{
	if(!g_Config.m_ClInvalidFreezeVignette || Proximity <= 0.02f)
		return;
	const float Width = (float)Graphics()->ScreenWidth();
	const float Height = (float)Graphics()->ScreenHeight();
	if(Width <= 0.0f || Height <= 0.0f)
		return;
	const CScreenRect Screen = Graphics()->GetScreen();
	Graphics()->MapScreenToSize(Width, Height);

	const float T = Client()->GlobalTime();
	const float Pulse = 0.5f + 0.5f * std::sin(T * 7.0f);
	const float Alpha = (0.32f + Pulse * 0.30f) * Proximity;
	// One soft full-screen gradient: no hard rectangle edges in the corners.
	Graphics()->BlendAdditive();
	Graphics()->TextureSet(m_VignetteTexture);
	Graphics()->QuadsBegin();
	Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha);
	IGraphics::CQuadItem Vignette(0.0f, 0.0f, Width, Height);
	Graphics()->QuadsDraw(&Vignette, 1);
	Graphics()->QuadsEnd();
	Graphics()->BlendNormal();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	Graphics()->MapScreen(Screen);
}

void CInvalidClient::OnUpdate()
{
	if(m_PeeTime > 0.0f)
		m_PeeTime = std::max(0.0f, m_PeeTime - Client()->RenderFrameTime());
	if(m_PeeTime <= 0.0f && !g_Config.m_ClInvalidPee)
		m_PeeTime = 0.0f;
}

void CInvalidClient::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;
	RenderFreezeWarning();
	RenderMarkers();
	RenderSpectatorWarning();
	ManageParticles(std::clamp(Client()->RenderFrameTime(), 0.001f, 0.1f));

	const int Style = std::clamp(g_Config.m_ClInvalidTrailStyle, 0, NUM_TRAIL_STYLES - 1);
	Graphics()->BlendAdditive();
	for(const CParticle &P : m_aParticles)
	{
		if(P.m_Life <= 0.0f)
			continue;
		const float T = P.m_Life / P.m_MaxLife;
		IGraphics::CTextureHandle Texture;
		ColorRGBA Color(1.0f, 1.0f, 1.0f, 1.0f);
		if(P.m_Style == 1)
		{
			Texture = m_SmokeTexture;
			Color = ColorRGBA(0.62f, 0.62f, 0.66f, T * 0.30f);
		}
		else if(P.m_Style == 2)
		{
			Texture = m_SmokeTexture;
			Color = ColorRGBA(0.35f, 0.70f, 1.0f, T * 0.75f);
		}
		else if(P.m_Style == 3)
		{
			Texture = m_TearTexture;
			Color = ColorRGBA(0.55f, 0.85f, 1.0f, T * 0.95f);
		}
		else if(P.m_Style == 4)
		{
			Texture = m_SadTexture;
			Color = ColorRGBA(1.0f, 1.0f, 1.0f, T * 0.95f);
		}
		else if(P.m_Style == 5)
		{
			Texture = m_TearTexture;
			Color = ColorRGBA(0.98f, 0.86f, 0.24f, T * 0.80f);
		}
		else
		{
			Texture = m_SparkTexture;
			vec3 C(1.0f, 1.0f, 1.0f);
			switch(Style)
			{
			case 0: C = vec3(1.0f, 0.85f, 0.35f); break;
			case 1: C = vec3(0.45f, 0.90f, 1.0f); break;
			case 2: C = vec3(1.0f, 0.55f, 0.85f); break;
			case 3: C = vec3(1.0f, 0.35f, 0.10f); break;
			default: C = vec3(0.5f + 0.5f * std::sin(T * 12.0f), 0.5f + 0.5f * std::sin(T * 12.0f + 2.0f), 0.5f + 0.5f * std::sin(T * 12.0f + 4.0f)); break;
			}
			Color = ColorRGBA(C.r, C.g, C.b, T * (g_Config.m_ClInvalidTrail / 100.0f));
		}
		Graphics()->TextureSet(Texture);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
		Graphics()->SetColor(Color);
		const float Size = P.m_Size * (P.m_Style == 1 ? 1.0f : T);
		IGraphics::CQuadItem Item(P.m_Pos.x, P.m_Pos.y, Size, Size);
		Graphics()->QuadsDraw(&Item, 1);
		Graphics()->QuadsEnd();
	}
	Graphics()->BlendNormal();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

// ---------- chat helpers ----------
static void AppendLatin(char *&pOut, int &Remaining, const char *pLatin)
{
	while(*pLatin && Remaining > 1)
	{
		*pOut++ = *pLatin++;
		--Remaining;
	}
	*pOut = '\0';
}

void CInvalidClient::Transliterate(const char *pIn, char *pOut, int OutSize)
{
	static const char *s_apMap[2][64] = {
		{"a", "b", "v", "g", "d", "e", "yo", "zh", "z", "i", "j", "k", "l", "m", "n", "o", "p", "r", "s", "t", "u", "f", "h", "ts", "ch", "sh", "sch", "", "y", "", "e", "yu", "ya"},
		{"A", "B", "V", "G", "D", "E", "Yo", "Zh", "Z", "I", "J", "K", "L", "M", "N", "O", "P", "R", "S", "T", "U", "F", "H", "Ts", "Ch", "Sh", "Sch", "", "Y", "", "E", "Yu", "Ya"}};
	int Remaining = OutSize - 1;
	while(*pIn && Remaining > 1)
	{
		const unsigned char C = (unsigned char)*pIn;
		if(C < 0x80)
		{
			*pOut++ = (char)C;
			--Remaining;
			pIn++;
			continue;
		}
		int Code = -1;
		int Len = 0;
		if((C == 0xD0 || C == 0xD1) && (unsigned char)pIn[1] >= 0x80)
		{
			Code = ((C & 0x1F) << 6) | ((unsigned char)pIn[1] & 0x3F);
			Len = 2;
		}
		if(Code >= 0x410 && Code <= 0x44F)
		{
			const int Idx = (Code <= 0x42F ? Code - 0x410 : Code - 0x430);
			const int Upper = (Code <= 0x42F) ? 1 : 0;
			AppendLatin(pOut, Remaining, s_apMap[Upper][Idx]);
			pIn += Len;
		}
		else
		{
			*pOut++ = (char)C;
			--Remaining;
			pIn++;
		}
	}
	*pOut = '\0';
}

bool CInvalidClient::ProcessOutgoing(char *pLine, int Size)
{
	if(pLine[0] == '!')
	{
		char aCmd[32] = {};
		char aName[MAX_NAME_LENGTH] = {};
		const char *pSpace = std::strchr(pLine, ' ');
		const int CmdLen = pSpace ? (int)(pSpace - pLine) : str_length(pLine);
		str_copy(aCmd, pLine, std::min(CmdLen + 1, (int)sizeof(aCmd)));
		if(pSpace)
		{
			const char *pName = str_utf8_skip_whitespaces(pSpace + 1);
			str_copy(aName, pName, sizeof(aName));
		}
		if(str_comp_nocase(aCmd, "!team") == 0)
		{
			str_copy(g_Config.m_ClInvalidTeamName, aName, sizeof(g_Config.m_ClInvalidTeamName));
			GameClient()->Echo(aName[0] ? "!team marker set" : "!team marker cleared");
			return true;
		}
		if(str_comp_nocase(aCmd, "!target") == 0)
		{
			str_copy(g_Config.m_ClInvalidTargetName, aName, sizeof(g_Config.m_ClInvalidTargetName));
			GameClient()->Echo(aName[0] ? "!target marker set" : "!target marker cleared");
			return true;
		}
		if(str_comp_nocase(aCmd, "!clear") == 0)
		{
			g_Config.m_ClInvalidTeamName[0] = '\0';
			g_Config.m_ClInvalidTargetName[0] = '\0';
			GameClient()->Echo("markers cleared");
			return true;
		}
	}
	if(g_Config.m_ClInvalidTransliterate && pLine[0] != '/' && pLine[0] != '!')
	{
		char aBuf[512];
		Transliterate(pLine, aBuf, sizeof(aBuf));
		str_copy(pLine, aBuf, Size);
	}
	return false;
}

void CInvalidClient::ProcessIncoming(int ClientId, const char *pLine)
{
	if(!g_Config.m_ClInvalidAutoReply)
		return;
	const int LocalId = GameClient()->m_Snap.m_LocalClientId;
	if(ClientId < 0 || LocalId < 0 || ClientId == LocalId)
		return;
	const char *pMyName = GameClient()->m_aClients[LocalId].m_aName;
	if(!pMyName[0] || !str_find_nocase(pLine, pMyName))
		return;
	static int64_t s_LastReply = 0;
	const int64_t Now = time_get();
	if(Now - s_LastReply < (int64_t)time_freq() * 8)
		return;
	s_LastReply = Now;
	GameClient()->m_Chat.SendChat(0, g_Config.m_ClInvalidAutoReplyText);
}

void CInvalidClient::OnConsoleInit()
{
	Console()->Register("invalid_pee", "", CFGFLAG_CLIENT, ConPee, this, "Make your tee pee (visual only)");
	Console()->Register("invalid_rescue", "", CFGFLAG_CLIENT, ConRescue, this, "Send /rescue (dummy rescue helper)");
	Console()->Register("invalid_save", "", CFGFLAG_CLIENT, ConSave, this, "Send /save");
	Console()->Register("invalid_load", "", CFGFLAG_CLIENT, ConLoad, this, "Send /load");
	Console()->Register("invalid_team", "?s[name]", CFGFLAG_CLIENT, ConTeam, this, "Set the !team marker name");
	Console()->Register("invalid_target", "?s[name]", CFGFLAG_CLIENT, ConTarget, this, "Set the !target marker name");
	Console()->Register("invalid_say", "r[text]", CFGFLAG_CLIENT, ConSay, this, "Send transliterated text to chat");
}

void CInvalidClient::TriggerPee()
{
	if(!g_Config.m_ClInvalidPee)
	{
		GameClient()->Echo("Invalid pee effect is disabled in Settings -> Invalid");
		return;
	}
	m_PeeTime = 2.8f;
	m_LastPeeEmit = 0.0f;
	GameClient()->Echo("колобок мой ссыт мочой");
}

void CInvalidClient::SendHelper(const char *pLine)
{
	if(Client()->State() != IClient::STATE_ONLINE)
		return;
	GameClient()->m_Chat.SendChat(0, pLine);
}

void CInvalidClient::ConPee(IConsole::IResult *pResult, void *pUserData)
{
	((CInvalidClient *)pUserData)->TriggerPee();
}

void CInvalidClient::ConRescue(IConsole::IResult *pResult, void *pUserData)
{
	((CInvalidClient *)pUserData)->SendHelper("/rescue");
}

void CInvalidClient::ConSave(IConsole::IResult *pResult, void *pUserData)
{
	((CInvalidClient *)pUserData)->SendHelper("/save");
}

void CInvalidClient::ConLoad(IConsole::IResult *pResult, void *pUserData)
{
	((CInvalidClient *)pUserData)->SendHelper("/load");
}

void CInvalidClient::ConTeam(IConsole::IResult *pResult, void *pUserData)
{
	if(pResult->NumArguments() > 0)
		str_copy(g_Config.m_ClInvalidTeamName, pResult->GetString(0), sizeof(g_Config.m_ClInvalidTeamName));
	else
		g_Config.m_ClInvalidTeamName[0] = '\0';
}

void CInvalidClient::ConTarget(IConsole::IResult *pResult, void *pUserData)
{
	if(pResult->NumArguments() > 0)
		str_copy(g_Config.m_ClInvalidTargetName, pResult->GetString(0), sizeof(g_Config.m_ClInvalidTargetName));
	else
		g_Config.m_ClInvalidTargetName[0] = '\0';
}

void CInvalidClient::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	char aBuf[512];
	Transliterate(pResult->GetString(0), aBuf, sizeof(aBuf));
	((CInvalidClient *)pUserData)->SendHelper(aBuf);
}

// ---------- spectator warning ----------
void CInvalidClient::RenderMarkers()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;
	const CGameClient::CSnapState &Snap = GameClient()->m_Snap;
	if(!Snap.m_pGameInfoObj)
		return;

	const bool TeamOn = g_Config.m_ClInvalidTeamMarker && g_Config.m_ClInvalidTeamName[0];
	const bool TargetOn = g_Config.m_ClInvalidTargetMarker && g_Config.m_ClInvalidTargetName[0];
	if(!TeamOn && !TargetOn)
		return;

	const float Time = Client()->GlobalTime();
	const vec2 LocalPos = GameClient()->m_Controls.m_aTargetPos[g_Config.m_ClDummy];
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!Snap.m_aCharacters[i].m_Active || i == Snap.m_LocalClientId)
			continue;
		const char *pName = GameClient()->m_aClients[i].m_aName;
		const bool Team = TeamOn && str_comp_nocase(pName, g_Config.m_ClInvalidTeamName) == 0;
		const bool Target = TargetOn && str_comp_nocase(pName, g_Config.m_ClInvalidTargetName) == 0;
		if(!Team && !Target)
			continue;

		const vec2 TargetPos = GameClient()->m_aClients[i].m_RenderPos;
		const vec2 Delta = TargetPos - LocalPos;
		const float DistBlocks = length(Delta) / 32.0f;
		const float Angle = std::atan2(Delta.y, Delta.x);
		const vec2 Dir(std::cos(Angle), std::sin(Angle));
		const vec2 Arrow = LocalPos + Dir * 56.0f;
		const float Pulse = 0.75f + 0.25f * std::sin(Time * 6.0f + i);

		const vec3 Color = Team ? vec3(0.18f, 1.0f, 0.34f) : vec3(1.0f, 0.24f, 0.28f);

		// soft chevron that points at the marked player, with an additive halo
		Graphics()->BlendAdditive();
		Graphics()->TextureSet(m_ArrowTexture);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
		Graphics()->QuadsSetRotation(Angle);
		Graphics()->SetColor(Color.r, Color.g, Color.b, 0.30f * Pulse);
		const float ArrowGlow = 52.0f;
		IGraphics::CQuadItem GlowItem(Arrow.x - ArrowGlow * 0.5f, Arrow.y - ArrowGlow * 0.5f, ArrowGlow, ArrowGlow);
		Graphics()->QuadsDraw(&GlowItem, 1);
		Graphics()->SetColor(Color.r, Color.g, Color.b, 0.95f * Pulse);
		const float ArrowSize = 30.0f;
		IGraphics::CQuadItem ArrowItem(Arrow.x - ArrowSize * 0.5f, Arrow.y - ArrowSize * 0.5f, ArrowSize, ArrowSize);
		Graphics()->QuadsDraw(&ArrowItem, 1);
		Graphics()->QuadsSetRotation(0.0f);
		Graphics()->QuadsEnd();

		// pulsing reticle floating above the marked tee
		const float RingW = 54.0f + Pulse * 7.0f;
		const float RingH = 27.0f + Pulse * 4.0f;
		const float RingY = TargetPos.y - 64.0f;
		Graphics()->TextureSet(m_RingTexture);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetSubset(0.0f, 0.0f, 1.0f, 1.0f);
		Graphics()->SetColor(Color.r, Color.g, Color.b, 0.90f * Pulse);
		IGraphics::CQuadItem RingItem(TargetPos.x - RingW * 0.5f, RingY - RingH * 0.5f, RingW, RingH);
		Graphics()->QuadsDraw(&RingItem, 1);
		Graphics()->TextureSet(m_SparkTexture);
		Graphics()->SetColor(Color.r, Color.g, Color.b, 0.32f * Pulse);
		IGraphics::CQuadItem Core(TargetPos.x - 20.0f, RingY - 10.0f, 40.0f, 20.0f);
		Graphics()->QuadsDraw(&Core, 1);
		Graphics()->QuadsEnd();
		Graphics()->BlendNormal();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%.0f", DistBlocks);
		TextRender()->TextColor(0.0f, 0.0f, 0.0f, 0.55f);
		TextRender()->Text(TargetPos.x + 1.0f, TargetPos.y - 84.0f, 14.0f, aBuf, -1.0f);
		TextRender()->TextColor(Color.r, Color.g, Color.b, 1.0f);
		TextRender()->Text(TargetPos.x, TargetPos.y - 85.0f, 14.0f, aBuf, -1.0f);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

void CInvalidClient::RenderSpectatorWarning()
{
	if(!g_Config.m_ClInvalidSpectatorWarning || Client()->State() != IClient::STATE_ONLINE)
		return;
	const CGameClient::CSnapState &Snap = GameClient()->m_Snap;
	if(!Snap.m_pGameInfoObj || Snap.m_NumSpectatorWatchers <= 0)
		return;

	int aSpecs[MAX_CLIENTS];
	int Num = 0;
	for(int i = 0; i < MAX_CLIENTS && Num < MAX_CLIENTS; i++)
	{
		if(!Snap.m_aSpectatorWatchers[i] || i == Snap.m_LocalClientId)
			continue;
		if(!GameClient()->m_aClients[i].m_Active)
			continue;
		aSpecs[Num++] = i;
	}
	if(Num == 0)
		return;

	const float Width = 300.0f * Graphics()->ScreenAspect();
	const float Height = 300.0f;
	const CScreenRect Screen = Graphics()->GetScreen();
	Graphics()->MapScreenToSize(Width, Height);
	const float BoxW = 132.0f;
	const float BoxH = 12.0f + Num * 9.5f;
	Graphics()->DrawRect(4.0f, 34.0f, BoxW, BoxH, ColorRGBA(0.03f, 0.03f, 0.05f, 0.62f), IGraphics::CORNER_ALL, 4.0f);
	Graphics()->DrawRect(4.0f, 34.0f, 2.0f, BoxH, ColorRGBA(0.95f, 0.15f, 0.18f, 0.95f), IGraphics::CORNER_ALL, 1.0f);
	TextRender()->TextColor(1.0f, 0.35f, 0.35f, 1.0f);
	TextRender()->Text(8.0f, 35.5f, 7.0f, "Наблюдают:", -1.0f);
	TextRender()->TextColor(0.85f, 0.88f, 0.95f, 1.0f);
	for(int j = 0; j < Num; j++)
		TextRender()->Text(8.0f, 45.0f + j * 9.5f, 6.5f, GameClient()->m_aClients[aSpecs[j]].m_aName, -1.0f);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	Graphics()->MapScreen(Screen);
}
