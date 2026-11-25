/* Copyright © 2026 KOG Helper contributors (BestClient fork) */
#ifndef GAME_CLIENT_COMPONENTS_BESTCLIENT_KOG_HELPER_H
#define GAME_CLIENT_COMPONENTS_BESTCLIENT_KOG_HELPER_H

#include <game/client/component.h>

#include <array>

class CKogHelper : public CComponent
{
public:
	int Sizeof() const override { return sizeof(*this); }

	void OnReset() override;
	void OnRender() override;

	// Called from CControls right after the player direction is resolved.
	// Only corrects movement; aim and hook are never touched.
	void ApplyAvoid(int *pDirection, int *pJump, int *pFire);

private:
	static constexpr int MAX_PREVIEW_TICKS = 48;

	struct SPreviewPoint
	{
		vec2 m_Pos;
		bool m_Freeze = false;
	};

	void Predict(int Direction, int Jump, int Fire, int Horizon, SPreviewPoint *pOut, int *pOutLen, int *pOutFreezeTick) const;
	bool CanIntervene() const;
	void LogIntervention(int FromDirection, int ToDirection, int Jump, int Fire, int FreezeTick);

	int m_TickCounter = 0;
	int m_OverrideDirection = 0;
	int m_OverrideJump = 0;
	int m_OverrideFire = 0;
	int m_OverrideTicks = 0;
	int m_CooldownTicks = 0;
	int64_t m_LastLogTime = 0;

	std::array<SPreviewPoint, MAX_PREVIEW_TICKS> m_aPreview = {};
	int m_PreviewLen = 0;
	int m_PreviewFreezeTick = -1;
	int m_PreviewDirection = 0;
};

#endif
