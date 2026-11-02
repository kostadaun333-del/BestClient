#include "anticheat.h"

#include <base/math.h>
#include <base/system.h>
#include <base/time.h>

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <game/client/gameclient.h>

CAntiCheat::CAntiCheat()
{
	m_NumAnalyses = 0;
	m_LastScan = 0;
	m_ScanInterval = time_freq() / 10; // 100 ms
	mem_zero(m_aAnalyses);
}

void CAntiCheat::OnInit()
{
	m_NumAnalyses = 0;
	m_LastScan = 0;
	mem_zero(m_aAnalyses);
}

void CAntiCheat::OnReset()
{
	m_NumAnalyses = 0;
	m_LastScan = 0;
	mem_zero(m_aAnalyses);
}

const CAntiCheat::SAnalysis *CAntiCheat::GetAnalysis(int ClientId) const
{
	for(int i = 0; i < m_NumAnalyses; i++)
	{
		if(m_aAnalyses[i].m_ClientId == ClientId)
			return &m_aAnalyses[i];
	}
	return nullptr;
}

void CAntiCheat::OnUpdate()
{
	if(!g_Config.m_BcAntiCheat)
		return;

	CGameClient *pGameClient = GameClient();
	if(pGameClient == nullptr)
		return;

	const int State = Client()->State();
	if(State != IClient::STATE_ONLINE && State != IClient::STATE_DEMOPLAYBACK)
		return;

	const int64_t Now = time_get();
	if(Now - m_LastScan < m_ScanInterval)
		return;
	m_LastScan = Now;

	m_NumAnalyses = 0;

	const int LocalId = pGameClient->m_Snap.m_LocalClientId;
	const int MaxId = minimum(MAX_CLIENTS, MAX_ANALYSES);
	for(int i = 0; i < MaxId; i++)
	{
		if(i == LocalId)
			continue;
		if(!pGameClient->m_aClients[i].m_Active)
			continue;
		if(!pGameClient->m_Snap.m_aCharacters[i].m_Active)
			continue;

		AnalyzeClient(i, pGameClient);
	}
}

void CAntiCheat::AnalyzeClient(int ClientId, CGameClient *pGameClient)
{
	if(m_NumAnalyses >= MAX_ANALYSES)
		return;

	const CGameClient::CClientData *pClient = &pGameClient->m_aClients[ClientId];
	const CGameClient::CSnapState::CCharacterInfo *pChar = &pGameClient->m_Snap.m_aCharacters[ClientId];

	SAnalysis *pAnalysis = &m_aAnalyses[m_NumAnalyses];
	mem_zero(pAnalysis);
	pAnalysis->m_ClientId = ClientId;
	str_copy(pAnalysis->m_aName, pClient->m_aName, sizeof(pAnalysis->m_aName));
	pAnalysis->m_Active = true;

	// --- Эвристика 1: фиксация прицела на ближайшем враге ---
	const float OwnAngle = pClient->m_Angle;
	const vec2 OwnPos = vec2(pChar->m_Cur.m_X, pChar->m_Cur.m_Y);

	vec2 BestTarget(0.0f, 0.0f);
	float BestDist = 100000.0f;
	bool FoundTarget = false;

	for(int j = 0; j < MAX_CLIENTS; j++)
	{
		if(j == ClientId)
			continue;
		if(!pGameClient->m_aClients[j].m_Active)
			continue;
		if(!pGameClient->m_Snap.m_aCharacters[j].m_Active)
			continue;
		if(pGameClient->IsOtherTeam(j) == pGameClient->IsOtherTeam(ClientId))
			continue; // интересуют только враги

		const CGameClient::CSnapState::CCharacterInfo *pOther = &pGameClient->m_Snap.m_aCharacters[j];
		const vec2 OtherPos = vec2(pOther->m_Cur.m_X, pOther->m_Cur.m_Y);
		const float Dist = distance(OwnPos, OtherPos);
		if(Dist < BestDist)
		{
			BestDist = Dist;
			BestTarget = OtherPos;
			FoundTarget = true;
		}
	}

	if(FoundTarget && BestDist < 800.0f)
	{
		const vec2 ToTarget = normalize(BestTarget - OwnPos);
		const float TargetAngle = atan2f(ToTarget.y, ToTarget.x);
		const float Diff = absolute(remainderf(OwnAngle - TargetAngle, 2.0f * pi));
		if(Diff < 0.15f) // ~8.6 градуса
			pAnalysis->m_SuspiciousAimFrames++;
	}

	// --- Эвристика 2: статистика выстрелов ---
	pAnalysis->m_Shots = 0;
	pAnalysis->m_Hits = 0;
	pAnalysis->m_Accuracy = 0.0f;

	// --- Эвристика 3: время реакции ---
	if(FoundTarget && BestDist < 400.0f)
	{
		const vec2 ToTarget = normalize(BestTarget - OwnPos);
		const float TargetAngle = atan2f(ToTarget.y, ToTarget.x);
		const float Diff = absolute(remainderf(OwnAngle - TargetAngle, 2.0f * pi));
		if(Diff < 0.08f)
			pAnalysis->m_SuspiciousReactionFrames++;
	}

	UpdateScore(pAnalysis);
	m_NumAnalyses++;
}

void CAntiCheat::UpdateScore(SAnalysis *pAnalysis)
{
	float Score = 0.0f;

	if(pAnalysis->m_SuspiciousAimFrames > 100)
		Score += 0.45f;
	else if(pAnalysis->m_SuspiciousAimFrames > 40)
		Score += 0.25f;
	else if(pAnalysis->m_SuspiciousAimFrames > 15)
		Score += 0.1f;

	if(pAnalysis->m_SuspiciousReactionFrames > 60)
		Score += 0.35f;
	else if(pAnalysis->m_SuspiciousReactionFrames > 25)
		Score += 0.2f;
	else if(pAnalysis->m_SuspiciousReactionFrames > 10)
		Score += 0.1f;

	if(pAnalysis->m_Shots > 20)
	{
		if(pAnalysis->m_Accuracy > 0.9f)
			Score += 0.3f;
		else if(pAnalysis->m_Accuracy > 0.75f)
			Score += 0.15f;
	}

	pAnalysis->m_Suspicion = clamp(Score, 0.0f, 1.0f);
}

const char *CAntiCheat::LevelText(float Suspicion) const
{
	if(Suspicion < 0.25f)
		return "Clean";
	if(Suspicion < 0.60f)
		return "Suspicious";
	return "Soft";
}

ColorRGBA CAntiCheat::LevelColor(float Suspicion) const
{
	if(Suspicion < 0.25f)
		return ColorRGBA(0.2f, 1.0f, 0.2f, 1.0f);
	if(Suspicion < 0.60f)
		return ColorRGBA(1.0f, 0.8f, 0.2f, 1.0f);
	return ColorRGBA(1.0f, 0.2f, 0.2f, 1.0f);
}

void CAntiCheat::OnRender()
{
	if(!g_Config.m_BcAntiCheat || !g_Config.m_BcAntiCheatShowHud)
		return;

	if(m_NumAnalyses == 0)
		return;

	const int State = Client()->State();
	if(State != IClient::STATE_ONLINE && State != IClient::STATE_DEMOPLAYBACK)
		return;

	float X = 12.0f;
	float Y = 120.0f;
	const float LineH = 22.0f;

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->Text(X, Y, 14.0f, "BestClient AntiCheat", -1.0f);
	Y += LineH;

	for(int i = 0; i < m_NumAnalyses; i++)
	{
		const SAnalysis *p = &m_aAnalyses[i];
		const ColorRGBA Col = LevelColor(p->m_Suspicion);

		char aLine[256];
		str_format(aLine, sizeof(aLine), "%s  %.0f%%  [%s]",
			p->m_aName,
			p->m_Suspicion * 100.0f,
			LevelText(p->m_Suspicion));

		TextRender()->TextColor(Col);
		TextRender()->Text(X, Y, 12.0f, aLine, -1.0f);
		Y += LineH;

		const float BarW = 180.0f;
		const float BarH = 8.0f;
		const float BarX = X + 8.0f;

		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.15f, 0.15f, 0.15f, 0.85f);
		Graphics()->DrawRect(BarX, Y, BarW, BarH, IGraphics::CORNER_ALL, 2.0f);
		Graphics()->SetColor(Col.r, Col.g, Col.b, 1.0f);
		Graphics()->DrawRect(BarX, Y, BarW * p->m_Suspicion, BarH, IGraphics::CORNER_ALL, 2.0f);
		Graphics()->QuadsEnd();

		Y += BarH + 12.0f;
	}

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
}
