from CvPythonExtensions import *
import CvUtil
import ScreenInput
import CvScreenEnums
gc = CyGlobalContext()

class CvPediaFeature:
	def __init__(self, main):
		self.iFeature = -1
		self.top = main

	def interfaceScreen(self, iFeature):
		self.iFeature = iFeature
		self.top.deleteAllWidgets()		
		screen = self.top.getScreen()
		if not screen.isActive():
			self.top.setPediaCommonWidgets()

		self.H_ICON = 150
		self.W_MAIN_PANE = (self.top.W_ITEMS_PANE - self.top.W_BORDER)/2
		self.H_MAIN_PANE = 210
		self.X_ICON = self.top.X_ITEMS_PANE + 30
		self.Y_ICON = (self.H_MAIN_PANE - self.H_ICON)/2 + self.top.Y_ITEMS_PANE
		
		self.X_STATS_PANE = self.X_ICON + self.H_ICON
		self.Y_STATS_PANE = self.Y_ICON + self.H_ICON /4
		self.W_STATS_PANE = self.W_MAIN_PANE - self.H_ICON - self.top.W_BORDER * 2
		self.H_STATS_PANE = self.H_MAIN_PANE - self.Y_STATS_PANE + self.top.Y_ITEMS_PANE
		
		self.X_SPECIAL = self.top.X_ITEMS_PANE + self.W_MAIN_PANE + self.top.W_BORDER
		self.X_ANIMATION = self.X_SPECIAL
		self.Y_ANIMATION = self.top.Y_ITEMS_PANE + 8
		self.H_ANIMATION = self.H_MAIN_PANE - 10
		self.Y_SPECIAL = self.top.Y_ITEMS_PANE + self.H_MAIN_PANE + 10
		self.H_SPECIAL = 110
		self.Y_IMPROVEMENT = self.Y_SPECIAL + self.H_SPECIAL + 10
		self.X_TERRAIN = self.top.X_ITEMS_PANE + self.W_MAIN_PANE + self.top.W_BORDER
		self.H_TERRAIN = 110
		self.Y_BONUS = self.Y_IMPROVEMENT + self.H_TERRAIN + 10
		self.Y_HISTORY_PANE = self.Y_BONUS + self.H_TERRAIN + 10
		self.H_HISTORY_PANE = screen.getYResolution() - self.Y_HISTORY_PANE - self.top.Y_ITEMS_PANE

		szHeader = gc.getFeatureInfo(self.iFeature).getDescription().upper()
		szHeader = u"<font=4b>" + self.top.color4 + CyTranslator().getText(self.top.sFeatureIcon, ()) + szHeader + " " + CyTranslator().getText(self.top.sFeatureIcon, ()) + "</color></font>"
		screen.setLabel(self.top.getNextWidgetName(), "Background", szHeader, CvUtil.FONT_CENTER_JUSTIFY, screen.getXResolution()/2, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		
		screen.setText(self.top.getNextWidgetName(), "Background", self.top.MENU_TEXT, CvUtil.FONT_CENTER_JUSTIFY, screen.getXResolution()/2, screen.getYResolution() - 42, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_MAIN, self.top.PLATYPEDIA_FEATURE, -1)
		self.top.addPediaPanel(screen,  self.top.getNextWidgetName(), "", "", False, False, self.top.X_ITEMS_PANE, self.top.Y_ITEMS_PANE, self.W_MAIN_PANE, self.H_MAIN_PANE, PanelStyles.PANEL_STYLE_MAIN_BLACK50)
		self.top.addPediaPanel(screen, self.top.getNextWidgetName(), "", "", false, false, self.X_ICON, self.Y_ICON, self.H_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getFeatureInfo(self.iFeature).getButton(), self.X_ICON + self.H_ICON/2 - 64/2, self.Y_ICON + self.H_ICON/2 - 64/2, 64, 64, WidgetTypes.WIDGET_GENERAL, -1, -1 )
		self.placeAnimation(screen)

		self.placeStats()
		self.placeSpecial()
		self.placeTerrain()
		self.placeBonus()
		self.placeImprovements()
		self.placeHistory()
		self.placeLinks(self.top.iLastScreen == CvScreenEnums.PEDIA_FEATURE and screen.isActive())
		self.top.iLastScreen = CvScreenEnums.PEDIA_FEATURE

	def placeHistory(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.top.addPediaPanel(screen, panelName, CyTranslator().getText("TXT_KEY_CIVILOPEDIA_HISTORY", ()), "", True, True, self.top.X_ITEMS_PANE, self.Y_HISTORY_PANE, self.top.W_ITEMS_PANE, self.H_HISTORY_PANE, PanelStyles.PANEL_STYLE_MAIN_BLACK50 )
		szText = ""
		sStrategy = gc.getFeatureInfo(self.iFeature).getStrategy()
		if len(sStrategy) and sStrategy.find("TXT_KEY") == -1:
			szText += CyTranslator().getText("TXT_KEY_CIVILOPEDIA_STRATEGY", ())
			szText += sStrategy + "\n\n"
			szText += CyTranslator().getText("TXT_KEY_CIVILOPEDIA_BACKGROUND", ())
		sPedia = gc.getFeatureInfo(self.iFeature).getCivilopedia()
		if sPedia.find("TXT_KEY") == -1:
			szText += sPedia
		self.top.addPediaBodyText(screen, self.top.getNextWidgetName(), szText, self.top.X_ITEMS_PANE + 10, self.Y_HISTORY_PANE + 30, self.top.W_ITEMS_PANE - 20, self.H_HISTORY_PANE - 30, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeAnimation(self, screen):
		iAnimationInset = self.top.addPediaAnimationFrame(screen, self.top.getNextWidgetName(), self.X_ANIMATION, self.Y_ANIMATION, self.W_MAIN_PANE, self.H_ANIMATION)
		pPlot = self.getFeaturePreviewPlot()
		if pPlot:
			screen.addPlotGraphicGFC(self.top.getNextWidgetName(), self.X_ANIMATION + iAnimationInset, self.Y_ANIMATION + iAnimationInset, self.W_MAIN_PANE - 2 * iAnimationInset, self.H_ANIMATION - 2 * iAnimationInset, pPlot, 300, False, WidgetTypes.WIDGET_GENERAL, -1, -1)
		else:
			iIconSize = min(96, self.H_ANIMATION - 2 * iAnimationInset)
			screen.addDDSGFC(self.top.getNextWidgetName(), gc.getFeatureInfo(self.iFeature).getButton(), self.X_ANIMATION + (self.W_MAIN_PANE - iIconSize) / 2, self.Y_ANIMATION + (self.H_ANIMATION - iIconSize) / 2, iIconSize, iIconSize, WidgetTypes.WIDGET_GENERAL, -1, -1)

	def getFeaturePreviewPlot(self):
		if not CyGame().isFinalInitialized():
			return None
		iActiveTeam = CyGame().getActiveTeam()
		if iActiveTeam < 0:
			return None
		Map = CyMap()
		for iPlot in xrange(Map.numPlots()):
			pPlot = Map.plotByIndex(iPlot)
			if pPlot.getFeatureType() == self.iFeature and pPlot.isRevealed(iActiveTeam, False):
				return pPlot
		return None

	def placeStats(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		screen.addListBoxGFC(panelName, "", self.X_STATS_PANE, self.Y_STATS_PANE, self.W_STATS_PANE, self.H_STATS_PANE, TableStyles.TABLE_STYLE_EMPTY)
		screen.enableSelect(panelName, False)
		sText = ""
		for k in range(YieldTypes.NUM_YIELD_TYPES):
			iYieldChange = gc.getFeatureInfo(self.iFeature).getYieldChange(k)
			if iYieldChange != 0:
				sText += u"%+d%c" % (iYieldChange, gc.getYieldInfo(k).getChar())
		if len(sText):
			self.top.appendPediaListText(screen, panelName, "<font=5>" + CyTranslator().getText("TXT_KEY_PEDIA_YIELDS", ()) + ": </font>", WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
			self.top.appendPediaListText(screen, panelName, "<font=5>" + sText + "</font>", WidgetTypes.WIDGET_GENERAL, 0, 0, CvUtil.FONT_LEFT_JUSTIFY)
		
	def placeSpecial(self):
		screen = self.top.getScreen()
		self.top.addPediaPanel(screen, self.top.getNextWidgetName(), CyTranslator().getText("TXT_KEY_PEDIA_SPECIAL_ABILITIES", ()), "", true, false, self.top.X_ITEMS_PANE, self.Y_SPECIAL, self.top.W_ITEMS_PANE, self.H_SPECIAL, PanelStyles.PANEL_STYLE_MAIN_BLACK50 )
		szSpecialText = CyGameTextMgr().getFeatureHelp(self.iFeature, True)[1:]
		self.top.addPediaBodyText(screen, self.top.getNextWidgetName(), szSpecialText, self.top.X_ITEMS_PANE + 5, self.Y_SPECIAL + 30, self.top.W_ITEMS_PANE - 10, self.H_SPECIAL - 30, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeTerrain(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.top.addPediaPanel(screen,  panelName, CyTranslator().getText("TXT_KEY_CONCEPT_TERRAIN", ()), "", false, true, self.X_TERRAIN, self.Y_IMPROVEMENT, self.W_MAIN_PANE, self.H_TERRAIN, PanelStyles.PANEL_STYLE_MAIN_BLACK50 )
		for iTerrain in xrange(gc.getNumTerrainInfos()):
			FeatureInfo = gc.getFeatureInfo(self.iFeature)
			if FeatureInfo.isTerrain(iTerrain):
				TerrainInfo = gc.getTerrainInfo(iTerrain)
				screen.attachImageButton( panelName, "", TerrainInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_TERRAIN, iTerrain, 1, False )

	def placeBonus(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.top.addPediaPanel(screen,  panelName, CyTranslator().getText("TXT_KEY_PEDIA_CATEGORY_BONUS", ()), "", false, true, self.top.X_ITEMS_PANE, self.Y_BONUS, self.top.W_ITEMS_PANE, self.H_TERRAIN, PanelStyles.PANEL_STYLE_MAIN_BLACK50 )
		for iBonus in xrange(gc.getNumBonusInfos()):
			BonusInfo = gc.getBonusInfo(iBonus)
			if BonusInfo.isFeature(self.iFeature):
				screen.attachImageButton( panelName, "", BonusInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BONUS, iBonus, 1, False )

	def placeImprovements(self):
		screen = self.top.getScreen()
		panelName = self.top.getNextWidgetName()
		self.top.addPediaPanel(screen,  panelName, CyTranslator().getText("TXT_KEY_PEDIA_CATEGORY_IMPROVEMENT", ()), "", false, true, self.top.X_ITEMS_PANE, self.Y_IMPROVEMENT, self.W_MAIN_PANE, self.H_TERRAIN, PanelStyles.PANEL_STYLE_MAIN_BLACK50 )
		for iImprovement in xrange(gc.getNumImprovementInfos()):
			ImprovementInfo = gc.getImprovementInfo(iImprovement)
			if ImprovementInfo.getFeatureMakesValid(self.iFeature):
				screen.attachImageButton( panelName, "", ImprovementInfo.getButton(), GenericButtonSizes.BUTTON_SIZE_CUSTOM, WidgetTypes.WIDGET_PEDIA_JUMP_TO_IMPROVEMENT, iImprovement, 1, False )

	def placeLinks(self, bRedraw):
		screen = self.top.getScreen()
		if bRedraw:
			screen.show("PlatyTable")
			return
		screen.addTableControlGFC("PlatyTable", 1, self.top.X_PANEL, 55, self.top.W_PANEL, screen.getYResolution() - 110, False, False, 24, 24, TableStyles.TABLE_STYLE_STANDARD);
		screen.enableSelect("PlatyTable", True)
		screen.setTableColumnHeader("PlatyTable", 0, "", self.top.W_PANEL)
		listSorted = self.top.sortFeatures(0)
		self.top.placePediaLinks(listSorted, CyTranslator().getText(self.top.sFeatureIcon, ()), self.iFeature, WidgetTypes.WIDGET_PEDIA_JUMP_TO_FEATURE, -1)

	def handleInput (self, inputClass):
		return 0
