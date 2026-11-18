/* Invalid: high quality procedural cosmetics and effects for BestClient. */
#ifndef GAME_CLIENT_COMPONENTS_BESTCLIENT_INVALID_CLIENT_H
#define GAME_CLIENT_COMPONENTS_BESTCLIENT_INVALID_CLIENT_H

#include <engine/console.h>
#include <engine/graphics.h>

#include <game/client/component.h>

#include <generated/protocol.h>

class CInvalidClient : public CComponent
{
public:
	CInvalidClient();
	int Sizeof() const override { return sizeof(*this); }

	void OnInit() override;
	void OnReset() override;
	void OnConsoleInit() override;
	void OnUpdate() override;
	void OnRender() override;

	// Wings are rendered behind the tee, accessories in front of it.
	void RenderWings(vec2 BodyPos, vec2 Direction, float Alpha, float BaseSize, int ClientId);
	void RenderAccessories(vec2 BodyPos, vec2 Direction, float Alpha, float BaseSize, int ClientId, int Emote);

	// Outgoing chat post-processing: consumes "!team"/"!target", optional transliteration.
	// Returns true when the message must not be sent to the server.
	bool ProcessOutgoing(char *pLine, int Size);
	void ProcessIncoming(int ClientId, const char *pLine);

	static void Transliterate(const char *pIn, char *pOut, int OutSize);

	// Starts the pee effect immediately (used by the settings page test button).
	void TriggerPee();

private:
	IGraphics::CTextureHandle m_aWingTextures[6];
	IGraphics::CTextureHandle m_aGlassesTextures[8];
	IGraphics::CTextureHandle m_aHatTextures[6];
	IGraphics::CTextureHandle m_SadTexture;
	IGraphics::CTextureHandle m_CigaretteTexture;
	IGraphics::CTextureHandle m_ArrowTexture;
	IGraphics::CTextureHandle m_RingTexture;
	IGraphics::CTextureHandle m_VignetteTexture;
	IGraphics::CTextureHandle m_BlockGlowTexture;
	IGraphics::CTextureHandle m_SparkTexture;
	IGraphics::CTextureHandle m_SmokeTexture;
	IGraphics::CTextureHandle m_TearTexture;
	IGraphics::CTextureHandle m_EmberTexture;
	bool m_bTexturesReady = false;

	void GenerateTextures();
	IGraphics::CTextureHandle MakeWingTexture(int Style);
	IGraphics::CTextureHandle MakeGlassesTexture(int Style);
	IGraphics::CTextureHandle MakeHatTexture(int Style);
	IGraphics::CTextureHandle MakeSoftCircle(const char *pName, float R, float G, float B, float Core);
	IGraphics::CTextureHandle MakeSadTexture();
	IGraphics::CTextureHandle MakeCigaretteTexture();
	IGraphics::CTextureHandle MakeArrowTexture();
	IGraphics::CTextureHandle MakeRingTexture();
	IGraphics::CTextureHandle MakeVignetteTexture();
	IGraphics::CTextureHandle MakeBlockGlowTexture();

	struct CParticle
	{
		vec2 m_Pos;
		vec2 m_Vel;
		float m_Life;
		float m_MaxLife;
		float m_Size;
		int m_Style;
	};
	CParticle m_aParticles[512];
	int m_NextParticle = 0;
	float m_LastEmit = 0.0f;
	float m_LastPeeEmit = 0.0f;
	float m_LastSmokeEmit = 0.0f;
	float m_PeeTime = 0.0f;
	int m_aLastEmote[MAX_CLIENTS];

	void Emit(int Style, vec2 Pos, vec2 Vel, float Life, float Size);
	void ManageParticles(float Delta);
	void RenderCigarette(vec2 BodyPos, vec2 Direction, float Alpha, float BaseSize);
	void RenderPee(vec2 BodyPos, vec2 Direction, float Alpha, float BaseSize);
	void RenderFreezeWarning();
	void RenderFreezeVignette(float Proximity);
	void RenderMarkers();
	void RenderSpectatorWarning();

	static void ConPee(IConsole::IResult *pResult, void *pUserData);
	static void ConRescue(IConsole::IResult *pResult, void *pUserData);
	static void ConSave(IConsole::IResult *pResult, void *pUserData);
	static void ConLoad(IConsole::IResult *pResult, void *pUserData);
	static void ConTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConTarget(IConsole::IResult *pResult, void *pUserData);
	static void ConSay(IConsole::IResult *pResult, void *pUserData);

	void SendHelper(const char *pLine);

	static const int NUM_TRAIL_STYLES = 5;
};

#endif
