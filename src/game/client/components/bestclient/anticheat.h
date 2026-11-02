#ifndef GAME_CLIENT_COMPONENTS_BESTCLIENT_ANTICHEAT_H
#define GAME_CLIENT_COMPONENTS_BESTCLIENT_ANTICHEAT_H

#include <game/client/component.h>

#include <base/color.h>
#include <base/vmath.h>

class CAntiCheat : public CComponent
{
public:
	enum
	{
		MAX_ANALYSES = MAX_CLIENTS,
	};

	struct SAnalysis
	{
		int m_ClientId;
		char m_aName[MAX_NAME_LENGTH];
		int m_Shots;
		int m_Hits;
		int m_SuspiciousAimFrames;
		int m_SuspiciousReactionFrames;
		float m_Accuracy;
		float m_Suspicion; // 0.0 .. 1.0
		bool m_Active;
	};

	CAntiCheat();

	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnInit() override;
	virtual void OnReset() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;

	const SAnalysis *GetAnalysis(int ClientId) const;
	int NumAnalyses() const { return m_NumAnalyses; }
	const SAnalysis *Analyses() const { return m_aAnalyses; }

private:
	SAnalysis m_aAnalyses[MAX_ANALYSES];
	int m_NumAnalyses;
	int64_t m_LastScan;
	int64_t m_ScanInterval;

	void AnalyzeClient(int ClientId, CGameClient *pGameClient);
	void UpdateScore(SAnalysis *pAnalysis);
	const char *LevelText(float Suspicion) const;
	ColorRGBA LevelColor(float Suspicion) const;
};

#endif
