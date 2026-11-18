/* Copyright © 2026 BestProject Team */
#include <engine/font_icons.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <base/color.h>
#include <base/str.h>

#include <game/client/components/bestclient/ui_theme/style.h>
#include <game/client/components/bestclient/settings_search.h>
#include <game/client/components/menus.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

#include <algorithm>
#include <iterator>

enum
{
	INVALID_TAB_VISUALS = 0,
	INVALID_TAB_KOG,
	INVALID_TAB_CHAT,
	INVALID_TAB_WEATHER,
	INVALID_TAB_MISC,
	NUM_INVALID_TABS,
};

static int s_CurInvalidTab = INVALID_TAB_VISUALS;

namespace
{
constexpr float INVALID_LINE_SIZE = 20.0f;
constexpr float INVALID_MARGIN = 5.0f;
constexpr float INVALID_ROW = INVALID_LINE_SIZE + INVALID_MARGIN;
constexpr float INVALID_HEADLINE_SIZE = 20.0f;
constexpr float INVALID_BLOCK_PADDING = 20.0f;

void DrawInvalidBlockBg(CUIRect Block)
{
	CUIRect Bg = Block;
	Bg.w += INVALID_BLOCK_PADDING;
	Bg.h += INVALID_BLOCK_PADDING;
	Bg.x -= INVALID_BLOCK_PADDING * 0.5f;
	Bg.y -= INVALID_BLOCK_PADDING * 0.5f;
	BestClientUiTheme::DrawUiBlock(&Bg, IGraphics::CORNER_ALL, 10.0f);
}

CUIRect BeginInvalidBlock(CUIRect &Column, int Rows, const char *pTitle, CUi *pUi, const void *pHighlightId = nullptr)
{
	CUIRect Block;
	Column.HSplitTop(INVALID_LINE_SIZE + Rows * INVALID_ROW, &Block, &Column);
	DrawInvalidBlockBg(Block);
	if(pHighlightId)
	{
		CUIRect Bg = Block;
		Bg.w += INVALID_BLOCK_PADDING;
		Bg.h += INVALID_BLOCK_PADDING;
		Bg.x -= INVALID_BLOCK_PADDING * 0.5f;
		Bg.y -= INVALID_BLOCK_PADDING * 0.5f;
		BestClientSettingsSearch::DrawBlockHighlight(pHighlightId, &Bg);
	}

	CUIRect Label;
	Block.HSplitTop(INVALID_LINE_SIZE, &Label, &Block);
	pUi->DoLabel(&Label, pTitle, INVALID_HEADLINE_SIZE, TEXTALIGN_ML);
	return Block;
}

void RowCheck(CUIRect &View, int *pValue, const char *pText, CMenus *pMenus)
{
	View.HSplitTop(INVALID_MARGIN, nullptr, &View);
	CUIRect Line;
	View.HSplitTop(INVALID_LINE_SIZE, &Line, &View);
	pMenus->DoButton_CheckBoxAutoVMarginAndSet(pValue, pText, pValue, &Line, INVALID_LINE_SIZE);
}

void RowSlider(CUIRect &View, int *pValue, const char *pText, int Min, int Max, const char *pSuffix, CUi *pUi)
{
	View.HSplitTop(INVALID_MARGIN, nullptr, &View);
	CUIRect Line;
	View.HSplitTop(INVALID_LINE_SIZE, &Line, &View);
	pUi->DoScrollbarOption(pValue, pValue, &Line, pText, Min, Max, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_DELAYUPDATE, pSuffix);
}

void RowDropDown(CUIRect &View, const char *pLabel, int &Value, const char **pItems, int Num, CUi::SDropDownState &State, CScrollRegion &Scroll, CUi *pUi)
{
	View.HSplitTop(INVALID_MARGIN, nullptr, &View);
	CUIRect Line, Label, Select;
	View.HSplitTop(INVALID_LINE_SIZE, &Line, &View);
	Line.VSplitLeft(110.0f, &Label, &Select);
	pUi->DoLabel(&Label, pLabel, 14.0f, TEXTALIGN_ML);
	State.m_SelectionPopupContext.m_pScrollRegion = &Scroll;
	Value = pUi->DoDropDown(&Select, Value, pItems, Num, State);
}

void RowBind(CUIRect &View, CButtonContainer &Reader, CButtonContainer &Clear, const char *pLabel, const char *pCommand, CMenus *pMenus)
{
	pMenus->DoLine_KeyReader(View, Reader, Clear, pLabel, pCommand);
}

bool RowButton(CUIRect &View, CButtonContainer *pButton, const char *pText, CMenus *pMenus)
{
	View.HSplitTop(INVALID_MARGIN, nullptr, &View);
	CUIRect Line;
	View.HSplitTop(INVALID_LINE_SIZE, &Line, &View);
	return pMenus->DoButton_Menu(pButton, pText, 0, &Line) != 0;
}

bool RowEdit(CUIRect &View, CLineInput *pInput, const char *pLabel, const char *pDefault, char *pBuf, size_t BufSize, CMenus *pMenus)
{
	View.HSplitTop(INVALID_MARGIN, nullptr, &View);
	CUIRect Line;
	View.HSplitTop(INVALID_LINE_SIZE + 2.0f, &Line, &View);
	return pMenus->DoEditBoxWithLabel(pInput, &Line, pLabel, pDefault, pBuf, BufSize);
}

void RowNote(CUIRect &View, const char *pText, CUi *pUi)
{
	View.HSplitTop(INVALID_MARGIN, nullptr, &View);
	CUIRect Line;
	View.HSplitTop(16.0f, &Line, &View);
	pUi->DoLabel(&Line, pText, 11.5f, TEXTALIGN_ML);
}

void ResetInvalidVisuals()
{
	g_Config.m_ClInvalidWings = 0;
	g_Config.m_ClInvalidWingStyle = 0;
	g_Config.m_ClInvalidWingFlap = 50;
	g_Config.m_ClInvalidWingGlow = 60;
	g_Config.m_ClInvalidGlasses = 0;
	g_Config.m_ClInvalidGlassesStyle = 0;
	g_Config.m_ClInvalidHats = 0;
	g_Config.m_ClInvalidHatStyle = 0;
	g_Config.m_ClInvalidTrail = 0;
	g_Config.m_ClInvalidTrailStyle = 0;
	g_Config.m_ClInvalidTears = 1;
	g_Config.m_ClInvalidCigarette = 0;
	g_Config.m_ClInvalidSpray = 0;
}

void ResetInvalidEffects()
{
	g_Config.m_ClInvalidPee = 1;
	g_Config.m_ClInvalidPeeAmount = 100;
	g_Config.m_ClInvalidFreezeGlow = 1;
	g_Config.m_ClInvalidFreezeGlowRange = 220;
	g_Config.m_ClInvalidFreezeVignette = 1;
	g_Config.m_ClInvalidSpectatorWarning = 0;
}

void ResetInvalidChat()
{
	g_Config.m_ClInvalidTransliterate = 0;
	g_Config.m_ClInvalidAutoReply = 0;
	str_copy(g_Config.m_ClInvalidAutoReplyText, "I'm here", sizeof(g_Config.m_ClInvalidAutoReplyText));
	g_Config.m_ClInvalidTeamName[0] = '\0';
	g_Config.m_ClInvalidTargetName[0] = '\0';
	g_Config.m_ClInvalidTeamMarker = 1;
	g_Config.m_ClInvalidTargetMarker = 1;
}
} // namespace

void CMenus::SetInvalidSettingsTab(int Tab)
{
	if(Tab < INVALID_TAB_VISUALS || Tab >= NUM_INVALID_TABS)
		Tab = INVALID_TAB_VISUALS;
	s_CurInvalidTab = Tab;
}

int CMenus::GetInvalidSettingsTab() const
{
	return s_CurInvalidTab;
}

void CMenus::RenderSettingsInvalid(CUIRect MainView)
{
	const float TopExtend = BestClientUiTheme::IsNewTabOption() ? 8.0f : 20.0f;
	MainView.y -= TopExtend;
	MainView.h += TopExtend;

	static CButtonContainer s_aPageTabs[NUM_INVALID_TABS] = {};

	MainView.HSplitTop(BestClientUiTheme::IsNewTabOption() ? 6.0f : 8.0f, nullptr, &MainView);

	const char *apTabNames[NUM_INVALID_TABS] = {
		BcLocalize("Visuals"),
		BcLocalize("Kog"),
		BcLocalize("Chat"),
		BcLocalize("Weather"),
		BcLocalize("Misc"),
	};

	if(!BestClientUiTheme::IsNewTabOption())
	{
		CUIRect TabBar, TabButton;
		MainView.HSplitTop(24.0f, &TabBar, &MainView);
		const float TabWidth = TabBar.w / (float)NUM_INVALID_TABS;
		for(int i = 0; i < NUM_INVALID_TABS; ++i)
		{
			TabBar.VSplitLeft(TabWidth, &TabButton, &TabBar);
			const int Corners = i == 0 ? IGraphics::CORNER_L : (i == NUM_INVALID_TABS - 1 ? IGraphics::CORNER_R : IGraphics::CORNER_NONE);
			if(DoButton_MenuTab(&s_aPageTabs[i], apTabNames[i], s_CurInvalidTab == i, &TabButton, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
				s_CurInvalidTab = i;
		}
		MainView.HSplitTop(10.0f, nullptr, &MainView);
	}

	if(s_CurInvalidTab < INVALID_TAB_VISUALS || s_CurInvalidTab >= NUM_INVALID_TABS)
		s_CurInvalidTab = INVALID_TAB_VISUALS;

	switch(s_CurInvalidTab)
	{
	case INVALID_TAB_VISUALS: RenderSettingsInvalidVisuals(MainView); break;
	case INVALID_TAB_KOG: RenderSettingsInvalidKog(MainView); break;
	case INVALID_TAB_CHAT: RenderSettingsInvalidChat(MainView); break;
	case INVALID_TAB_WEATHER: RenderSettingsInvalidWeather(MainView); break;
	case INVALID_TAB_MISC: RenderSettingsInvalidMisc(MainView); break;
	}
}

void CMenus::RenderSettingsInvalidVisuals(CUIRect MainView)
{
	static CScrollRegion s_ScrollRegion;
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 60.0f;
	ScrollParams.m_ScrollbarMargin = 5.0f;
	s_ScrollRegion.Begin(&MainView, &ScrollParams);
	MainView.VSplitRight(5.0f, &MainView, nullptr);
	MainView.VSplitLeft(5.0f, nullptr, &MainView);

	CUIRect LeftView, RightView;
	MainView.VSplitMid(&LeftView, &RightView, 30.0f);
	LeftView.VSplitLeft(5.0f, nullptr, &LeftView);
	RightView.VSplitRight(5.0f, &RightView, nullptr);

	CUIRect LeftColumn = LeftView;
	CUIRect RightColumn = RightView;
	LeftColumn.HSplitTop(10.0f, nullptr, &LeftColumn);
	RightColumn.HSplitTop(10.0f, nullptr, &RightColumn);

	{
		int Rows = 4;
		if(g_Config.m_ClInvalidWings)
			Rows += 3;
		if(g_Config.m_ClInvalidGlasses)
			Rows += 1;
		if(g_Config.m_ClInvalidHats)
			Rows += 1;
		CUIRect Block = BeginInvalidBlock(LeftColumn, Rows, BcLocalize("Tee cosmetics"), Ui(), &g_Config.m_ClInvalidWings);

		static CUi::SDropDownState s_WingStyleState;
		static CScrollRegion s_WingStyleScroll;
		static const char *apWingStyles[] = {
			BcLocalize("Angel"),
			BcLocalize("Demon"),
			BcLocalize("Ice"),
			BcLocalize("Fire"),
			BcLocalize("Galaxy"),
			BcLocalize("Forest"),
		};
		static CUi::SDropDownState s_GlassesStyleState;
		static CScrollRegion s_GlassesStyleScroll;
		static const char *apGlassesStyles[] = {
			BcLocalize("Luxury"),
			BcLocalize("Shades"),
			BcLocalize("Visor"),
			BcLocalize("Heart"),
			BcLocalize("Ninja"),
			BcLocalize("Sport"),
			BcLocalize("Round"),
			BcLocalize("Crystal"),
		};
		static CUi::SDropDownState s_HatStyleState;
		static CScrollRegion s_HatStyleScroll;
		static const char *apHatStyles[] = {
			BcLocalize("Crown"),
			BcLocalize("Wizard hat"),
			BcLocalize("Cap"),
			BcLocalize("Halo"),
			BcLocalize("Cat ears"),
			BcLocalize("Top hat"),
		};

		RowCheck(Block, &g_Config.m_ClInvalidWings, BcLocalize("Wings"), this);
		if(g_Config.m_ClInvalidWings)
		{
			RowDropDown(Block, BcLocalize("Style"), g_Config.m_ClInvalidWingStyle, apWingStyles, (int)std::size(apWingStyles), s_WingStyleState, s_WingStyleScroll, Ui());
			RowSlider(Block, &g_Config.m_ClInvalidWingFlap, BcLocalize("Flap speed"), 0, 100, "%", Ui());
			RowSlider(Block, &g_Config.m_ClInvalidWingGlow, BcLocalize("Glow"), 0, 100, "%", Ui());
		}
		RowCheck(Block, &g_Config.m_ClInvalidGlasses, BcLocalize("Glasses"), this);
		if(g_Config.m_ClInvalidGlasses)
			RowDropDown(Block, BcLocalize("Style"), g_Config.m_ClInvalidGlassesStyle, apGlassesStyles, (int)std::size(apGlassesStyles), s_GlassesStyleState, s_GlassesStyleScroll, Ui());
		RowCheck(Block, &g_Config.m_ClInvalidHats, BcLocalize("Hats"), this);
		if(g_Config.m_ClInvalidHats)
			RowDropDown(Block, BcLocalize("Style"), g_Config.m_ClInvalidHatStyle, apHatStyles, (int)std::size(apHatStyles), s_HatStyleState, s_HatStyleScroll, Ui());
		RowCheck(Block, &g_Config.m_ClInvalidTears, BcLocalize("Tears when hit"), this);
	}

	{
		CUIRect Block = BeginInvalidBlock(LeftColumn, 5, BcLocalize("Particles"), Ui(), &g_Config.m_ClInvalidTrail);
		static CUi::SDropDownState s_TrailStyleState;
		static CScrollRegion s_TrailStyleScroll;
		static const char *apTrailStyles[] = {
			BcLocalize("Sparks"),
			BcLocalize("Bubbles"),
			BcLocalize("Stars"),
			BcLocalize("Flames"),
			BcLocalize("Rainbow"),
		};

		RowSlider(Block, &g_Config.m_ClInvalidTrail, BcLocalize("Trail intensity"), 0, 100, "%", Ui());
		RowDropDown(Block, BcLocalize("Style"), g_Config.m_ClInvalidTrailStyle, apTrailStyles, (int)std::size(apTrailStyles), s_TrailStyleState, s_TrailStyleScroll, Ui());
		RowCheck(Block, &g_Config.m_ClInvalidCigarette, BcLocalize("Cigarette in hand"), this);
		RowCheck(Block, &g_Config.m_ClInvalidSpray, BcLocalize("Water spray"), this);
		RowNote(Block, BcLocalize("Particles are drawn locally and never sent to the server."), Ui());
	}

	{
		CUIRect Block = BeginInvalidBlock(RightColumn, 6, BcLocalize("Pee"), Ui(), &g_Config.m_ClInvalidPee);
		static CButtonContainer s_PeeBindReader;
		static CButtonContainer s_PeeBindClear;
		static CButtonContainer s_PeeTestButton;

		RowCheck(Block, &g_Config.m_ClInvalidPee, BcLocalize("Enable pee effect"), this);
		RowSlider(Block, &g_Config.m_ClInvalidPeeAmount, BcLocalize("Stream intensity"), 25, 300, "%", Ui());
		RowBind(Block, s_PeeBindReader, s_PeeBindClear, BcLocalize("Pee bind"), "invalid_pee", this);
		if(RowButton(Block, &s_PeeTestButton, BcLocalize("Test the effect"), this))
			GameClient()->m_InvalidClient.TriggerPee();
		RowNote(Block, BcLocalize("Visual only. Trigger with the bind or invalid_pee."), Ui());
		RowNote(Block, "\"колобок мой ссыт мочой\"", Ui());
	}

	{
		CUIRect Block = BeginInvalidBlock(RightColumn, 6, BcLocalize("Freeze warning"), Ui(), &g_Config.m_ClInvalidFreezeGlow);
		RowCheck(Block, &g_Config.m_ClInvalidFreezeGlow, BcLocalize("Red glow near ice"), this);
		RowSlider(Block, &g_Config.m_ClInvalidFreezeGlowRange, BcLocalize("Range"), 80, 500, "px", Ui());
		RowCheck(Block, &g_Config.m_ClInvalidFreezeVignette, BcLocalize("Screen edge vignette"), this);
		RowNote(Block, BcLocalize("Nearby ice glows red and pulses."), Ui());
		RowNote(Block, BcLocalize("A marker shows where your current velocity would freeze you."), Ui());
		RowNote(Block, BcLocalize("Visual only: movement and input are never changed."), Ui());
	}

	s_ScrollRegion.End();
}

void CMenus::RenderSettingsInvalidKog(CUIRect MainView)
{
	static CScrollRegion s_ScrollRegion;
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 60.0f;
	ScrollParams.m_ScrollbarMargin = 5.0f;
	s_ScrollRegion.Begin(&MainView, &ScrollParams);
	MainView.VSplitRight(5.0f, &MainView, nullptr);
	MainView.VSplitLeft(5.0f, nullptr, &MainView);

	CUIRect LeftView, RightView;
	MainView.VSplitMid(&LeftView, &RightView, 30.0f);
	LeftView.VSplitLeft(5.0f, nullptr, &LeftView);
	RightView.VSplitRight(5.0f, &RightView, nullptr);

	CUIRect LeftColumn = LeftView;
	CUIRect RightColumn = RightView;
	LeftColumn.HSplitTop(10.0f, nullptr, &LeftColumn);
	RightColumn.HSplitTop(10.0f, nullptr, &RightColumn);

	{
		CUIRect Block = BeginInvalidBlock(LeftColumn, 7, BcLocalize("Team & target markers"), Ui(), &g_Config.m_ClInvalidTeamMarker);
		static CLineInput s_TeamNameInput(g_Config.m_ClInvalidTeamName, sizeof(g_Config.m_ClInvalidTeamName));
		static CLineInput s_TargetNameInput(g_Config.m_ClInvalidTargetName, sizeof(g_Config.m_ClInvalidTargetName));

		RowCheck(Block, &g_Config.m_ClInvalidTeamMarker, BcLocalize("Highlight !team"), this);
		RowEdit(Block, &s_TeamNameInput, BcLocalize("Teammate"), BcLocalize("nickname"), g_Config.m_ClInvalidTeamName, sizeof(g_Config.m_ClInvalidTeamName), this);
		RowCheck(Block, &g_Config.m_ClInvalidTargetMarker, BcLocalize("Highlight !target"), this);
		RowEdit(Block, &s_TargetNameInput, BcLocalize("Target"), BcLocalize("nickname"), g_Config.m_ClInvalidTargetName, sizeof(g_Config.m_ClInvalidTargetName), this);
		RowNote(Block, BcLocalize("Use !team <nick> and !target <nick> in chat."), Ui());
		RowNote(Block, BcLocalize("Both commands are consumed locally and never reach the server."), Ui());
		RowNote(Block, BcLocalize("Markers show nickname, distance and direction through walls."), Ui());
	}

	{
		CUIRect Block = BeginInvalidBlock(LeftColumn, 5, BcLocalize("Quick kog binds"), Ui());
		static CButtonContainer s_SaveReader, s_SaveClear;
		static CButtonContainer s_LoadReader, s_LoadClear;
		static CButtonContainer s_RescueReader, s_RescueClear;
		static CButtonContainer s_PeeReader, s_PeeClear;

		RowBind(Block, s_SaveReader, s_SaveClear, BcLocalize("Save position"), "invalid_save", this);
		RowBind(Block, s_LoadReader, s_LoadClear, BcLocalize("Load position"), "invalid_load", this);
		RowBind(Block, s_RescueReader, s_RescueClear, BcLocalize("Rescue"), "invalid_rescue", this);
		RowBind(Block, s_PeeReader, s_PeeClear, BcLocalize("Pee"), "invalid_pee", this);
		RowNote(Block, BcLocalize("Binds send the normal kog commands /save, /load and /rescue."), Ui());
	}

	{
		CUIRect Block = BeginInvalidBlock(RightColumn, 7, BcLocalize("Invalid commands"), Ui());
		RowNote(Block, BcLocalize("invalid_pee - play the pee effect"), Ui());
		RowNote(Block, BcLocalize("invalid_say <text> - send transliterated chat text"), Ui());
		RowNote(Block, BcLocalize("invalid_team <nick> / invalid_target <nick>"), Ui());
		RowNote(Block, BcLocalize("invalid_save / invalid_load / invalid_rescue"), Ui());
		RowNote(Block, BcLocalize("!team <nick>, !target <nick>, !clear in chat"), Ui());
		RowNote(Block, BcLocalize("Binds are stored by the normal DDNet bind system."), Ui());
		RowNote(Block, BcLocalize("Clear a bind with the X button on the right."), Ui());
	}

	s_ScrollRegion.End();
}

void CMenus::RenderSettingsInvalidChat(CUIRect MainView)
{
	static CScrollRegion s_ScrollRegion;
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 60.0f;
	ScrollParams.m_ScrollbarMargin = 5.0f;
	s_ScrollRegion.Begin(&MainView, &ScrollParams);
	MainView.VSplitRight(5.0f, &MainView, nullptr);
	MainView.VSplitLeft(5.0f, nullptr, &MainView);

	CUIRect LeftView, RightView;
	MainView.VSplitMid(&LeftView, &RightView, 30.0f);
	LeftView.VSplitLeft(5.0f, nullptr, &LeftView);
	RightView.VSplitRight(5.0f, &RightView, nullptr);

	CUIRect LeftColumn = LeftView;
	CUIRect RightColumn = RightView;
	LeftColumn.HSplitTop(10.0f, nullptr, &LeftColumn);
	RightColumn.HSplitTop(10.0f, nullptr, &RightColumn);

	{
		CUIRect Block = BeginInvalidBlock(LeftColumn, 3, BcLocalize("Transliteration"), Ui(), &g_Config.m_ClInvalidTransliterate);
		RowCheck(Block, &g_Config.m_ClInvalidTransliterate, BcLocalize("Convert Cyrillic to Latin"), this);
		RowNote(Block, BcLocalize("Text is converted right before it is sent to the server."), Ui());
		RowNote(Block, BcLocalize("Commands starting with / or ! are never modified."), Ui());
	}

	{
		CUIRect Block = BeginInvalidBlock(LeftColumn, 5, BcLocalize("Auto reply"), Ui(), &g_Config.m_ClInvalidAutoReply);
		static CLineInput s_AutoReplyInput(g_Config.m_ClInvalidAutoReplyText, sizeof(g_Config.m_ClInvalidAutoReplyText));

		RowCheck(Block, &g_Config.m_ClInvalidAutoReply, BcLocalize("Answer when mentioned"), this);
		RowEdit(Block, &s_AutoReplyInput, BcLocalize("Reply"), BcLocalize("I'm here"), g_Config.m_ClInvalidAutoReplyText, sizeof(g_Config.m_ClInvalidAutoReplyText), this);
		RowNote(Block, BcLocalize("Replies at most once every 8 seconds."), Ui());
		RowNote(Block, BcLocalize("Skips your own messages and bots."), Ui());
	}

	{
		CUIRect Block = BeginInvalidBlock(RightColumn, 6, BcLocalize("Chat commands"), Ui());
		RowNote(Block, BcLocalize("!team <nick> marks a teammate in green."), Ui());
		RowNote(Block, BcLocalize("!target <nick> marks a target in red."), Ui());
		RowNote(Block, BcLocalize("!clear removes both markers."), Ui());
		RowNote(Block, BcLocalize("Markers also show distance and direction."), Ui());
		RowNote(Block, BcLocalize("Commands are intercepted client-side before sending."), Ui());
		RowNote(Block, BcLocalize("Nothing extra is transmitted to the server."), Ui());
	}

	{
		CUIRect Block = BeginInvalidBlock(RightColumn, 4, BcLocalize("Good to know"), Ui());
		RowNote(Block, BcLocalize("Auto reply is rate limited by design."), Ui());
		RowNote(Block, BcLocalize("Disable it on servers where chat bots are not welcome."), Ui());
		RowNote(Block, BcLocalize("Transliteration is off by default."), Ui());
		RowNote(Block, BcLocalize("Markers work in teams, race and kog modes."), Ui());
	}

	s_ScrollRegion.End();
}

void CMenus::RenderSettingsInvalidWeather(CUIRect MainView)
{
	static CScrollRegion s_ScrollRegion;
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 60.0f;
	ScrollParams.m_ScrollbarMargin = 5.0f;
	s_ScrollRegion.Begin(&MainView, &ScrollParams);
	MainView.VSplitRight(5.0f, &MainView, nullptr);
	MainView.VSplitLeft(5.0f, nullptr, &MainView);

	CUIRect LeftView, RightView;
	MainView.VSplitMid(&LeftView, &RightView, 30.0f);
	LeftView.VSplitLeft(5.0f, nullptr, &LeftView);
	RightView.VSplitRight(5.0f, &RightView, nullptr);

	CUIRect LeftColumn = LeftView;
	CUIRect RightColumn = RightView;
	LeftColumn.HSplitTop(10.0f, nullptr, &LeftColumn);
	RightColumn.HSplitTop(10.0f, nullptr, &RightColumn);

	const auto RowColor = [&](CUIRect &View, CButtonContainer *pResetId, const char *pText, unsigned *pColor, unsigned DefaultColor) {
		View.HSplitTop(INVALID_MARGIN, nullptr, &View);
		DoLine_ColorPicker(pResetId, INVALID_LINE_SIZE, 13.0f, 0.0f, &View, pText, pColor, color_cast<ColorRGBA>(ColorHSLA(DefaultColor)), false);
	};

	{
		int Rows = 4;
		if(g_Config.m_BcAtmosphere)
		{
			if(g_Config.m_BcWeather == 0)
				Rows += 4;
			else if(g_Config.m_BcWeather == 1)
				Rows += 4;
			else if(g_Config.m_BcWeather == 2)
				Rows += 3;
			else
				Rows += 2;
		}
		CUIRect Block = BeginInvalidBlock(LeftColumn, Rows, BcLocalize("Atmosphere"), Ui(), &g_Config.m_BcAtmosphere);
		static CUi::SDropDownState s_WeatherState;
		static CScrollRegion s_WeatherScroll;
		static const char *apWeatherTypes[] = {
			BcLocalize("Snow"),
			BcLocalize("Rain"),
			BcLocalize("Fireflies"),
			BcLocalize("Dust"),
		};
		static CButtonContainer s_SnowColor;
		static CButtonContainer s_RainColor;
		static CButtonContainer s_FireflyColor;

		RowCheck(Block, &g_Config.m_BcAtmosphere, BcLocalize("Weather around you"), this);
		RowDropDown(Block, BcLocalize("Type"), g_Config.m_BcWeather, apWeatherTypes, (int)std::size(apWeatherTypes), s_WeatherState, s_WeatherScroll, Ui());
		RowSlider(Block, &g_Config.m_BcWeatherAmount, BcLocalize("Amount"), 25, 300, "%", Ui());
		RowNote(Block, BcLocalize("Rendered by BestClient's atmosphere system."), Ui());

		if(g_Config.m_BcAtmosphere)
		{
			if(g_Config.m_BcWeather == 0)
			{
				RowCheck(Block, &g_Config.m_BcWeatherSnowflakes, BcLocalize("Snowflake sprites"), this);
				RowSlider(Block, &g_Config.m_BcWeatherSnowSize, BcLocalize("Size"), 25, 300, "%", Ui());
				RowSlider(Block, &g_Config.m_BcWeatherSnowSpeed, BcLocalize("Speed"), 25, 300, "%", Ui());
				RowColor(Block, &s_SnowColor, BcLocalize("Color"), &g_Config.m_BcWeatherSnowColor, 255);
			}
			else if(g_Config.m_BcWeather == 1)
			{
				RowSlider(Block, &g_Config.m_BcWeatherRainSize, BcLocalize("Size"), 25, 300, "%", Ui());
				RowSlider(Block, &g_Config.m_BcWeatherRainSpeed, BcLocalize("Speed"), 25, 300, "%", Ui());
				RowSlider(Block, &g_Config.m_BcWeatherRainTilt, BcLocalize("Tilt"), -75, 75, "°", Ui());
				RowColor(Block, &s_RainColor, BcLocalize("Color"), &g_Config.m_BcWeatherRainColor, 10063569);
			}
			else if(g_Config.m_BcWeather == 2)
			{
				RowSlider(Block, &g_Config.m_BcWeatherFireflySize, BcLocalize("Size"), 25, 300, "%", Ui());
				RowSlider(Block, &g_Config.m_BcWeatherFireflySpeed, BcLocalize("Speed"), 25, 300, "%", Ui());
				RowColor(Block, &s_FireflyColor, BcLocalize("Color"), &g_Config.m_BcWeatherFireflyColor, 3791782);
			}
			else
			{
				RowSlider(Block, &g_Config.m_BcWeatherDustSize, BcLocalize("Size"), 25, 300, "%", Ui());
				RowSlider(Block, &g_Config.m_BcWeatherDustSpeed, BcLocalize("Speed"), 25, 300, "%", Ui());
			}
		}
	}

	{
		CUIRect Block = BeginInvalidBlock(RightColumn, 4, BcLocalize("Ambient dust"), Ui(), &g_Config.m_BcAmbientDust);
		static CButtonContainer s_DustColor;
		RowCheck(Block, &g_Config.m_BcAmbientDust, BcLocalize("Drifting dust motes"), this);
		RowSlider(Block, &g_Config.m_BcAmbientDustAmount, BcLocalize("Amount"), 25, 200, "%", Ui());
		RowColor(Block, &s_DustColor, BcLocalize("Color"), &g_Config.m_BcAmbientDustColor, 1981660);
		RowNote(Block, BcLocalize("A quiet layer of dust that follows the camera."), Ui());
	}

	{
		CUIRect Block = BeginInvalidBlock(RightColumn, 4, BcLocalize("Where is the rest?"), Ui());
		RowNote(Block, BcLocalize("Freeze lightning and hook effects live in BestClient -> Visuals."), Ui());
		RowNote(Block, BcLocalize("Weather is fully client-side and does not affect gameplay."), Ui());
		RowNote(Block, BcLocalize("The Invalid freeze warning is on the Visuals tab."), Ui());
		static CButtonContainer s_OpenBestClientButton;
		if(RowButton(Block, &s_OpenBestClientButton, BcLocalize("Open BestClient visuals"), this))
		{
			SetBestClientSettingsTab(0);
			g_Config.m_UiSettingsPage = SETTINGS_BESTCLIENT;
		}
	}

	s_ScrollRegion.End();
}

void CMenus::RenderSettingsInvalidMisc(CUIRect MainView)
{
	static CScrollRegion s_ScrollRegion;
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 60.0f;
	ScrollParams.m_ScrollbarMargin = 5.0f;
	s_ScrollRegion.Begin(&MainView, &ScrollParams);
	MainView.VSplitRight(5.0f, &MainView, nullptr);
	MainView.VSplitLeft(5.0f, nullptr, &MainView);

	CUIRect LeftView, RightView;
	MainView.VSplitMid(&LeftView, &RightView, 30.0f);
	LeftView.VSplitLeft(5.0f, nullptr, &LeftView);
	RightView.VSplitRight(5.0f, &RightView, nullptr);

	CUIRect LeftColumn = LeftView;
	CUIRect RightColumn = RightView;
	LeftColumn.HSplitTop(10.0f, nullptr, &LeftColumn);
	RightColumn.HSplitTop(10.0f, nullptr, &RightColumn);

	{
		CUIRect Block = BeginInvalidBlock(LeftColumn, 3, BcLocalize("Overlays"), Ui(), &g_Config.m_ClInvalidSpectatorWarning);
		RowCheck(Block, &g_Config.m_ClInvalidSpectatorWarning, BcLocalize("Show who is spectating you"), this);
		RowNote(Block, BcLocalize("A small list in the top-left corner while online."), Ui());
		RowNote(Block, BcLocalize("BestClient has its own spectator notification as well."), Ui());
	}

	{
		CUIRect Block = BeginInvalidBlock(LeftColumn, 5, BcLocalize("Reset"), Ui());
		static CButtonContainer s_ResetVisualsButton;
		static CButtonContainer s_ResetEffectsButton;
		static CButtonContainer s_ResetChatButton;
		static CButtonContainer s_ResetAllButton;

		if(RowButton(Block, &s_ResetVisualsButton, BcLocalize("Reset cosmetics and particles"), this))
			ResetInvalidVisuals();
		if(RowButton(Block, &s_ResetEffectsButton, BcLocalize("Reset pee and freeze warning"), this))
			ResetInvalidEffects();
		if(RowButton(Block, &s_ResetChatButton, BcLocalize("Reset chat helpers and markers"), this))
			ResetInvalidChat();
		if(RowButton(Block, &s_ResetAllButton, BcLocalize("Reset all Invalid settings"), this))
		{
			ResetInvalidVisuals();
			ResetInvalidEffects();
			ResetInvalidChat();
		}
		RowNote(Block, BcLocalize("Key binds are not touched by these buttons."), Ui());
	}

	{
		CUIRect Block = BeginInvalidBlock(RightColumn, 6, BcLocalize("About Invalid"), Ui());
		RowNote(Block, BcLocalize("A separate section for cosmetic and quality-of-life extras."), Ui());
		RowNote(Block, BcLocalize("Everything here is client-side: no input automation,"), Ui());
		RowNote(Block, BcLocalize("no freeze avoidance and no server-side advantages."), Ui());
		RowNote(Block, BcLocalize("Effects never change movement, aim or physics."), Ui());
		RowNote(Block, BcLocalize("Some effects are experimental; report bugs in the repo."), Ui());
		RowNote(Block, "Invalid v20.3-invalid2", Ui());
	}

	{
		CUIRect Block = BeginInvalidBlock(RightColumn, 3, BcLocalize("Integration"), Ui());
		static CButtonContainer s_OpenBestClientButton;
		if(RowButton(Block, &s_OpenBestClientButton, BcLocalize("Open BestClient settings"), this))
		{
			SetBestClientSettingsTab(0);
			g_Config.m_UiSettingsPage = SETTINGS_BESTCLIENT;
		}
		RowNote(Block, BcLocalize("Invalid reuses BestClient's theme, widgets and search."), Ui());
		RowNote(Block, BcLocalize("Weather, lightning and hooks are configured in BestClient."), Ui());
	}

	s_ScrollRegion.End();
}
