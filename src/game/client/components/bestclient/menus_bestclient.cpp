/* Copyright © 2026 BestProject Team */
#include <engine/shared/config.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/font_icons.h>
#include <engine/graphics.h>
#include <engine/client.h>
#include <engine/keys.h>
#include <engine/updater.h>

#include <base/io.h>
#include <base/math.h>
#include <base/str.h>
#include <base/time.h>

#include <cstdlib>

#include <game/client/bc_menu_badges.h>
#include <game/client/bc_ui_animations.h>
#include <game/client/components/bestclient/fast_actions.h>
#include <game/client/components/bestclient/gradient.h>
#include <game/client/components/bestclient/bestclient.h>
#include <game/client/components/bestclient/cursor_trail.h>
#include <game/client/components/bestclient/physicball.h>
#include <game/client/components/bestclient/media_decoder.h>
#include <game/client/components/bestclient/menu_media_background.h>
#include <game/client/components/bestclient/settings_search.h>
#include <game/client/components/bestclient/ui_theme/style.h>
#include <game/client/components/bestclient/ui_theme/widgets.h>
#include <game/client/components/hud_layout.h>
#include <game/client/components/menus.h>
#include <game/client/gameclient.h>
#include <game/client/lineinput.h>
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// bestclient
enum
{
	BESTCLIENT_TAB_VISUALS = 0,
	BESTCLIENT_TAB_GAMEPLAY,
	BESTCLIENT_TAB_OTHERS,
	BESTCLIENT_TAB_ANTICHEAT,
	BESTCLIENT_TAB_CONFIGS,
	BESTCLIENT_TAB_INFO,
	NUM_BESTCLIENT_TABS,
};

static int s_CurBestClientTab = BESTCLIENT_TAB_VISUALS;
static bool s_ShowBestClientFun = false;

void CMenus::SetBestClientSettingsTab(int Tab)
{
	if(Tab < BESTCLIENT_TAB_VISUALS || Tab >= NUM_BESTCLIENT_TABS)
		Tab = BESTCLIENT_TAB_VISUALS;
	s_CurBestClientTab = Tab;
}

int CMenus::GetBestClientSettingsTab() const
{
	return s_CurBestClientTab;
}

void CMenus::OpenBestClientFun()
{
	s_CurBestClientTab = BESTCLIENT_TAB_INFO;
	s_ShowBestClientFun = true;
}

void CMenus::CloseBestClientFun()
{
	s_ShowBestClientFun = false;
}

void CMenus::FinishWelcomeToStylePicker()
{
	m_Popup = g_Config.m_BcShowStylePicker ? POPUP_STYLE_PICKER : POPUP_NONE;
}

void CMenus::RenderPopupStylePicker(CUIRect Box)
{
	CUIRect Buttons, Left, Right, Label, Row, Content, Tabs;
	Box.HSplitBottom(8.0f, &Box, nullptr);
	Box.HSplitBottom(28.0f, &Box, &Buttons);
	Box.HSplitBottom(8.0f, &Box, nullptr);
	Box.VMargin(16.0f, &Box);
	Buttons.VMargin(40.0f, &Buttons);

	Box.VSplitMid(&Left, &Right, 16.0f);

	const int SavedStyle = g_Config.m_BcMenuUiStyle;
	const int SavedCheckbox = g_Config.m_BcMenuUiNewCheckbox;
	const int SavedScrollbar = g_Config.m_BcMenuUiNewScrollbar;
	const int SavedColorPicker = g_Config.m_BcMenuUiNewColorPicker;
	const int SavedTabOption = g_Config.m_BcMenuUiNewTabOption;
	const int SavedTabSide = g_Config.m_BcMenuUiTabSide;
	const int SavedBetterFont = g_Config.m_BcMenuUiBetterFont;

	static int s_LegacyCheck = 1;
	static int s_NewCheck = 1;
	static float s_LegacyScroll = 0.35f;
	static float s_NewScroll = 0.65f;
	static unsigned s_LegacyColor = 0xE4A046AFU;
	static unsigned s_NewColor = 0xE4A046AFU;
	static CButtonContainer s_LegacyColorReset;
	static CButtonContainer s_NewColorReset;
	static CButtonContainer s_LegacyTabGeneral;
	static CButtonContainer s_LegacyTabAssets;
	static CButtonContainer s_LegacyTabTee;
	static CButtonContainer s_LegacyTabBestClient;
	static CButtonContainer s_NewTabDdnet;
	static CButtonContainer s_NewTabTclient;
	static CButtonContainer s_NewTabBestClient;
	static int s_LegacyTab = 0;
	static int s_NewTab = 2;

	const auto ApplyPreviewStyle = [&](bool BetterSide) {
		g_Config.m_BcMenuUiStyle = BetterSide ? 1 : 0;
		g_Config.m_BcMenuUiNewCheckbox = 1;
		g_Config.m_BcMenuUiNewScrollbar = 1;
		g_Config.m_BcMenuUiNewColorPicker = BetterSide ? 1 : 0;
		g_Config.m_BcMenuUiNewTabOption = BetterSide ? 1 : 0;
		g_Config.m_BcMenuUiTabSide = 1;
		g_Config.m_BcMenuUiBetterFont = BetterSide ? 1 : 0;
		if(BetterSide)
			TextRender()->SetCustomFace(BESTCLIENT_BETTER_FONT_FACE);
		else
			TextRender()->SetCustomFace("DejaVu Sans");
	};

	const auto DrawSide = [&](CUIRect Panel, bool BetterSide, int *pCheck, float *pScroll, unsigned *pColor, CButtonContainer *pColorReset, const char *pTitle) {
		Panel.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.35f), IGraphics::CORNER_ALL, 10.0f);
		Panel.Margin(10.0f, &Panel);

		ApplyPreviewStyle(BetterSide);

		Panel.HSplitTop(20.0f, &Label, &Panel);
		Ui()->DoLabel(&Label, pTitle, 18.0f, TEXTALIGN_MC);
		Panel.HSplitTop(8.0f, nullptr, &Panel);

		Panel.VSplitRight(100.0f, &Content, &Tabs);
		Content.VSplitRight(8.0f, &Content, nullptr);

		if(BetterSide)
		{
			Tabs.HSplitTop(26.0f, &Row, &Tabs);
			if(DoButton_MenuTab(&s_NewTabDdnet, BcLocalize("DDNet"), s_NewTab == 0, &Row, IGraphics::CORNER_R, nullptr, nullptr, nullptr, nullptr, 10.0f))
				s_NewTab = 0;
			Tabs.HSplitTop(8.0f, nullptr, &Tabs);
			Tabs.HSplitTop(26.0f, &Row, &Tabs);
			if(DoButton_MenuTab(&s_NewTabTclient, "TClient", s_NewTab == 1, &Row, IGraphics::CORNER_R, nullptr, nullptr, nullptr, nullptr, 10.0f))
				s_NewTab = 1;
			Tabs.HSplitTop(8.0f, nullptr, &Tabs);
			Tabs.HSplitTop(26.0f, &Row, &Tabs);
			if(DoButton_MenuTab(&s_NewTabBestClient, BcLocalize("BestClient"), s_NewTab == 2, &Row, IGraphics::CORNER_R, nullptr, nullptr, nullptr, nullptr, 10.0f))
				s_NewTab = 2;
		}
		else
		{
			Tabs.HSplitTop(22.0f, &Row, &Tabs);
			if(DoButton_MenuTab(&s_LegacyTabGeneral, BcLocalize("General"), s_LegacyTab == 0, &Row, IGraphics::CORNER_R, nullptr, nullptr, nullptr, nullptr, 4.0f))
				s_LegacyTab = 0;
			Tabs.HSplitTop(4.0f, nullptr, &Tabs);
			Tabs.HSplitTop(22.0f, &Row, &Tabs);
			if(DoButton_MenuTab(&s_LegacyTabAssets, BcLocalize("Assets"), s_LegacyTab == 1, &Row, IGraphics::CORNER_R, nullptr, nullptr, nullptr, nullptr, 4.0f))
				s_LegacyTab = 1;
			Tabs.HSplitTop(4.0f, nullptr, &Tabs);
			Tabs.HSplitTop(22.0f, &Row, &Tabs);
			if(DoButton_MenuTab(&s_LegacyTabTee, BcLocalize("Tee"), s_LegacyTab == 2, &Row, IGraphics::CORNER_R, nullptr, nullptr, nullptr, nullptr, 4.0f))
				s_LegacyTab = 2;
			Tabs.HSplitTop(4.0f, nullptr, &Tabs);
			Tabs.HSplitTop(22.0f, &Row, &Tabs);
			if(DoButton_MenuTab(&s_LegacyTabBestClient, BcLocalize("BestClient"), s_LegacyTab == 3, &Row, IGraphics::CORNER_R, nullptr, nullptr, nullptr, nullptr, 4.0f))
				s_LegacyTab = 3;
		}

		Content.HSplitTop(22.0f, &Row, &Content);
		if(DoButton_CheckBox(pCheck, BcLocalize("Checkbox"), *pCheck, &Row))
			*pCheck ^= 1;

		Content.HSplitTop(6.0f, nullptr, &Content);
		Content.HSplitTop(18.0f, &Row, &Content);
		Ui()->DoLabel(&Row, BcLocalize("Sample text Aa"), 16.0f, TEXTALIGN_ML);

		Content.HSplitTop(6.0f, nullptr, &Content);
		Content.HSplitTop(16.0f, &Row, &Content);
		*pScroll = Ui()->DoScrollbarH(pScroll, &Row, *pScroll);

		Content.HSplitTop(6.0f, nullptr, &Content);
		DoLine_ColorPicker(pColorReset, 25.0f, 13.0f, 0.0f, &Content, BcLocalize("Color"), pColor, color_cast<ColorRGBA>(ColorHSLA(0xE4A046AFU, true)), false, nullptr, true);
	};

	DrawSide(Left, false, &s_LegacyCheck, &s_LegacyScroll, &s_LegacyColor, &s_LegacyColorReset, BcLocalize("Legacy"));
	DrawSide(Right, true, &s_NewCheck, &s_NewScroll, &s_NewColor, &s_NewColorReset, BcLocalize("Better UI"));

	g_Config.m_BcMenuUiStyle = SavedStyle;
	g_Config.m_BcMenuUiNewCheckbox = SavedCheckbox;
	g_Config.m_BcMenuUiNewScrollbar = SavedScrollbar;
	g_Config.m_BcMenuUiNewColorPicker = SavedColorPicker;
	g_Config.m_BcMenuUiNewTabOption = SavedTabOption;
	g_Config.m_BcMenuUiTabSide = SavedTabSide;
	g_Config.m_BcMenuUiBetterFont = SavedBetterFont;
	BestClientApplyMenuFont(TextRender());

	if(Ui()->IsPopupOpen(&m_ColorPickerPopupContext))
	{
		if(m_ColorPickerPopupContext.m_pHslaColor == &s_LegacyColor)
		{
			g_Config.m_BcMenuUiStyle = 0;
			g_Config.m_BcMenuUiNewColorPicker = 0;
		}
		else if(m_ColorPickerPopupContext.m_pHslaColor == &s_NewColor)
		{
			g_Config.m_BcMenuUiStyle = 1;
			g_Config.m_BcMenuUiNewColorPicker = 1;
		}
	}

	CUIRect LegacyBtn, NewBtn;
	Buttons.VSplitMid(&LegacyBtn, &NewBtn, 20.0f);
	static CButtonContainer s_LegacyButton;
	static CButtonContainer s_NewButton;
	if(DoButton_Menu(&s_LegacyButton, BcLocalize("Choose Legacy"), 0, &LegacyBtn))
	{
		g_Config.m_BcMenuUiStyle = 0;
		g_Config.m_BcMenuUiBetterFont = 0;
		g_Config.m_BcShowStylePicker = 0;
		Ui()->ClosePopupMenus();
		BestClientRefreshMenuFont(GameClient());
		m_Popup = POPUP_NONE;
	}
	if(DoButton_Menu(&s_NewButton, BcLocalize("Choose Better UI"), 0, &NewBtn))
	{
		g_Config.m_BcMenuUiStyle = 1;
		g_Config.m_BcMenuUiNewCheckbox = 1;
		g_Config.m_BcMenuUiNewScrollbar = 1;
		g_Config.m_BcMenuUiNewColorPicker = 1;
		g_Config.m_BcMenuUiNewTabOption = 1;
		g_Config.m_BcMenuUiTabSide = 1;
		g_Config.m_BcMenuUiBetterFont = 1;
		g_Config.m_BcShowStylePicker = 0;
		Ui()->ClosePopupMenus();
		BestClientRefreshMenuFont(GameClient());
		m_Popup = POPUP_NONE;
	}
}
static unsigned BcUiThemeParseHex(const char *pValue)
{
	if(pValue[0] == '$')
		++pValue;
	return (unsigned)str_toulong_base(pValue, 16);
}

static bool BcUiThemeParseKvInt(const char *pToken, const char *pKey, int *pOut)
{
	const int KeyLen = str_length(pKey);
	if(str_comp_nocase_num(pToken, pKey, KeyLen) != 0 || pToken[KeyLen] != '=')
		return false;
	*pOut = str_toint(pToken + KeyLen + 1);
	return true;
}

static bool BcUiThemeParseKvColor(const char *pToken, const char *pKey, unsigned *pOut)
{
	const int KeyLen = str_length(pKey);
	if(str_comp_nocase_num(pToken, pKey, KeyLen) != 0 || pToken[KeyLen] != '=')
		return false;
	*pOut = BcUiThemeParseHex(pToken + KeyLen + 1);
	return true;
}

struct SBcUiThemeElem
{
	const char *m_pName;
	int *m_pEnable;
	unsigned *m_pColor;
	int *m_pGradient;
	unsigned *m_pGradientColor;
	int *m_pAccent;
	unsigned *m_pAccentColor;
	int *m_pAnimate;
	int *m_pOpacity;
	int m_DefEnable;
	unsigned m_DefColor;
	int m_DefGradient;
	unsigned m_DefGradientColor;
	int m_DefAccent;
	unsigned m_DefAccentColor;
	int m_DefAnimate;
	int m_DefOpacity;
};

static SBcUiThemeElem *BcUiThemeElems(int &Count)
{
	static SBcUiThemeElem s_aElems[] = {
		{"text", &g_Config.m_BcCustomUiText, &g_Config.m_BcCustomUiTextColor, &g_Config.m_BcCustomUiTextGradient, &g_Config.m_BcCustomUiTextGradientColor, &g_Config.m_BcCustomUiTextUseAccent, &g_Config.m_BcCustomUiTextAccentColor, &g_Config.m_BcCustomUiTextAnimateSpeed, nullptr, DefaultConfig::BcCustomUiText, DefaultConfig::BcCustomUiTextColor, DefaultConfig::BcCustomUiTextGradient, DefaultConfig::BcCustomUiTextGradientColor, DefaultConfig::BcCustomUiTextUseAccent, DefaultConfig::BcCustomUiTextAccentColor, DefaultConfig::BcCustomUiTextAnimateSpeed, -1},
		{"checkbox", &g_Config.m_BcCustomUiCheckbox, &g_Config.m_BcCustomUiCheckboxColor, &g_Config.m_BcCustomUiCheckboxGradient, &g_Config.m_BcCustomUiCheckboxGradientColor, &g_Config.m_BcCustomUiCheckboxUseAccent, &g_Config.m_BcCustomUiCheckboxAccentColor, &g_Config.m_BcCustomUiCheckboxAnimateSpeed, nullptr, DefaultConfig::BcCustomUiCheckbox, DefaultConfig::BcCustomUiCheckboxColor, DefaultConfig::BcCustomUiCheckboxGradient, DefaultConfig::BcCustomUiCheckboxGradientColor, DefaultConfig::BcCustomUiCheckboxUseAccent, DefaultConfig::BcCustomUiCheckboxAccentColor, DefaultConfig::BcCustomUiCheckboxAnimateSpeed, -1},
		{"scrollbar", &g_Config.m_BcCustomUiScrollbar, &g_Config.m_BcCustomUiScrollbarColor, &g_Config.m_BcCustomUiScrollbarGradient, &g_Config.m_BcCustomUiScrollbarGradientColor, &g_Config.m_BcCustomUiScrollbarUseAccent, &g_Config.m_BcCustomUiScrollbarAccentColor, &g_Config.m_BcCustomUiScrollbarAnimateSpeed, nullptr, DefaultConfig::BcCustomUiScrollbar, DefaultConfig::BcCustomUiScrollbarColor, DefaultConfig::BcCustomUiScrollbarGradient, DefaultConfig::BcCustomUiScrollbarGradientColor, DefaultConfig::BcCustomUiScrollbarUseAccent, DefaultConfig::BcCustomUiScrollbarAccentColor, DefaultConfig::BcCustomUiScrollbarAnimateSpeed, -1},
		{"button", &g_Config.m_BcCustomUiButton, &g_Config.m_BcCustomUiButtonColor, &g_Config.m_BcCustomUiButtonGradient, &g_Config.m_BcCustomUiButtonGradientColor, &g_Config.m_BcCustomUiButtonUseAccent, &g_Config.m_BcCustomUiButtonAccentColor, &g_Config.m_BcCustomUiButtonAnimateSpeed, nullptr, DefaultConfig::BcCustomUiButton, DefaultConfig::BcCustomUiButtonColor, DefaultConfig::BcCustomUiButtonGradient, DefaultConfig::BcCustomUiButtonGradientColor, DefaultConfig::BcCustomUiButtonUseAccent, DefaultConfig::BcCustomUiButtonAccentColor, DefaultConfig::BcCustomUiButtonAnimateSpeed, -1},
		{"droplist", &g_Config.m_BcCustomUiDropdown, &g_Config.m_BcCustomUiDropdownColor, &g_Config.m_BcCustomUiDropdownGradient, &g_Config.m_BcCustomUiDropdownGradientColor, &g_Config.m_BcCustomUiDropdownUseAccent, &g_Config.m_BcCustomUiDropdownAccentColor, &g_Config.m_BcCustomUiDropdownAnimateSpeed, nullptr, DefaultConfig::BcCustomUiDropdown, DefaultConfig::BcCustomUiDropdownColor, DefaultConfig::BcCustomUiDropdownGradient, DefaultConfig::BcCustomUiDropdownGradientColor, DefaultConfig::BcCustomUiDropdownUseAccent, DefaultConfig::BcCustomUiDropdownAccentColor, DefaultConfig::BcCustomUiDropdownAnimateSpeed, -1},
		{"input", &g_Config.m_BcCustomUiEditBox, &g_Config.m_BcCustomUiEditBoxColor, &g_Config.m_BcCustomUiEditBoxGradient, &g_Config.m_BcCustomUiEditBoxGradientColor, &g_Config.m_BcCustomUiEditBoxUseAccent, &g_Config.m_BcCustomUiEditBoxAccentColor, &g_Config.m_BcCustomUiEditBoxAnimateSpeed, nullptr, DefaultConfig::BcCustomUiEditBox, DefaultConfig::BcCustomUiEditBoxColor, DefaultConfig::BcCustomUiEditBoxGradient, DefaultConfig::BcCustomUiEditBoxGradientColor, DefaultConfig::BcCustomUiEditBoxUseAccent, DefaultConfig::BcCustomUiEditBoxAccentColor, DefaultConfig::BcCustomUiEditBoxAnimateSpeed, -1},
		{"block", &g_Config.m_BcCustomUiBlock, &g_Config.m_BcCustomUiBlockColor, &g_Config.m_BcCustomUiBlockGradient, &g_Config.m_BcCustomUiBlockGradientColor, &g_Config.m_BcCustomUiBlockUseAccent, &g_Config.m_BcCustomUiBlockAccentColor, &g_Config.m_BcCustomUiBlockAnimateSpeed, &g_Config.m_BcCustomUiBlockOpacity, DefaultConfig::BcCustomUiBlock, DefaultConfig::BcCustomUiBlockColor, DefaultConfig::BcCustomUiBlockGradient, DefaultConfig::BcCustomUiBlockGradientColor, DefaultConfig::BcCustomUiBlockUseAccent, DefaultConfig::BcCustomUiBlockAccentColor, DefaultConfig::BcCustomUiBlockAnimateSpeed, DefaultConfig::BcCustomUiBlockOpacity},
		{"panel", &g_Config.m_BcCustomUiMainPanel, &g_Config.m_BcCustomUiMainPanelColor, &g_Config.m_BcCustomUiMainPanelGradient, &g_Config.m_BcCustomUiMainPanelGradientColor, &g_Config.m_BcCustomUiMainPanelUseAccent, &g_Config.m_BcCustomUiMainPanelAccentColor, &g_Config.m_BcCustomUiMainPanelAnimateSpeed, &g_Config.m_BcCustomUiMainPanelOpacity, DefaultConfig::BcCustomUiMainPanel, DefaultConfig::BcCustomUiMainPanelColor, DefaultConfig::BcCustomUiMainPanelGradient, DefaultConfig::BcCustomUiMainPanelGradientColor, DefaultConfig::BcCustomUiMainPanelUseAccent, DefaultConfig::BcCustomUiMainPanelAccentColor, DefaultConfig::BcCustomUiMainPanelAnimateSpeed, DefaultConfig::BcCustomUiMainPanelOpacity},
		{"nav", &g_Config.m_BcCustomUiNavTab, &g_Config.m_BcCustomUiNavTabColor, &g_Config.m_BcCustomUiNavTabGradient, &g_Config.m_BcCustomUiNavTabGradientColor, &g_Config.m_BcCustomUiNavTabUseAccent, &g_Config.m_BcCustomUiNavTabAccentColor, &g_Config.m_BcCustomUiNavTabAnimateSpeed, &g_Config.m_BcCustomUiNavTabOpacity, DefaultConfig::BcCustomUiNavTab, DefaultConfig::BcCustomUiNavTabColor, DefaultConfig::BcCustomUiNavTabGradient, DefaultConfig::BcCustomUiNavTabGradientColor, DefaultConfig::BcCustomUiNavTabUseAccent, DefaultConfig::BcCustomUiNavTabAccentColor, DefaultConfig::BcCustomUiNavTabAnimateSpeed, DefaultConfig::BcCustomUiNavTabOpacity},
		{"badge", &g_Config.m_BcCustomUiBadge, &g_Config.m_BcCustomUiBadgeColor, &g_Config.m_BcCustomUiBadgeGradient, &g_Config.m_BcCustomUiBadgeGradientColor, &g_Config.m_BcCustomUiBadgeUseAccent, &g_Config.m_BcCustomUiBadgeAccentColor, &g_Config.m_BcCustomUiBadgeAnimateSpeed, nullptr, DefaultConfig::BcCustomUiBadge, DefaultConfig::BcCustomUiBadgeColor, DefaultConfig::BcCustomUiBadgeGradient, DefaultConfig::BcCustomUiBadgeGradientColor, DefaultConfig::BcCustomUiBadgeUseAccent, DefaultConfig::BcCustomUiBadgeAccentColor, DefaultConfig::BcCustomUiBadgeAnimateSpeed, -1},
		{"binds", &g_Config.m_BcCustomUiBind, &g_Config.m_BcCustomUiBindColor, &g_Config.m_BcCustomUiBindGradient, &g_Config.m_BcCustomUiBindGradientColor, &g_Config.m_BcCustomUiBindUseAccent, &g_Config.m_BcCustomUiBindAccentColor, &g_Config.m_BcCustomUiBindAnimateSpeed, nullptr, DefaultConfig::BcCustomUiBind, DefaultConfig::BcCustomUiBindColor, DefaultConfig::BcCustomUiBindGradient, DefaultConfig::BcCustomUiBindGradientColor, DefaultConfig::BcCustomUiBindUseAccent, DefaultConfig::BcCustomUiBindAccentColor, DefaultConfig::BcCustomUiBindAnimateSpeed, -1},
	};
	Count = (int)std::size(s_aElems);
	return s_aElems;
}

static void BcUiThemeSetAllAnimateSpeeds(int Speed)
{
	Speed = std::clamp(Speed, 0, 100);
	g_Config.m_BcCustomUiGradientAnimateSpeed = Speed;
	int Count = 0;
	SBcUiThemeElem *pElems = BcUiThemeElems(Count);
	for(int i = 0; i < Count; ++i)
		*pElems[i].m_pAnimate = Speed;
}

static void BcUiThemeApplyDefaults()
{
	g_Config.m_BcCustomUiBackground = DefaultConfig::BcCustomUiBackground;
	int Count = 0;
	SBcUiThemeElem *pElems = BcUiThemeElems(Count);
	for(int i = 0; i < Count; ++i)
	{
		const SBcUiThemeElem &Elem = pElems[i];
		*Elem.m_pEnable = Elem.m_DefEnable;
		*Elem.m_pColor = Elem.m_DefColor;
		*Elem.m_pGradient = Elem.m_DefGradient;
		*Elem.m_pGradientColor = Elem.m_DefGradientColor;
		*Elem.m_pAccent = Elem.m_DefAccent;
		*Elem.m_pAccentColor = Elem.m_DefAccentColor;
		*Elem.m_pAnimate = Elem.m_DefAnimate;
		if(Elem.m_pOpacity)
			*Elem.m_pOpacity = Elem.m_DefOpacity;
	}
	g_Config.m_BcCustomUiGradientAnimateSpeed = DefaultConfig::BcCustomUiGradientAnimateSpeed;
}

static bool BcUiThemeParseElemLine(const char *pLine, int *pEnable, unsigned *pColor, int *pGradient, unsigned *pGradientColor, int *pAccent, unsigned *pAccentColor, int *pAnimate, int *pOpacity)
{
	char aTok[128];
	const char *pCursor = pLine;
	bool First = true;
	while((pCursor = str_next_token(pCursor, " \t", aTok, sizeof(aTok))))
	{
		if(First)
		{
			First = false;
			continue;
		}
		BcUiThemeParseKvInt(aTok, "enable", pEnable) ||
			BcUiThemeParseKvColor(aTok, "color", pColor) ||
			BcUiThemeParseKvInt(aTok, "gradient", pGradient) ||
			BcUiThemeParseKvColor(aTok, "gradient_color", pGradientColor) ||
			BcUiThemeParseKvInt(aTok, "accent", pAccent) ||
			BcUiThemeParseKvColor(aTok, "accent_color", pAccentColor) ||
			(pAnimate && BcUiThemeParseKvInt(aTok, "animate", pAnimate)) ||
			(pOpacity && BcUiThemeParseKvInt(aTok, "opacity", pOpacity));
	}
	return true;
}

static bool BcUiThemeImportFromText(const char *pText)
{
	if(!pText || !str_startswith(pText, "BC_UI_THEME_EXPORT"))
		return false;

	const char *p = pText;
	while(*p)
	{
		char aLine[512];
		int i = 0;
		while(*p && *p != '\n' && *p != '\r' && i < (int)sizeof(aLine) - 1)
			aLine[i++] = *p++;
		aLine[i] = '\0';
		while(*p == '\n' || *p == '\r')
			++p;
		if(aLine[0] == '\0' || str_startswith(aLine, "BC_UI_THEME_EXPORT"))
			continue;

		if(str_startswith(aLine, "animate_speed="))
		{
			BcUiThemeSetAllAnimateSpeeds(str_toint(aLine + 14));
			continue;
		}
		if(str_startswith(aLine, "background="))
		{
			g_Config.m_BcCustomUiBackground = str_toint(aLine + 11) != 0 ? 1 : 0;
			continue;
		}
		if(str_startswith(aLine, "ui_color="))
		{
			g_Config.m_UiColor = BcUiThemeParseHex(aLine + 9);
			continue;
		}

		int Count = 0;
		SBcUiThemeElem *pElems = BcUiThemeElems(Count);
		for(int Elem = 0; Elem < Count; ++Elem)
		{
			const int NameLen = str_length(pElems[Elem].m_pName);
			if(str_startswith(aLine, pElems[Elem].m_pName) && (aLine[NameLen] == ' ' || aLine[NameLen] == '\t'))
				BcUiThemeParseElemLine(aLine, pElems[Elem].m_pEnable, pElems[Elem].m_pColor, pElems[Elem].m_pGradient, pElems[Elem].m_pGradientColor, pElems[Elem].m_pAccent, pElems[Elem].m_pAccentColor, pElems[Elem].m_pAnimate, pElems[Elem].m_pOpacity);
		}
	}
	int Count = 0;
	SBcUiThemeElem *pElems = BcUiThemeElems(Count);
	for(int Elem = 0; Elem < Count; ++Elem)
	{
		*pElems[Elem].m_pAnimate = std::clamp(*pElems[Elem].m_pAnimate, 0, 100);
		if(pElems[Elem].m_pOpacity)
			*pElems[Elem].m_pOpacity = std::clamp(*pElems[Elem].m_pOpacity, 0, 100);
	}
	return true;
}

namespace BestClientUiTheme
{
void AppendExport(char *pBuf, int Size, bool SkipDefaults)
{
	pBuf[0] = '\0';
	auto AppendLine = [&](const char *pLine) {
		str_append(pBuf, pLine, Size);
		str_append(pBuf, "\n", Size);
	};
	AppendLine("BC_UI_THEME_EXPORT v1");
	const int HeaderLen = str_length(pBuf);

	if(!SkipDefaults || g_Config.m_BcCustomUiBackground != DefaultConfig::BcCustomUiBackground)
	{
		char aLine[128];
		str_format(aLine, sizeof(aLine), "background=%d", g_Config.m_BcCustomUiBackground);
		AppendLine(aLine);
	}
	if(!SkipDefaults || g_Config.m_UiColor != DefaultConfig::UiColor)
	{
		char aLine[128];
		str_format(aLine, sizeof(aLine), "ui_color=$%08X", g_Config.m_UiColor);
		AppendLine(aLine);
	}

	int Count = 0;
	const SBcUiThemeElem *pElems = BcUiThemeElems(Count);
	for(int Elem = 0; Elem < Count; ++Elem)
	{
		const SBcUiThemeElem &Field = pElems[Elem];
		const int Opacity = Field.m_pOpacity ? *Field.m_pOpacity : -1;
		const int DefOpacity = Field.m_pOpacity ? Field.m_DefOpacity : -1;
		if(SkipDefaults && *Field.m_pEnable == Field.m_DefEnable && *Field.m_pColor == Field.m_DefColor && *Field.m_pGradient == Field.m_DefGradient && *Field.m_pGradientColor == Field.m_DefGradientColor && *Field.m_pAccent == Field.m_DefAccent && *Field.m_pAccentColor == Field.m_DefAccentColor && *Field.m_pAnimate == Field.m_DefAnimate && Opacity == DefOpacity)
			continue;
		char aLine[320];
		if(Opacity >= 0)
			str_format(aLine, sizeof(aLine), "%s enable=%d color=$%08X gradient=%d gradient_color=$%08X accent=%d accent_color=$%08X opacity=%d animate=%d", Field.m_pName, *Field.m_pEnable, *Field.m_pColor, *Field.m_pGradient, *Field.m_pGradientColor, *Field.m_pAccent, *Field.m_pAccentColor, Opacity, *Field.m_pAnimate);
		else
			str_format(aLine, sizeof(aLine), "%s enable=%d color=$%08X gradient=%d gradient_color=$%08X accent=%d accent_color=$%08X animate=%d", Field.m_pName, *Field.m_pEnable, *Field.m_pColor, *Field.m_pGradient, *Field.m_pGradientColor, *Field.m_pAccent, *Field.m_pAccentColor, *Field.m_pAnimate);
		AppendLine(aLine);
	}

	if(SkipDefaults && str_length(pBuf) == HeaderLen)
		pBuf[0] = '\0';
}

void SaveFile(IStorage *pStorage)
{
	if(!pStorage)
		return;
	char aBuf[8192];
	AppendExport(aBuf, sizeof(aBuf), true);
	IOHANDLE File = pStorage->OpenFile(FILE_NAME, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
		return;
	if(aBuf[0] != '\0')
		io_write(File, aBuf, str_length(aBuf));
	io_close(File);
}

void LoadFile(IStorage *pStorage)
{
	if(!pStorage)
		return;
	if(!pStorage->FileExists(FILE_NAME, IStorage::TYPE_SAVE))
	{
		if(g_Config.m_BcCustomUiGradientAnimateSpeed != DefaultConfig::BcCustomUiGradientAnimateSpeed)
			BcUiThemeSetAllAnimateSpeeds(g_Config.m_BcCustomUiGradientAnimateSpeed);
		return;
	}
	IOHANDLE File = pStorage->OpenFile(FILE_NAME, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!File)
		return;
	char *pData = io_read_all_str(File);
	io_close(File);
	if(!pData)
		return;
	BcUiThemeApplyDefaults();
	BcUiThemeImportFromText(pData);
	free(pData);
}
} // namespace BestClientUiTheme
void CMenus::RenderSettingsBestClientChatMediaBlock(CUIRect &Column)
{
	const float LineSize = 20.0f;
	const float MarginSmall = 5.0f;
	const float HeadlineFontSize = 20.0f;
	const float MarginBetweenViews = 30.0f;
	const float BlockPadding = MarginBetweenViews * 0.6666f;

	const bool Expanded = g_Config.m_BcChatMediaPreview != 0;
	const float DomainsHeight = g_Config.m_BcChatMediaContentFilter ? 2.0f * (MarginSmall + LineSize) : 0.0f;
	const float ExpandedTargetHeight = 5.0f * (MarginSmall + LineSize) + DomainsHeight;
	const float ExpandedHeight = BC_MODULE_REVEAL(Expanded, ExpandedTargetHeight, Client()->RenderFrameTime());
	const float HeaderHeight = LineSize + MarginSmall + LineSize;
	const float BlockHeight = HeaderHeight + ExpandedHeight;

	CUIRect Block;
	Column.HSplitTop(BlockHeight, &Block, &Column);
	CUIRect BlockBg = Block;
	BlockBg.w += BlockPadding;
	BlockBg.h += BlockPadding;
	BlockBg.x -= BlockPadding * 0.5f;
	BlockBg.y -= BlockPadding * 0.5f;
	BestClientUiTheme::DrawUiBlock(&BlockBg, IGraphics::CORNER_ALL, 10.0f);
	BestClientSettingsSearch::DrawBlockHighlight(&g_Config.m_BcChatMediaPreview, &BlockBg);

	CUIRect Content, Label, Button;
	Block.HSplitTop(LineSize, &Label, &Block);
	Ui()->DoLabel(&Label, BcLocalize("Chat Media"), HeadlineFontSize, TEXTALIGN_ML);
	Block.HSplitTop(MarginSmall, nullptr, &Block);

	CChat &Chat = GameClient()->m_Chat;
	Block.HSplitTop(LineSize, &Content, &Block);
	if(DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcChatMediaPreview, BcLocalize("Render media previews from chat links"), &g_Config.m_BcChatMediaPreview, &Content, LineSize))
		Chat.RebuildChat();

	if(ExpandedHeight <= 0.5f)
		return;

	CUIRect Visible = Block;
	Visible.h = ExpandedHeight;
	Ui()->ClipEnable(&Visible);

	Block.HSplitTop(MarginSmall, nullptr, &Block);
	Block.HSplitTop(LineSize, &Content, &Block);
	if(DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcChatMediaPhotos, BcLocalize("Show photos in chat media"), &g_Config.m_BcChatMediaPhotos, &Content, LineSize))
		Chat.RebuildChat();

	Block.HSplitTop(MarginSmall, nullptr, &Block);
	Block.HSplitTop(LineSize, &Content, &Block);
	if(DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcChatMediaGifs, BcLocalize("Show GIFs in chat media"), &g_Config.m_BcChatMediaGifs, &Content, LineSize))
		Chat.RebuildChat();

	Block.HSplitTop(MarginSmall, nullptr, &Block);
	Block.HSplitTop(LineSize, &Content, &Block);
	if(DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcChatMediaContentFilter, BcLocalize("Content filtering"), &g_Config.m_BcChatMediaContentFilter, &Content, LineSize))
		Chat.RebuildChat();

	if(g_Config.m_BcChatMediaContentFilter)
	{
		Block.HSplitTop(MarginSmall, nullptr, &Block);
		Block.HSplitTop(LineSize, &Label, &Block);
		Ui()->DoLabel(&Label, BcLocalize("Allowed media domains"), 12.0f, TEXTALIGN_ML);
		Block.HSplitTop(MarginSmall, nullptr, &Block);
		Block.HSplitTop(LineSize, &Button, &Block);
		static CLineInput DomainsInput(g_Config.m_BcChatMediaAllowedDomains, sizeof(g_Config.m_BcChatMediaAllowedDomains));
		DomainsInput.SetEmptyText("tenor.com; imgur.com; giphy.com; gifs.teeworlds.xyz");
		if(Ui()->DoClearableEditBox(&DomainsInput, &Button, 14.0f))
			Chat.RebuildChat();
	}

	Block.HSplitTop(MarginSmall, nullptr, &Block);
	Block.HSplitTop(LineSize, &Button, &Block);
	if(Ui()->DoScrollbarOption(&g_Config.m_BcChatMediaPreviewMaxWidth, &g_Config.m_BcChatMediaPreviewMaxWidth, &Button, BcLocalize("Media preview width"), 120, 400))
		Chat.RebuildChat();

	Block.HSplitTop(MarginSmall, nullptr, &Block);
	Block.HSplitTop(LineSize, &Label, &Block);
	static CButtonContainer HideMediaBindReader, HideMediaBindClear;
	DoLine_KeyReader(Label, HideMediaBindReader, HideMediaBindClear, BcLocalize("Hide media bind"), "toggle_chat_media_hidden");
	Ui()->ClipDisable();
}

static void SetBestClientTabFlag(int32_t &Flags, int Tab, bool Hidden)
{
	if(Hidden)
		Flags |= (1 << Tab);
	else
		Flags &= ~(1 << Tab);
}

static bool IsBestClientTabFlagSet(int32_t Flags, int Tab)
{
	return (Flags & (1 << Tab)) != 0;
}

void CMenus::RenderSettingsBestClientConfigPresets(CUIRect MainView)
{
	const float FontSize = 14.0f;
	const float EditBoxFontSize = 12.0f;
	const float LineSize = 20.0f;
	const float MarginSmall = 5.0f;

	CBcConfigPresets &Presets = GameClient()->m_BcConfigPresets;
	static int s_Selected = -1;
	static CLineInputBuffered<CBcConfigPresets::MAX_NAME_LENGTH> s_NameInput;
	if(s_Selected >= (int)Presets.m_vPresets.size())
		s_Selected = (int)Presets.m_vPresets.size() - 1;

	CUIRect Label, Button;
	const bool HasSelection = s_Selected >= 0 && s_Selected < (int)Presets.m_vPresets.size();
	{
		CUIRect Top, NameColumn, Actions;
		MainView.HSplitTop(30.0f + MarginSmall + 30.0f, &Top, &MainView);
		Top.VSplitLeft(300.0f, &NameColumn, &Actions);
		CUIRect NameCard;
		NameColumn.HSplitTop(60.0f, &NameCard, &NameColumn);
		NameCard.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f), IGraphics::CORNER_ALL, 5.0f);
		NameCard.Margin(5.0f, &NameCard);
		{
			CUIRect Icon, Text, NameRow;
			NameCard.VSplitLeft(50.0f, &Icon, &Text);
			RenderFontIcon(Icon, FontIcon::FILE, 28.0f, TEXTALIGN_MC);
			Text.VSplitLeft(5.0f, nullptr, &Text);
			const float RowHeight = Text.h / 2.0f;
			Text.HSplitTop(RowHeight, &NameRow, &Text);
			NameRow.VMargin(2.0f, &NameRow);
			Ui()->DoEditBox(&s_NameInput, &NameRow, EditBoxFontSize);
			s_NameInput.SetEmptyText(BcLocalize("Name"));
			char aTime[64];
			str_timestamp_ex(time_timestamp(), aTime, sizeof(aTime), "%d.%m.%Y %H:%M");
			Ui()->DoLabel(&Text, aTime, RowHeight * 0.55f, TEXTALIGN_ML);
		}

		Actions.VSplitLeft(20.0f, nullptr, &Actions);
		CUIRect Row, LeftButton, RightButton;
		Actions.HSplitTop(30.0f, &Row, &Actions);
		Row.VSplitMid(&LeftButton, &RightButton, MarginSmall);
		static CButtonContainer s_LoadButton;
		if(DoButton_Menu(&s_LoadButton, BcLocalize("Load"), 0, &LeftButton) && HasSelection)
			Presets.Load(Presets.m_vPresets[s_Selected].m_aName);
		static CButtonContainer s_SaveButton;
		if(DoButton_Menu(&s_SaveButton, BcLocalize("Save"), 0, &RightButton))
		{
			const char *pName = s_NameInput.GetString();
			if(Presets.SaveNew(pName))
			{
				for(int i = 0; i < (int)Presets.m_vPresets.size(); ++i)
				{
					if(str_comp(Presets.m_vPresets[i].m_aName, pName) == 0)
						s_Selected = i;
				}
				s_NameInput.Clear();
			}
		}
		Actions.HSplitTop(MarginSmall, nullptr, &Actions);
		Actions.HSplitTop(30.0f, &Row, &Actions);
		Row.VSplitMid(&LeftButton, &RightButton, MarginSmall);
		static CButtonContainer s_DeleteButton;
		if(DoButton_Menu(&s_DeleteButton, BcLocalize("Delete"), 0, &LeftButton) && HasSelection)
		{
			Presets.Delete(Presets.m_vPresets[s_Selected].m_aName);
			if(s_Selected >= (int)Presets.m_vPresets.size())
				s_Selected = (int)Presets.m_vPresets.size() - 1;
		}
		static CButtonContainer s_OverrideButton;
		if(DoButton_Menu(&s_OverrideButton, BcLocalize("Override"), 0, &RightButton) && HasSelection)
			Presets.Override(Presets.m_vPresets[s_Selected].m_aName);
	}

	{
		CUIRect Bottom, Saved;
		MainView.HSplitBottom(LineSize + MarginSmall, &MainView, &Bottom);
		Bottom.HSplitTop(MarginSmall, nullptr, &Bottom);
		Bottom.VSplitLeft(170.0f, &Button, &Bottom);
		static CButtonContainer s_FolderButton;
		char aFolder[64];
		str_format(aFolder, sizeof(aFolder), "%s %s", BcLocalize("Configs"), BcLocalize("Folder"));
		if(DoButton_Menu(&s_FolderButton, aFolder, 0, &Button))
			Presets.OpenFolder();
		Bottom.VSplitRight(120.0f, nullptr, &Saved);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), BcLocalize("Saved: %d"), (int)Presets.m_vPresets.size());
		Ui()->DoLabel(&Saved, aBuf, FontSize, TEXTALIGN_MR);
	}

	MainView.HSplitTop(MarginSmall, nullptr, &MainView);
	static CListBox s_ListBox;
	s_ListBox.DoStart(64.0f, Presets.m_vPresets.size(), MainView.w / 220.0f, 3, s_Selected, &MainView, true, IGraphics::CORNER_ALL, true);
	static std::vector<int> s_Ids;
	if(s_Ids.size() < Presets.m_vPresets.size())
		s_Ids.resize(Presets.m_vPresets.size());
	for(int i = 0; i < (int)Presets.m_vPresets.size(); ++i)
	{
		const CListboxItem Item = s_ListBox.DoNextItem(&s_Ids[i], s_Selected == i);
		if(!Item.m_Visible)
			continue;

		CUIRect Icon, Text;
		Item.m_Rect.VSplitLeft(50.0f, &Icon, &Text);
		Text.VSplitLeft(5.0f, nullptr, &Text);
		RenderFontIcon(Icon, FontIcon::FILE, 28.0f, TEXTALIGN_MC);

		char aTime[64];
		str_timestamp_ex(Presets.m_vPresets[i].m_Time, aTime, sizeof(aTime), "%d.%m.%Y %H:%M");
		char aChanges[64];
		str_format(aChanges, sizeof(aChanges), BcLocalize("Changes: %d"), Presets.m_vPresets[i].m_Changes);
		const float RowHeight = Text.h / 3.0f;
		Text.HSplitTop(RowHeight, &Label, &Text);
		Ui()->DoLabel(&Label, Presets.m_vPresets[i].m_aName, RowHeight * 0.8f, TEXTALIGN_ML);
		Text.HSplitTop(RowHeight, &Label, &Text);
		Ui()->DoLabel(&Label, aTime, RowHeight * 0.7f, TEXTALIGN_ML);
		Ui()->DoLabel(&Text, aChanges, RowHeight * 0.7f, TEXTALIGN_ML);
	}
	s_Selected = s_ListBox.DoEnd();
}

void CMenus::RenderSettingsBestClient(CUIRect MainView)
{
	const float TopExtend = BestClientUiTheme::IsNewTabOption() ? 8.0f : 20.0f;
	MainView.y -= TopExtend;
	MainView.h += TopExtend;

	static CButtonContainer s_aPageTabs[NUM_BESTCLIENT_TABS] = {};

	MainView.HSplitTop(BestClientUiTheme::IsNewTabOption() ? 6.0f : 8.0f, nullptr, &MainView);

	auto IsTabHidden = [&](int Tab) {
		return Tab != BESTCLIENT_TAB_INFO && IsBestClientTabFlagSet(g_Config.m_BcBestClientSettingsTabs, Tab);
	};

	const char *apTabNames[NUM_BESTCLIENT_TABS] = {
		BcLocalize("Visuals"),
		BcLocalize("Gameplay"),
		BcLocalize("Others"),
		BcLocalize("AntiCheat"),
		BcLocalize("Configs"),
		BcLocalize("Info"),
	};
	const int aTabOrder[NUM_BESTCLIENT_TABS] = {
		BESTCLIENT_TAB_VISUALS,
		BESTCLIENT_TAB_GAMEPLAY,
		BESTCLIENT_TAB_OTHERS,
		BESTCLIENT_TAB_ANTICHEAT,
		BESTCLIENT_TAB_CONFIGS,
		BESTCLIENT_TAB_INFO,
	};

	CButtonContainer *apVisibleButtons[NUM_BESTCLIENT_TABS];
	const char *apVisibleNames[NUM_BESTCLIENT_TABS];
	int aVisibleValues[NUM_BESTCLIENT_TABS];
	int VisibleCount = 0;
	int FirstVisibleTab = -1;
	for(const int Tab : aTabOrder)
	{
		if(IsTabHidden(Tab))
			continue;
		if(FirstVisibleTab == -1)
			FirstVisibleTab = Tab;
		apVisibleButtons[VisibleCount] = &s_aPageTabs[Tab];
		apVisibleNames[VisibleCount] = apTabNames[Tab];
		aVisibleValues[VisibleCount] = Tab;
		++VisibleCount;
	}

	if(FirstVisibleTab == -1)
	{
		s_CurBestClientTab = BESTCLIENT_TAB_INFO;
		FirstVisibleTab = BESTCLIENT_TAB_INFO;
		VisibleCount = 1;
		apVisibleButtons[0] = &s_aPageTabs[BESTCLIENT_TAB_INFO];
		apVisibleNames[0] = apTabNames[BESTCLIENT_TAB_INFO];
		aVisibleValues[0] = BESTCLIENT_TAB_INFO;
	}

	if(s_CurBestClientTab < BESTCLIENT_TAB_VISUALS || s_CurBestClientTab >= NUM_BESTCLIENT_TABS || IsTabHidden(s_CurBestClientTab))
		s_CurBestClientTab = FirstVisibleTab;

	int SelectedVisibleIndex = 0;
	if(!BestClientUiTheme::IsNewTabOption())
	{
		CUIRect TabBar, TabButton;
		MainView.HSplitTop(24.0f, &TabBar, &MainView);
		const float TabWidth = TabBar.w / (float)VisibleCount;
		for(int i = 0; i < VisibleCount; ++i)
		{
			TabBar.VSplitLeft(TabWidth, &TabButton, &TabBar);
			const int Corners = i == 0 ? IGraphics::CORNER_L : (i == VisibleCount - 1 ? IGraphics::CORNER_R : IGraphics::CORNER_NONE);
			if(DoButton_MenuTab(apVisibleButtons[i], apVisibleNames[i], s_CurBestClientTab == aVisibleValues[i], &TabButton, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
			{
				s_CurBestClientTab = aVisibleValues[i];
				s_ShowBestClientFun = false;
			}
			if(aVisibleValues[i] == s_CurBestClientTab)
				SelectedVisibleIndex = i;
		}
		MainView.HSplitTop(10.0f, nullptr, &MainView);
	}
	else
	{
		for(int i = 0; i < VisibleCount; ++i)
		{
			if(aVisibleValues[i] == s_CurBestClientTab)
				SelectedVisibleIndex = i;
		}
	}

	if(s_ShowBestClientFun)
	{
		RenderSettingsBestClientFun(MainView);
		return;
	}

	auto RenderTabContent = [&](int Tab, CUIRect Content) {
		if(Tab == BESTCLIENT_TAB_VISUALS)
			RenderSettingsBestClientVisuals(Content);
		else if(Tab == BESTCLIENT_TAB_GAMEPLAY)
			RenderSettingsBestClientGameplay(Content);
		else if(Tab == BESTCLIENT_TAB_OTHERS)
			RenderSettingsBestClientOthers(Content);
		else if(Tab == BESTCLIENT_TAB_ANTICHEAT)
			RenderSettingsBestClientAntiCheat(Content);
		else if(Tab == BESTCLIENT_TAB_CONFIGS)
			RenderSettingsBestClientConfigPresets(Content);
		else if(Tab == BESTCLIENT_TAB_INFO)
			RenderSettingsBestClientInfo(Content);
	};

	if(BestClientUiTheme::IsNewTabOption())
	{
		RenderTabContent(s_CurBestClientTab, MainView);
		return;
	}

	const BCUiAnimations::STabSwitch TabSwitch = BCUiAnimations::AnimateTabSwitch(SelectedVisibleIndex, VisibleCount, Client()->RenderFrameTime(), 0);
	const int FromTab = aVisibleValues[TabSwitch.m_FromIndex];
	const int ToTab = aVisibleValues[TabSwitch.m_ToIndex];
	if(TabSwitch.m_Progress >= 1.0f || FromTab == ToTab)
	{
		RenderTabContent(ToTab, MainView);
		return;
	}

	float FromOffset = 0.0f;
	float ToOffset = 0.0f;
	BCUiAnimations::TabSwitchContentOffsets(TabSwitch.m_Progress, TabSwitch.m_FromIndex, TabSwitch.m_ToIndex, MainView.w, FromOffset, ToOffset);

	CUIRect FromView = MainView;
	CUIRect ToView = MainView;
	FromView.x += FromOffset;
	ToView.x += ToOffset;

	Ui()->ClipEnable(&MainView);
	RenderTabContent(FromTab, FromView);
	RenderTabContent(ToTab, ToView);
	Ui()->ClipDisable();
}
void CMenus::RenderSettingsBestClientAntiCheat(CUIRect MainView)
{
	const float LineSize = 20.0f;
	const float MarginSmall = 5.0f;
	const float HeadlineFontSize = 20.0f;
	const float MarginBetweenViews = 30.0f;
	const float BlockPadding = MarginBetweenViews * 0.6666f;

	CUIRect Label, Content, Block;

	MainView.HSplitTop(LineSize * 3 + MarginSmall * 2 + BlockPadding, &Block, &MainView);

	CUIRect BlockBg = Block;
	BlockBg.w += BlockPadding;
	BlockBg.h += BlockPadding;
	BlockBg.x -= BlockPadding * 0.5f;
	BlockBg.y -= BlockPadding * 0.5f;
	BestClientUiTheme::DrawUiBlock(&BlockBg, IGraphics::CORNER_ALL, 10.0f);

	Block.HSplitTop(LineSize, &Label, &Block);
	Ui()->DoLabel(&Label, "Anti-Cheat Analyzer", HeadlineFontSize, TEXTALIGN_ML);
	Block.HSplitTop(MarginSmall, nullptr, &Block);

	Block.HSplitTop(LineSize, &Content, &Block);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcAntiCheat, "Enable Analysis", &g_Config.m_BcAntiCheat, &Content, LineSize);

	Block.HSplitTop(MarginSmall, nullptr, &Block);

	Block.HSplitTop(LineSize, &Content, &Block);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcAntiCheatShowHud, "Show HUD", &g_Config.m_BcAntiCheatShowHud, &Content, LineSize);
}

void CMenus::RenderSettingsBestClientVisuals(CUIRect MainView)
{
	const float LineSize = 20.0f;
	const float MarginSmall = 5.0f;
	const float HeadlineFontSize = 20.0f;
	const float MarginBetweenViews = 30.0f;
	const float BlockPadding = MarginBetweenViews * 0.6666f;

	static CScrollRegion s_VisualsScrollRegion;
	CScrollRegionParams VisualsScrollParams;
	VisualsScrollParams.m_ScrollUnit = 60.0f;
	VisualsScrollParams.m_ScrollbarMargin = 5.0f;
	s_VisualsScrollRegion.Begin(&MainView, &VisualsScrollParams);
	MainView.VSplitRight(5.0f, &MainView, nullptr);
	MainView.VSplitLeft(5.0f, nullptr, &MainView);
	const float VisualsScrollContentX = MainView.x;
	const float VisualsScrollContentW = MainView.w;

	CUIRect Content, Label, Button, LeftView, RightView, Column;

	MainView.VSplitMid(&LeftView, &RightView, MarginBetweenViews);
	LeftView.VSplitLeft(MarginSmall, nullptr, &LeftView);
	RightView.VSplitRight(MarginSmall, &RightView, nullptr);

	Column = LeftView;
	Column.HSplitTop(10.0f, nullptr, &Column);

	const float RealHitboxColorLineSize = 25.0f;
	const float RealHitboxColorLineSpacing = 5.0f;
	const float RealHitboxColorHeight = g_Config.m_BcShowRealHitbox ? RealHitboxColorLineSize + RealHitboxColorLineSpacing : 0.0f;
	const float VisualsQoLBlockHeight =
		LineSize + MarginSmall +
		11.0f * LineSize +
		RealHitboxColorHeight +
		3.0f * LineSize;
	CUIRect VisualsQoLBlock;
	Column.HSplitTop(VisualsQoLBlockHeight, &VisualsQoLBlock, &Column);

	CUIRect VisualsQoLBlockBg = VisualsQoLBlock;
	VisualsQoLBlockBg.w += BlockPadding;
	VisualsQoLBlockBg.h += BlockPadding;
	VisualsQoLBlockBg.x -= BlockPadding * 0.5f;
	VisualsQoLBlockBg.y -= BlockPadding * 0.5f;
	BestClientUiTheme::DrawUiBlock(&VisualsQoLBlockBg, IGraphics::CORNER_ALL, 10.0f);
	BestClientSettingsSearch::DrawBlockHighlight(&g_Config.m_BcServerMapPreview, &VisualsQoLBlockBg);

	VisualsQoLBlock.HSplitTop(LineSize, &Label, &VisualsQoLBlock);
	Ui()->DoLabel(&Label, BcLocalize("Visuals QoL"), HeadlineFontSize, TEXTALIGN_ML);
	VisualsQoLBlock.HSplitTop(MarginSmall, nullptr, &VisualsQoLBlock);

	{
		CUIRect MapPreviewRow;
		VisualsQoLBlock.HSplitTop(LineSize, &MapPreviewRow, &VisualsQoLBlock);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcServerMapPreview, BcLocalize("Map preview in server list"), &g_Config.m_BcServerMapPreview, &MapPreviewRow, LineSize);
	}
	{
		CUIRect TeeCoordsRow;
		VisualsQoLBlock.HSplitTop(LineSize, &TeeCoordsRow, &VisualsQoLBlock);
		BcMenuBadges::DrawNew(Graphics(), Ui(), TextRender(), &TeeCoordsRow, MarginSmall);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcShowTeeCoords, BcLocalize("Show coordinates above tees"), &g_Config.m_BcShowTeeCoords, &TeeCoordsRow, LineSize);
	}
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcShowRealHitbox, BcLocalize("Show real hitbox"), &g_Config.m_BcShowRealHitbox, &VisualsQoLBlock, LineSize);
	if(g_Config.m_BcShowRealHitbox)
	{
		static CButtonContainer s_RealHitboxDotColorButton;
		DoLine_ColorPicker(&s_RealHitboxDotColorButton, RealHitboxColorLineSize, 13.0f, RealHitboxColorLineSpacing, &VisualsQoLBlock, BcLocalize("Real hitbox dot color"), &g_Config.m_BcShowRealHitboxColor, color_cast<ColorRGBA>(ColorHSLA(DefaultConfig::BcShowRealHitboxColor, true)), false, nullptr, true);
	}
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcShowhudDummyCoordIndicator, BcLocalize("Show player below indicator"), &g_Config.m_BcShowhudDummyCoordIndicator, &VisualsQoLBlock, LineSize);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcShowCorrectCheckpoint, BcLocalize("Show current checkpoint in hud"), &g_Config.m_BcShowCorrectCheckpoint, &VisualsQoLBlock, LineSize);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcShowPointsInTab, BcLocalize("Show points in tab"), &g_Config.m_BcShowPointsInTab, &VisualsQoLBlock, LineSize);
	{
		CUIRect FriendMarkRow;
		VisualsQoLBlock.HSplitTop(LineSize, &FriendMarkRow, &VisualsQoLBlock);
		BcMenuBadges::DrawNew(Graphics(), Ui(), TextRender(), &FriendMarkRow, MarginSmall);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcScoreboardFriendMark, BcLocalize("Show friend icon in scoreboard"), &g_Config.m_BcScoreboardFriendMark, &FriendMarkRow, LineSize);
	}
	{
		CUIRect SpectatorNamesRow;
		VisualsQoLBlock.HSplitTop(LineSize, &SpectatorNamesRow, &VisualsQoLBlock);
		BcMenuBadges::DrawBeta(Graphics(), Ui(), TextRender(), &SpectatorNamesRow, MarginSmall);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcShowSpectatorNames, BcLocalize("Show spectator nicknames"), &g_Config.m_BcShowSpectatorNames, &SpectatorNamesRow, LineSize);
	}
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcShowVotePercentage, BcLocalize("Show vote percentage"), &g_Config.m_BcShowVotePercentage, &VisualsQoLBlock, LineSize);
	{
		CUIRect MaxFpsRow;
		VisualsQoLBlock.HSplitTop(LineSize, &MaxFpsRow, &VisualsQoLBlock);
		BcMenuBadges::DrawNew(Graphics(), Ui(), TextRender(), &MaxFpsRow, MarginSmall);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcDisplayMaxFps, BcLocalize("Display max FPS"), &g_Config.m_BcDisplayMaxFps, &MaxFpsRow, LineSize);
	}
	VisualsQoLBlock.HSplitTop(LineSize, &Button, &VisualsQoLBlock);
	Ui()->DoScrollbarOption(&g_Config.m_BcWheelScale, &g_Config.m_BcWheelScale, &Button, BcLocalize("Bind/Emote wheel scale"), 50, 100, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_DELAYUPDATE, "%");
	VisualsQoLBlock.HSplitTop(LineSize, &Button, &VisualsQoLBlock);
	Ui()->DoScrollbarOption(&g_Config.m_BcScoreboardScale, &g_Config.m_BcScoreboardScale, &Button, BcLocalize("Scoreboard scale"), 50, 100, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_DELAYUPDATE, "%");
	VisualsQoLBlock.HSplitTop(LineSize, &Button, &VisualsQoLBlock);
	Ui()->DoScrollbarOption(&g_Config.m_UiScale, &g_Config.m_UiScale, &Button, BcLocalize("UI scale"), 50, 110, &CUi::ms_LinearScrollbarScale, CUi::SCROLLBAR_OPTION_DELAYUPDATE, "%");

	Column.HSplitTop(MarginBetweenViews, nullptr, &Column);

	const bool JellyExpanded = g_Config.m_BcJellyTee != 0;
	const float JellyHeaderHeight = LineSize + MarginSmall + LineSize;
	const float JellyExpandedTargetHeight = MarginSmall + LineSize + MarginSmall + LineSize + MarginSmall + LineSize;
	const float JellyExpandedHeight = BC_MODULE_REVEAL(JellyExpanded, JellyExpandedTargetHeight, Client()->RenderFrameTime());
	const float JellyBlockHeight = JellyHeaderHeight + JellyExpandedHeight;

	CUIRect JellyBlock;
	Column.HSplitTop(JellyBlockHeight, &JellyBlock, &Column);

	CUIRect JellyBlockBg = JellyBlock;
	JellyBlockBg.w += BlockPadding;
	JellyBlockBg.h += BlockPadding;
	JellyBlockBg.x -= BlockPadding * 0.5f;
	JellyBlockBg.y -= BlockPadding * 0.5f;
	BestClientUiTheme::DrawUiBlock(&JellyBlockBg, IGraphics::CORNER_ALL, 10.0f);
	BestClientSettingsSearch::DrawBlockHighlight(&g_Config.m_BcJellyTee, &JellyBlockBg);

	MainView = JellyBlock;

	MainView.HSplitTop(LineSize, &Label, &MainView);
	CUIRect JellyTitleLabel, JellyResetButton;
	Label.VSplitRight(LineSize + 8.0f, &JellyTitleLabel, &JellyResetButton);
	static CButtonContainer s_JellyTeeResetButton;
	const bool JellyTeeResetClicked = Ui()->DoButton_FontIcon(&s_JellyTeeResetButton, FontIcon::ARROW_ROTATE_LEFT, 0, &JellyResetButton, BUTTONFLAG_LEFT);
	GameClient()->m_Tooltips.DoToolTip(&s_JellyTeeResetButton, &JellyResetButton, BcLocalize("Reset to defaults"));
	if(JellyTeeResetClicked)
	{
		g_Config.m_BcJellyTeeOthers = DefaultConfig::BcJellyTeeOthers;
		g_Config.m_BcJellyTeeStrength = DefaultConfig::BcJellyTeeStrength;
		g_Config.m_BcJellyTeeDuration = DefaultConfig::BcJellyTeeDuration;
	}
	Ui()->DoLabel(&JellyTitleLabel, BcLocalize("Jelly Tee"), HeadlineFontSize, TEXTALIGN_ML);
	MainView.HSplitTop(MarginSmall, nullptr, &MainView);

	MainView.HSplitTop(LineSize, &Content, &MainView);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcJellyTee, BcLocalize("Enable Jelly Tee"), &g_Config.m_BcJellyTee, &Content, LineSize);

	if(JellyExpandedHeight > 0.5f)
	{
		CUIRect Visible = MainView;
		Visible.h = JellyExpandedHeight;
		Ui()->ClipEnable(&Visible);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Content, &MainView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcJellyTeeOthers, BcLocalize("Jelly Others"), &g_Config.m_BcJellyTeeOthers, &Content, LineSize);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_BcJellyTeeStrength, &g_Config.m_BcJellyTeeStrength, &Button, BcLocalize("Jelly strength"), 1, 500);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_BcJellyTeeDuration, &g_Config.m_BcJellyTeeDuration, &Button, BcLocalize("Jelly duration"), 1, 80);

		Ui()->ClipDisable();
	}

	Column.HSplitTop(MarginBetweenViews, nullptr, &Column);

	RenderSettingsBestClientWeaponGlow(Column);

	Column.HSplitTop(MarginBetweenViews, nullptr, &Column);

	const bool CornerIndicatorEnabled = g_Config.m_BcCornerIndicator != 0;
	const bool CornerIndicatorBlocked = GameClient()->m_AspectRatio.IsBlockedByFng();
	const bool CornerIndicatorShowSizes = CornerIndicatorEnabled && g_Config.m_BcCornerIndicatorProximity == 0;
	const bool CornerIndicatorShowCustomColors = CornerIndicatorEnabled && g_Config.m_BcCornerIndicatorCustomColors != 0;
	const float CornerIndicatorColorPickerLineSize = 25.0f;
	const float CornerIndicatorColorPickerSpacing = 5.0f;
	const float CornerIndicatorExpandedTargetHeight =
		7.0f * (MarginSmall + LineSize) +
		(CornerIndicatorShowSizes ? 3.0f * (MarginSmall + LineSize) : 0.0f) +
		(CornerIndicatorShowCustomColors ? 3.0f * (CornerIndicatorColorPickerSpacing + CornerIndicatorColorPickerLineSize) : 0.0f);
	const float CornerIndicatorExpandedHeight = BC_MODULE_REVEAL(CornerIndicatorEnabled, CornerIndicatorExpandedTargetHeight, Client()->RenderFrameTime());
	const float CornerIndicatorBlockedHintHeight = CornerIndicatorBlocked ? (MarginSmall + LineSize) : 0.0f;
	const float CornerIndicatorBlockHeight = LineSize + MarginSmall + LineSize + CornerIndicatorExpandedHeight + CornerIndicatorBlockedHintHeight;
	CUIRect CornerIndicatorBlock;
	Column.HSplitTop(CornerIndicatorBlockHeight, &CornerIndicatorBlock, &Column);

	CUIRect CornerIndicatorBlockBg = CornerIndicatorBlock;
	CornerIndicatorBlockBg.w += BlockPadding;
	CornerIndicatorBlockBg.h += BlockPadding;
	CornerIndicatorBlockBg.x -= BlockPadding * 0.5f;
	CornerIndicatorBlockBg.y -= BlockPadding * 0.5f;
	BestClientUiTheme::DrawUiBlock(&CornerIndicatorBlockBg, IGraphics::CORNER_ALL, 10.0f);
	BestClientSettingsSearch::DrawBlockHighlight(&g_Config.m_BcCornerIndicator, &CornerIndicatorBlockBg);
	CUIRect CornerIndicatorView = CornerIndicatorBlock;
	CornerIndicatorView.HSplitTop(LineSize, &Label, &CornerIndicatorView);
	CUIRect CornerIndicatorTitleLabel, CornerIndicatorResetButton;
	Label.VSplitRight(LineSize + 8.0f, &CornerIndicatorTitleLabel, &CornerIndicatorResetButton);
	static CButtonContainer s_CornerIndicatorResetButton;
	const bool CornerIndicatorResetClicked = Ui()->DoButton_FontIcon(&s_CornerIndicatorResetButton, FontIcon::ARROW_ROTATE_LEFT, 0, &CornerIndicatorResetButton, BUTTONFLAG_LEFT);
	GameClient()->m_Tooltips.DoToolTip(&s_CornerIndicatorResetButton, &CornerIndicatorResetButton, BcLocalize("Reset to defaults"));
	if(CornerIndicatorResetClicked)
	{
		g_Config.m_BcCornerIndicatorOnlyInTeam = DefaultConfig::BcCornerIndicatorOnlyInTeam;
		g_Config.m_BcCornerIndicatorShowAlive = DefaultConfig::BcCornerIndicatorShowAlive;
		g_Config.m_BcCornerIndicatorShowFrozen = DefaultConfig::BcCornerIndicatorShowFrozen;
		g_Config.m_BcCornerIndicatorIndicateLastTeammate = DefaultConfig::BcCornerIndicatorIndicateLastTeammate;
		g_Config.m_BcCornerIndicatorProximity = DefaultConfig::BcCornerIndicatorProximity;
		g_Config.m_BcCornerIndicatorProximityRange = DefaultConfig::BcCornerIndicatorProximityRange;
		g_Config.m_BcCornerIndicatorProximityMinSize = DefaultConfig::BcCornerIndicatorProximityMinSize;
		g_Config.m_BcCornerIndicatorNinjaFrozen = DefaultConfig::BcCornerIndicatorNinjaFrozen;
		g_Config.m_BcCornerIndicatorSizeAlive = DefaultConfig::BcCornerIndicatorSizeAlive;
		g_Config.m_BcCornerIndicatorSizeFrozen = DefaultConfig::BcCornerIndicatorSizeFrozen;
		g_Config.m_BcCornerIndicatorLastTeammateSize = DefaultConfig::BcCornerIndicatorLastTeammateSize;
		g_Config.m_BcCornerIndicatorCustomColors = DefaultConfig::BcCornerIndicatorCustomColors;
		g_Config.m_BcCornerIndicatorColorAlive = DefaultConfig::BcCornerIndicatorColorAlive;
		g_Config.m_BcCornerIndicatorColorFrozen = DefaultConfig::BcCornerIndicatorColorFrozen;
		g_Config.m_BcCornerIndicatorLastTeammateColor = DefaultConfig::BcCornerIndicatorLastTeammateColor;
	}
	CUIRect CornerIndicatorAuthorBadge;
	static CButtonContainer s_CornerIndicatorAuthorBadge;
	CornerIndicatorTitleLabel.VSplitRight(6.0f, &CornerIndicatorTitleLabel, nullptr);
	BcMenuBadges::DrawAuthor(Graphics(), Ui(), TextRender(), &CornerIndicatorTitleLabel, 4.0f, &CornerIndicatorAuthorBadge);
	Ui()->DoButtonLogic(&s_CornerIndicatorAuthorBadge, 0, &CornerIndicatorAuthorBadge, BUTTONFLAG_NONE);
	GameClient()->m_Tooltips.DoToolTip(&s_CornerIndicatorAuthorBadge, &CornerIndicatorAuthorBadge, "blanc");
	GameClient()->m_Tooltips.SetFadeTime(&s_CornerIndicatorAuthorBadge, 0.0f);
	Ui()->DoLabel(&CornerIndicatorTitleLabel, BcLocalize("Corner indicator"), HeadlineFontSize, TEXTALIGN_ML);
	CornerIndicatorView.HSplitTop(MarginSmall, nullptr, &CornerIndicatorView);

	CornerIndicatorView.HSplitTop(LineSize, &Content, &CornerIndicatorView);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcCornerIndicator, BcLocalize("Enable corner indicator"), &g_Config.m_BcCornerIndicator, &Content, LineSize);

	if(CornerIndicatorExpandedHeight > 0.5f)
	{
		CUIRect Visible = CornerIndicatorView;
		Visible.h = CornerIndicatorExpandedHeight;
		Ui()->ClipEnable(&Visible);

		CornerIndicatorView.HSplitTop(MarginSmall, nullptr, &CornerIndicatorView);
		CornerIndicatorView.HSplitTop(LineSize, &Content, &CornerIndicatorView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcCornerIndicatorOnlyInTeam, BcLocalize("Only in team"), &g_Config.m_BcCornerIndicatorOnlyInTeam, &Content, LineSize);

		CornerIndicatorView.HSplitTop(MarginSmall, nullptr, &CornerIndicatorView);
		CornerIndicatorView.HSplitTop(LineSize, &Content, &CornerIndicatorView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcCornerIndicatorShowAlive, BcLocalize("Show alive teammates"), &g_Config.m_BcCornerIndicatorShowAlive, &Content, LineSize);

		CornerIndicatorView.HSplitTop(MarginSmall, nullptr, &CornerIndicatorView);
		CornerIndicatorView.HSplitTop(LineSize, &Content, &CornerIndicatorView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcCornerIndicatorShowFrozen, BcLocalize("Show frozen teammates"), &g_Config.m_BcCornerIndicatorShowFrozen, &Content, LineSize);

		CornerIndicatorView.HSplitTop(MarginSmall, nullptr, &CornerIndicatorView);
		CornerIndicatorView.HSplitTop(LineSize, &Content, &CornerIndicatorView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcCornerIndicatorIndicateLastTeammate, BcLocalize("Indicate last teammate"), &g_Config.m_BcCornerIndicatorIndicateLastTeammate, &Content, LineSize);

		CornerIndicatorView.HSplitTop(MarginSmall, nullptr, &CornerIndicatorView);
		CornerIndicatorView.HSplitTop(LineSize, &Content, &CornerIndicatorView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcCornerIndicatorProximity, BcLocalize("Scale by distance"), &g_Config.m_BcCornerIndicatorProximity, &Content, LineSize);

		CornerIndicatorView.HSplitTop(MarginSmall, nullptr, &CornerIndicatorView);
		CornerIndicatorView.HSplitTop(LineSize, &Content, &CornerIndicatorView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcCornerIndicatorNinjaFrozen, BcLocalize("Ninja skin when frozen"), &g_Config.m_BcCornerIndicatorNinjaFrozen, &Content, LineSize);

		if(CornerIndicatorShowSizes)
		{
			CornerIndicatorView.HSplitTop(MarginSmall, nullptr, &CornerIndicatorView);
			CornerIndicatorView.HSplitTop(LineSize, &Button, &CornerIndicatorView);
			Ui()->DoScrollbarOption(&g_Config.m_BcCornerIndicatorSizeAlive, &g_Config.m_BcCornerIndicatorSizeAlive, &Button, BcLocalize("Alive size"), 12, 128);

			CornerIndicatorView.HSplitTop(MarginSmall, nullptr, &CornerIndicatorView);
			CornerIndicatorView.HSplitTop(LineSize, &Button, &CornerIndicatorView);
			Ui()->DoScrollbarOption(&g_Config.m_BcCornerIndicatorSizeFrozen, &g_Config.m_BcCornerIndicatorSizeFrozen, &Button, BcLocalize("Frozen size"), 12, 128);

			CornerIndicatorView.HSplitTop(MarginSmall, nullptr, &CornerIndicatorView);
			CornerIndicatorView.HSplitTop(LineSize, &Button, &CornerIndicatorView);
			Ui()->DoScrollbarOption(&g_Config.m_BcCornerIndicatorLastTeammateSize, &g_Config.m_BcCornerIndicatorLastTeammateSize, &Button, BcLocalize("Last teammate size"), 12, 128);
		}

		CornerIndicatorView.HSplitTop(MarginSmall, nullptr, &CornerIndicatorView);
		CornerIndicatorView.HSplitTop(LineSize, &Content, &CornerIndicatorView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcCornerIndicatorCustomColors, BcLocalize("Use custom colors"), &g_Config.m_BcCornerIndicatorCustomColors, &Content, LineSize);

		if(CornerIndicatorShowCustomColors)
		{
			static CButtonContainer s_CornerIndicatorColorAliveButton;
			static CButtonContainer s_CornerIndicatorColorFrozenButton;
			static CButtonContainer s_CornerIndicatorLastTeammateColorButton;
			DoLine_ColorPicker(&s_CornerIndicatorColorAliveButton, CornerIndicatorColorPickerLineSize, 13.0f, CornerIndicatorColorPickerSpacing, &CornerIndicatorView, BcLocalize("Alive color"), &g_Config.m_BcCornerIndicatorColorAlive, color_cast<ColorRGBA>(ColorHSLA(DefaultConfig::BcCornerIndicatorColorAlive, true)), false, nullptr, true);
			DoLine_ColorPicker(&s_CornerIndicatorColorFrozenButton, CornerIndicatorColorPickerLineSize, 13.0f, CornerIndicatorColorPickerSpacing, &CornerIndicatorView, BcLocalize("Frozen color"), &g_Config.m_BcCornerIndicatorColorFrozen, color_cast<ColorRGBA>(ColorHSLA(DefaultConfig::BcCornerIndicatorColorFrozen, true)), false, nullptr, true);
			DoLine_ColorPicker(&s_CornerIndicatorLastTeammateColorButton, CornerIndicatorColorPickerLineSize, 13.0f, CornerIndicatorColorPickerSpacing, &CornerIndicatorView, BcLocalize("Last teammate color"), &g_Config.m_BcCornerIndicatorLastTeammateColor, color_cast<ColorRGBA>(ColorHSLA(DefaultConfig::BcCornerIndicatorLastTeammateColor, true)), false, nullptr, true);
		}

		Ui()->ClipDisable();
	}

	if(CornerIndicatorBlocked)
	{
		CUIRect HintArea, HintRow;
		CUIRect BlockForHint = CornerIndicatorBlock;
		BlockForHint.HSplitBottom(CornerIndicatorBlockedHintHeight, nullptr, &HintArea);
		HintArea.HSplitTop(MarginSmall, nullptr, &HintRow);
		HintRow.h = LineSize;
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
		Ui()->DoLabel(&HintRow, BcLocalize("Looks like you're on a server where this feature is forbidden"), 14.0f, TEXTALIGN_ML);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
	}

	Column.HSplitTop(MarginBetweenViews, nullptr, &Column);

	RenderSettingsBestClientChatMediaBlock(Column);

	Column.HSplitTop(MarginBetweenViews, nullptr, &Column);

	const bool ChatBubblesExpanded = g_Config.m_BcChatBubbles != 0;
	const bool ChatBubblesShowCustomColors = ChatBubblesExpanded && g_Config.m_BcChatBubbleCustomColors != 0;
	const float ChatBubbleColorPickerLineSize = 25.0f;
	const float ChatBubbleColorPickerSpacing = 5.0f;
	const float ChatBubblesExpandedTargetHeight =
		9.0f * (MarginSmall + LineSize) +
		(ChatBubblesShowCustomColors ? 3.0f * (ChatBubbleColorPickerSpacing + ChatBubbleColorPickerLineSize) : 0.0f);
	const float ChatBubblesExpandedHeight = BC_MODULE_REVEAL(ChatBubblesExpanded, ChatBubblesExpandedTargetHeight, Client()->RenderFrameTime());
	const float ChatBubblesBlockHeight = LineSize + MarginSmall + LineSize + ChatBubblesExpandedHeight;

	CUIRect ChatBubblesBlock;
	Column.HSplitTop(ChatBubblesBlockHeight, &ChatBubblesBlock, &Column);

	CUIRect ChatBubblesBlockBg = ChatBubblesBlock;
	ChatBubblesBlockBg.w += BlockPadding;
	ChatBubblesBlockBg.h += BlockPadding;
	ChatBubblesBlockBg.x -= BlockPadding * 0.5f;
	ChatBubblesBlockBg.y -= BlockPadding * 0.5f;
	BestClientUiTheme::DrawUiBlock(&ChatBubblesBlockBg, IGraphics::CORNER_ALL, 10.0f);
	BestClientSettingsSearch::DrawBlockHighlight(&g_Config.m_BcChatBubbles, &ChatBubblesBlockBg);

	MainView = ChatBubblesBlock;

	MainView.HSplitTop(LineSize, &Label, &MainView);
	CUIRect ChatBubblesTitleLabel, ChatBubblesResetButton;
	Label.VSplitRight(LineSize + 8.0f, &ChatBubblesTitleLabel, &ChatBubblesResetButton);
	static CButtonContainer s_ChatBubblesResetButton;
	const bool ChatBubblesResetClicked = Ui()->DoButton_FontIcon(&s_ChatBubblesResetButton, FontIcon::ARROW_ROTATE_LEFT, 0, &ChatBubblesResetButton, BUTTONFLAG_LEFT);
	GameClient()->m_Tooltips.DoToolTip(&s_ChatBubblesResetButton, &ChatBubblesResetButton, BcLocalize("Reset to defaults"));
	if(ChatBubblesResetClicked)
	{
		g_Config.m_BcChatBubblesSelf = DefaultConfig::BcChatBubblesSelf;
		g_Config.m_BcChatBubblesDemo = DefaultConfig::BcChatBubblesDemo;
		g_Config.m_BcChatBubbleSize = DefaultConfig::BcChatBubbleSize;
		g_Config.m_BcChatBubbleShowTime = DefaultConfig::BcChatBubbleShowTime;
		g_Config.m_BcChatBubbleFadeOut = DefaultConfig::BcChatBubbleFadeOut;
		g_Config.m_BcChatBubbleFadeIn = DefaultConfig::BcChatBubbleFadeIn;
		g_Config.m_BcChatBubbleAnimation = DefaultConfig::BcChatBubbleAnimation;
		g_Config.m_BcChatBubbleCustomColors = DefaultConfig::BcChatBubbleCustomColors;
		g_Config.m_BcChatBubbleBgColor = DefaultConfig::BcChatBubbleBgColor;
		g_Config.m_BcChatBubbleTextColor = DefaultConfig::BcChatBubbleTextColor;
		g_Config.m_BcChatBubbleOutlineColor = DefaultConfig::BcChatBubbleOutlineColor;
		g_Config.m_BcChatBubbleRounding = DefaultConfig::BcChatBubbleRounding;
	}
	ChatBubblesTitleLabel.VSplitRight(MarginSmall, &ChatBubblesTitleLabel, nullptr);
	BcMenuBadges::DrawEClient(Graphics(), Ui(), TextRender(), &ChatBubblesTitleLabel, MarginSmall);
	Ui()->DoLabel(&ChatBubblesTitleLabel, BcLocalize("Chat Bubbles"), HeadlineFontSize, TEXTALIGN_ML);
	MainView.HSplitTop(MarginSmall, nullptr, &MainView);

	MainView.HSplitTop(LineSize, &Content, &MainView);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcChatBubbles, BcLocalize("Show chat bubbles above players"), &g_Config.m_BcChatBubbles, &Content, LineSize);

	if(ChatBubblesExpandedHeight > 0.5f)
	{
		CUIRect Visible = MainView;
		Visible.h = ChatBubblesExpandedHeight;
		Ui()->ClipEnable(&Visible);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Content, &MainView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcChatBubblesSelf, BcLocalize("Show chat bubbles above you"), &g_Config.m_BcChatBubblesSelf, &Content, LineSize);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Content, &MainView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcChatBubblesDemo, BcLocalize("Show chat bubbles in demo"), &g_Config.m_BcChatBubblesDemo, &Content, LineSize);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_BcChatBubbleSize, &g_Config.m_BcChatBubbleSize, &Button, BcLocalize("Chat bubble size"), 20, 30);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_BcChatBubbleShowTime, &g_Config.m_BcChatBubbleShowTime, &Button, BcLocalize("Show for"), 100, 1000);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_BcChatBubbleFadeIn, &g_Config.m_BcChatBubbleFadeIn, &Button, BcLocalize("Fade in"), 15, 100);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_BcChatBubbleFadeOut, &g_Config.m_BcChatBubbleFadeOut, &Button, BcLocalize("Fade out"), 15, 100);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Content, &MainView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcChatBubbleAnimation, BcLocalize("Stack animation"), &g_Config.m_BcChatBubbleAnimation, &Content, LineSize);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_BcChatBubbleRounding, &g_Config.m_BcChatBubbleRounding, &Button, BcLocalize("Corner rounding"), 0, 30);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Content, &MainView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcChatBubbleCustomColors, BcLocalize("Custom colors"), &g_Config.m_BcChatBubbleCustomColors, &Content, LineSize);

		if(ChatBubblesShowCustomColors)
		{
			static CButtonContainer s_ChatBubbleBgColorButton;
			DoLine_ColorPicker(&s_ChatBubbleBgColorButton, ChatBubbleColorPickerLineSize, 13.0f, ChatBubbleColorPickerSpacing, &MainView, BcLocalize("Background"), &g_Config.m_BcChatBubbleBgColor, color_cast<ColorRGBA>(ColorHSLA(DefaultConfig::BcChatBubbleBgColor, true)), false, nullptr, true);

			static CButtonContainer s_ChatBubbleTextColorButton;
			DoLine_ColorPicker(&s_ChatBubbleTextColorButton, ChatBubbleColorPickerLineSize, 13.0f, ChatBubbleColorPickerSpacing, &MainView, BcLocalize("Text"), &g_Config.m_BcChatBubbleTextColor, color_cast<ColorRGBA>(ColorHSLA(DefaultConfig::BcChatBubbleTextColor, true)), false, nullptr, true);

			static CButtonContainer s_ChatBubbleOutlineColorButton;
			DoLine_ColorPicker(&s_ChatBubbleOutlineColorButton, ChatBubbleColorPickerLineSize, 13.0f, ChatBubbleColorPickerSpacing, &MainView, BcLocalize("Outline"), &g_Config.m_BcChatBubbleOutlineColor, color_cast<ColorRGBA>(ColorHSLA(DefaultConfig::BcChatBubbleOutlineColor, true)), false, nullptr, true);
		}

		Ui()->ClipDisable();
	}

	Column.HSplitTop(MarginBetweenViews, nullptr, &Column);
	const bool GradientNicknames = g_Config.m_BcNameplateGradient != 0;
	const bool GradientClans = g_Config.m_BcNameplateGradientClan != 0;
	const bool GradientSkin = g_Config.m_BcNameplateGradientSkin != 0;
	const bool GradientEverything = g_Config.m_BcNameplateGradientEverything != 0;
	const bool GradientExpanded = GradientNicknames || GradientClans || GradientSkin || GradientEverything;
	const bool GradientShowCustomColors = GradientExpanded && g_Config.m_BcNameplateGradientMode == BC_GRADIENT_MODE_CUSTOM;
	const float GradientColorPickerLineSize = 25.0f;
	const float GradientColorPickerSpacing = 5.0f;
	const int GradientCustomColorCount = std::clamp(g_Config.m_BcNameplateGradientColorCount, 2, 4);
	const float GradientHeaderHeight = LineSize + MarginSmall + 4.0f * LineSize;
	const float GradientTargetRadioHeight = 2.0f + LineSize;
	const float GradientExpandedTargetHeight = MarginSmall + LineSize + MarginSmall + LineSize + LineSize + MarginSmall + GradientTargetRadioHeight + (GradientShowCustomColors ? MarginSmall + LineSize + LineSize + MarginSmall + GradientCustomColorCount * (GradientColorPickerLineSize + GradientColorPickerSpacing) : 0.0f);
	const float GradientExpandedHeight = BC_MODULE_REVEAL(GradientExpanded, GradientExpandedTargetHeight, Client()->RenderFrameTime());
	const float GradientBlockHeight = GradientHeaderHeight + GradientExpandedHeight;

	CUIRect GradientBlock;
	Column.HSplitTop(GradientBlockHeight, &GradientBlock, &Column);

	CUIRect GradientBlockBg = GradientBlock;
	GradientBlockBg.w += BlockPadding;
	GradientBlockBg.h += BlockPadding;
	GradientBlockBg.x -= BlockPadding * 0.5f;
	GradientBlockBg.y -= BlockPadding * 0.5f;
	BestClientUiTheme::DrawUiBlock(&GradientBlockBg, IGraphics::CORNER_ALL, 10.0f);
	BestClientSettingsSearch::DrawBlockHighlight(&g_Config.m_BcNameplateGradient, &GradientBlockBg);

	MainView = GradientBlock;

	MainView.HSplitTop(LineSize, &Label, &MainView);
	CUIRect GradientTitleLabel, GradientResetButton;
	Label.VSplitRight(LineSize + 8.0f, &GradientTitleLabel, &GradientResetButton);
	static CButtonContainer s_GradientResetButton;
	const bool GradientResetClicked = Ui()->DoButton_FontIcon(&s_GradientResetButton, FontIcon::ARROW_ROTATE_LEFT, 0, &GradientResetButton, BUTTONFLAG_LEFT);
	GameClient()->m_Tooltips.DoToolTip(&s_GradientResetButton, &GradientResetButton, BcLocalize("Reset to defaults"));
	if(GradientResetClicked)
	{
		g_Config.m_BcNameplateGradient = DefaultConfig::BcNameplateGradient;
		g_Config.m_BcNameplateGradientClan = DefaultConfig::BcNameplateGradientClan;
		g_Config.m_BcNameplateGradientMode = DefaultConfig::BcNameplateGradientMode;
		g_Config.m_BcNameplateGradientColorCount = DefaultConfig::BcNameplateGradientColorCount;
		g_Config.m_BcNameplateGradientColor1 = DefaultConfig::BcNameplateGradientColor1;
		g_Config.m_BcNameplateGradientColor2 = DefaultConfig::BcNameplateGradientColor2;
		g_Config.m_BcNameplateGradientColor3 = DefaultConfig::BcNameplateGradientColor3;
		g_Config.m_BcNameplateGradientColor4 = DefaultConfig::BcNameplateGradientColor4;
		g_Config.m_BcNameplateGradientSkin = DefaultConfig::BcNameplateGradientSkin;
		g_Config.m_BcNameplateGradientEverything = DefaultConfig::BcNameplateGradientEverything;
		g_Config.m_BcNameplateGradientAnimateSpeed = DefaultConfig::BcNameplateGradientAnimateSpeed;
		g_Config.m_BcNameplateGradientTarget = DefaultConfig::BcNameplateGradientTarget;
	}
	GradientTitleLabel.VSplitRight(MarginSmall, &GradientTitleLabel, nullptr);
	Ui()->DoLabel(&GradientTitleLabel, BcLocalize("Gradient"), HeadlineFontSize, TEXTALIGN_ML);

	MainView.HSplitTop(MarginSmall, nullptr, &MainView);

	MainView.HSplitTop(LineSize, &Content, &MainView);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcNameplateGradient, BcLocalize("Gradient nicknames"), &g_Config.m_BcNameplateGradient, &Content, LineSize);
	MainView.HSplitTop(LineSize, &Content, &MainView);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcNameplateGradientClan, BcLocalize("Gradient clans"), &g_Config.m_BcNameplateGradientClan, &Content, LineSize);
	MainView.HSplitTop(LineSize, &Content, &MainView);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcNameplateGradientSkin, BcLocalize("Gradient skin"), &g_Config.m_BcNameplateGradientSkin, &Content, LineSize);
	MainView.HSplitTop(LineSize, &Content, &MainView);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcNameplateGradientEverything, BcLocalize("Gradient everything"), &g_Config.m_BcNameplateGradientEverything, &Content, LineSize);

	if(GradientExpandedHeight > 0.5f)
	{
		CUIRect Visible = MainView;
		Visible.h = GradientExpandedHeight;
		Ui()->ClipEnable(&Visible);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_BcNameplateGradientAnimateSpeed, &g_Config.m_BcNameplateGradientAnimateSpeed, &Button, BcLocalize("Animate speed"), 1, 100, &CUi::ms_LinearScrollbarScale, 0, "%");

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		CUIRect GradientModeLabel, GradientModeRow;
		MainView.HSplitTop(LineSize, &GradientModeLabel, &MainView);
		Ui()->DoLabel(&GradientModeLabel, BcLocalize("Mode"), 14.0f, TEXTALIGN_ML);

		MainView.HSplitTop(LineSize, &GradientModeRow, &MainView);
		{
			static CButtonContainer s_GradientModeSkin, s_GradientModeCustom, s_GradientModeRainbow;
			CUIRect SkinButton, CustomButton, RainbowButton;
			GradientModeRow.VSplitLeft(GradientModeRow.w / 3.0f, &SkinButton, &GradientModeRow);
			GradientModeRow.VSplitLeft(GradientModeRow.w / 2.0f, &CustomButton, &RainbowButton);
			g_Config.m_BcNameplateGradientMode = std::clamp(g_Config.m_BcNameplateGradientMode, 0, 2);
			if(DoButton_Menu(&s_GradientModeSkin, BcLocalize("Skin"), g_Config.m_BcNameplateGradientMode == BC_GRADIENT_MODE_SKIN, &SkinButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_L))
				g_Config.m_BcNameplateGradientMode = BC_GRADIENT_MODE_SKIN;
			if(DoButton_Menu(&s_GradientModeCustom, BcLocalize("Custom"), g_Config.m_BcNameplateGradientMode == BC_GRADIENT_MODE_CUSTOM, &CustomButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_NONE))
				g_Config.m_BcNameplateGradientMode = BC_GRADIENT_MODE_CUSTOM;
			if(DoButton_Menu(&s_GradientModeRainbow, BcLocalize("Rainbow"), g_Config.m_BcNameplateGradientMode == BC_GRADIENT_MODE_RAINBOW, &RainbowButton, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_R))
				g_Config.m_BcNameplateGradientMode = BC_GRADIENT_MODE_RAINBOW;
		}

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		static std::vector<CButtonContainer> s_vGradientTargetButtons = {{}, {}, {}};
		DoLine_RadioMenu(MainView, Localize("Apply to", "Gradient"),
			s_vGradientTargetButtons,
			{Localize("Own", "Gradient"), Localize("Others", "Gradient"), Localize("All", "Gradient")},
			{BC_GRADIENT_TARGET_OWN, BC_GRADIENT_TARGET_OTHERS, BC_GRADIENT_TARGET_ALL},
			g_Config.m_BcNameplateGradientTarget);

		if(GradientShowCustomColors)
		{
			MainView.HSplitTop(MarginSmall, nullptr, &MainView);
			CUIRect GradientColorCountLabel, GradientColorCountRow;
			MainView.HSplitTop(LineSize, &GradientColorCountLabel, &MainView);
			Ui()->DoLabel(&GradientColorCountLabel, BcLocalize("Color count"), 14.0f, TEXTALIGN_ML);

			MainView.HSplitTop(LineSize, &GradientColorCountRow, &MainView);
			{
				static CButtonContainer s_GradientColorCount2, s_GradientColorCount3, s_GradientColorCount4;
				CUIRect Count2Button, Count3Button, Count4Button;
				GradientColorCountRow.VSplitLeft(GradientColorCountRow.w / 3.0f, &Count2Button, &GradientColorCountRow);
				GradientColorCountRow.VSplitLeft(GradientColorCountRow.w / 2.0f, &Count3Button, &Count4Button);
				g_Config.m_BcNameplateGradientColorCount = std::clamp(g_Config.m_BcNameplateGradientColorCount, 2, 4);
				if(DoButton_Menu(&s_GradientColorCount2, "2", g_Config.m_BcNameplateGradientColorCount == 2, &Count2Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_L))
					g_Config.m_BcNameplateGradientColorCount = 2;
				if(DoButton_Menu(&s_GradientColorCount3, "3", g_Config.m_BcNameplateGradientColorCount == 3, &Count3Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_NONE))
					g_Config.m_BcNameplateGradientColorCount = 3;
				if(DoButton_Menu(&s_GradientColorCount4, "4", g_Config.m_BcNameplateGradientColorCount == 4, &Count4Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_R))
					g_Config.m_BcNameplateGradientColorCount = 4;
			}

			static CButtonContainer s_GradientColor1Button;
			static CButtonContainer s_GradientColor2Button;
			static CButtonContainer s_GradientColor3Button;
			static CButtonContainer s_GradientColor4Button;
			MainView.HSplitTop(MarginSmall, nullptr, &MainView);
			DoLine_ColorPicker(&s_GradientColor1Button, GradientColorPickerLineSize, 13.0f, GradientColorPickerSpacing, &MainView, BcLocalize("Color 1"), &g_Config.m_BcNameplateGradientColor1, color_cast<ColorRGBA>(ColorHSLA(DefaultConfig::BcNameplateGradientColor1, true)), false);
			DoLine_ColorPicker(&s_GradientColor2Button, GradientColorPickerLineSize, 13.0f, GradientColorPickerSpacing, &MainView, BcLocalize("Color 2"), &g_Config.m_BcNameplateGradientColor2, color_cast<ColorRGBA>(ColorHSLA(DefaultConfig::BcNameplateGradientColor2, true)), false);
			if(GradientCustomColorCount >= 3)
				DoLine_ColorPicker(&s_GradientColor3Button, GradientColorPickerLineSize, 13.0f, GradientColorPickerSpacing, &MainView, BcLocalize("Color 3"), &g_Config.m_BcNameplateGradientColor3, color_cast<ColorRGBA>(ColorHSLA(DefaultConfig::BcNameplateGradientColor3, true)), false);
			if(GradientCustomColorCount >= 4)
				DoLine_ColorPicker(&s_GradientColor4Button, GradientColorPickerLineSize, 13.0f, GradientColorPickerSpacing, &MainView, BcLocalize("Color 4"), &g_Config.m_BcNameplateGradientColor4, color_cast<ColorRGBA>(ColorHSLA(DefaultConfig::BcNameplateGradientColor4, true)), false);
		}

		Ui()->ClipDisable();
	}

	Column.HSplitTop(MarginBetweenViews, nullptr, &Column);

	const bool FlyingNamePlatesExpanded = g_Config.m_BcFlyingNamePlates != 0;
	const float FlyingNamePlatesHeaderHeight = LineSize + MarginSmall + LineSize;
	const float FlyingNamePlatesExpandedTargetHeight = MarginSmall + LineSize + MarginSmall + LineSize + MarginSmall + LineSize + MarginSmall + LineSize;
	const float FlyingNamePlatesExpandedHeight = BC_MODULE_REVEAL(FlyingNamePlatesExpanded, FlyingNamePlatesExpandedTargetHeight, Client()->RenderFrameTime());
	const float FlyingNamePlatesBlockHeight = FlyingNamePlatesHeaderHeight + FlyingNamePlatesExpandedHeight;

	CUIRect FlyingNamePlatesBlock;
	Column.HSplitTop(FlyingNamePlatesBlockHeight, &FlyingNamePlatesBlock, &Column);

	CUIRect FlyingNamePlatesBlockBg = FlyingNamePlatesBlock;
	FlyingNamePlatesBlockBg.w += BlockPadding;
	FlyingNamePlatesBlockBg.h += BlockPadding;
	FlyingNamePlatesBlockBg.x -= BlockPadding * 0.5f;
	FlyingNamePlatesBlockBg.y -= BlockPadding * 0.5f;
	BestClientUiTheme::DrawUiBlock(&FlyingNamePlatesBlockBg, IGraphics::CORNER_ALL, 10.0f);
	BestClientSettingsSearch::DrawBlockHighlight(&g_Config.m_BcFlyingNamePlates, &FlyingNamePlatesBlockBg);

	MainView = FlyingNamePlatesBlock;

	MainView.HSplitTop(LineSize, &Label, &MainView);
	CUIRect FlyingNamePlatesTitleLabel, FlyingNamePlatesResetButton;
	Label.VSplitRight(LineSize + 8.0f, &FlyingNamePlatesTitleLabel, &FlyingNamePlatesResetButton);
	static CButtonContainer s_FlyingNamePlatesResetButton;
	const bool FlyingNamePlatesResetClicked = Ui()->DoButton_FontIcon(&s_FlyingNamePlatesResetButton, FontIcon::ARROW_ROTATE_LEFT, 0, &FlyingNamePlatesResetButton, BUTTONFLAG_LEFT);
	GameClient()->m_Tooltips.DoToolTip(&s_FlyingNamePlatesResetButton, &FlyingNamePlatesResetButton, BcLocalize("Reset to defaults"));
	if(FlyingNamePlatesResetClicked)
	{
		g_Config.m_BcFlyingNamePlatesHideLine = DefaultConfig::BcFlyingNamePlatesHideLine;
		g_Config.m_BcFlyingNamePlatesLift = DefaultConfig::BcFlyingNamePlatesLift;
		g_Config.m_BcFlyingNamePlatesDrag = DefaultConfig::BcFlyingNamePlatesDrag;
		g_Config.m_BcFlyingNamePlatesFollow = DefaultConfig::BcFlyingNamePlatesFollow;
	}
	FlyingNamePlatesTitleLabel.VSplitRight(MarginSmall, &FlyingNamePlatesTitleLabel, nullptr);
	Ui()->DoLabel(&FlyingNamePlatesTitleLabel, BcLocalize("Flying Name Plates"), HeadlineFontSize, TEXTALIGN_ML);

	MainView.HSplitTop(MarginSmall, nullptr, &MainView);
	MainView.HSplitTop(LineSize, &Content, &MainView);
	DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcFlyingNamePlates, BcLocalize("Enable flying name plates"), &g_Config.m_BcFlyingNamePlates, &Content, LineSize);

	if(FlyingNamePlatesExpandedHeight > 0.5f)
	{
		CUIRect Visible = MainView;
		Visible.h = FlyingNamePlatesExpandedHeight;
		Ui()->ClipEnable(&Visible);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Content, &MainView);
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_BcFlyingNamePlatesHideLine, BcLocalize("Hide line"), &g_Config.m_BcFlyingNamePlatesHideLine, &Content, LineSize);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_BcFlyingNamePlatesLift, &g_Config.m_BcFlyingNamePlatesLift, &Button, BcLocalize("Lift above player"), 0, 120);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_BcFlyingNamePlatesDrag, &g_Config.m_BcFlyingNamePlatesDrag, &Button, BcLocalize("Movement drag"), 0, 200);

		MainView.HSplitTop(MarginSmall, nullptr, &MainView);
		MainView.HSplitTop(LineSize, &Button, &MainView);
		Ui()->DoScrollbarOption(&g_Config.m_BcFlyingNamePlatesFollow, &g_Config.m_BcFlyingNamePlatesFollow, &Button, BcLocalize("Follow speed"), 1, 100);

		Ui()->ClipDisable();
	}

	const float LeftColumnEndY = Column.y;
	CUIRect VisualsScrollContentRect;
	VisualsScrollContentRect.x = VisualsScrollContentX;
	VisualsScrollContentRect.y = std::max(LeftColumnEndY, RightView.y) + MarginSmall * 2.0f;
	VisualsScrollContentRect.w = VisualsScrollContentW;
	VisualsScrollContentRect.h = 0.0f;
	s_VisualsScrollRegion.AddRect(VisualsScrollContentRect);
	s_VisualsScrollRegion.End();
}
