from CvPythonExtensions import *
import CvUtil
import ScreenInput
import CvScreenEnums
gc = CyGlobalContext()

class CvPediaSpecialist:
	def __init__(self, main):
		self.iSpecialist = -1
		self.top = main
		self.iSize = 48

	def interfaceScreen(self, iSpecialist):
		self.iSpecialist = iSpecialist
		self.top.deleteAllWidgets()		
		screen = self.top.getScreen()
		if not screen.isActive():
			self.top.setPediaCommonWidgets()

		self.H_ICON = 150
		# Keep the icon/Great Person card compact and give the primary effects
		# panel most of the top row instead of leaving a wide empty card.
		self.W_MAIN_PANE = min(480, (self.top.W_ITEMS_PANE - self.top.W_BORDER)/2)
		self.H_MAIN_PANE = 210
		self.X_ICON = self.top.X_ITEMS_PANE + 30
		self.Y_ICON = (self.H_MAIN_PANE - self.H_ICON)/2 + self.top.Y_ITEMS_PANE
		self.X_LEADICON = self.top.X_ITEMS_PANE + self.W_MAIN_PANE - 30 - 64
		self.H_ARROW = 36
		self.X_ARROW = self.X_ICON + self.H_ICON + (self.X_LEADICON - self.X_ICON - self.H_ICON) / 2 - self.H_ARROW/2
		self.Y_ARROW = self.Y_ICON + self.H_ICON/2 - self.H_ARROW/2

		self.X_EFFECTS = self.top.X_ITEMS_PANE + self.W_MAIN_PANE + self.top.W_BORDER
		self.W_EFFECTS = self.top.W_ITEMS_PANE - self.W_MAIN_PANE - self.top.W_BORDER
		self.Y_EFFECTS = self.top.Y_ITEMS_PANE

		self.Y_SPECIAL = self.top.Y_ITEMS_PANE + self.H_MAIN_PANE + 10
		self.H_SPECIAL = max(1, screen.getYResolution() - self.Y_SPECIAL - 60)
		iContentWidth = self.top.W_ITEMS_PANE - self.top.W_BORDER * 2
		self.W_AVAILABLE = int(iContentWidth * 0.36)
		self.W_YIELD = int(iContentWidth * 0.26)
		self.W_HISTORY = iContentWidth - self.W_AVAILABLE - self.W_YIELD
		self.X_YIELD = self.top.X_ITEMS_PANE + self.W_AVAILABLE + self.top.W_BORDER
		self.X_HISTORY = self.X_YIELD + self.W_YIELD + self.top.W_BORDER

		szHeader = gc.getSpecialistInfo(self.iSpecialist).getDescription().upper()
		szHeader = u"<font=4b>" + self.top.color4 + CyTranslator().getText(self.top.sSpecialistIcon, ()) + szHeader + " " + CyTranslator().getText(self.top.sSpecialistIcon, ()) + "</color></font>"
		screen.setLabel(self.top.getNextWidgetName(), "Background", szHeader, CvUtil.FONT_CENTER_JUSTIFY, screen.getXResolution()/2, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		
		screen.setText(self.top.getNextWidgetName(), "Background", self.top.MENU_TEXT, CvUtil.FONT_CENTER_JUSTIFY, screen.getXResolution()/2, screen.getYResolution() - 42, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_MAIN, self.top.PLATYPEDIA_SPECIALIST, -1)
		self.top.addPediaPanel(screen,  self.top.getNextWidgetName(), "", "", False, False, self.top.X_ITEMS_PANE, self.top.Y_ITEMS_PANE, self.W_MAIN_PANE, self.H_MAIN_PANE, PanelStyles.PANEL_STYLE_MAIN_BLACK50)
		self.top.addPediaPanel(screen, self.top.getNextWidgetName(), "", "", false, false, self.X_ICON, self.Y_ICON, self.H_ICON, self.H_ICON, PanelStyles.PANEL_STYLE_MAIN)
		screen.addDDSGFC(self.top.getNextWidgetName(), gc.getSpecialistInfo(self.iSpecialist).getButton(), self.X_ICON + self.H_ICON/2 - 64/2, self.Y_ICON + self.H_ICON/2 - 64/2, 64, 64, WidgetTypes.WIDGET_GENERAL, -1, -1 )

		self.placeSpecial()
		self.placeYield()
		self.placeText()
		self.placeLinks(self.top.iLastScreen == CvScreenEnums.PEDIA_SPECIALIST and screen.isActive())
		self.top.iLastScreen = CvScreenEnums.PEDIA_SPECIALIST

	def placeSpecial(self):
		screen = self.top.getScreen()
		self.top.addPediaPanel(screen, self.top.getNextWidgetName(), CyTranslator().getText("TXT_KEY_PEDIA_EFFECTS", ()), "", true, false, self.X_EFFECTS, self.Y_EFFECTS, self.W_EFFECTS, self.H_MAIN_PANE, PanelStyles.PANEL_STYLE_MAIN_BLACK50 )
		szSpecialText = CyGameTextMgr().getSpecialistHelp(self.iSpecialist, True)
## Help Tag ##
		if len(gc.getSpecialistInfo(self.iSpecialist).getHelp()) > 0:
			szSpecialText += CyTranslator().getText("[NEWLINE][ICON_BULLET]", ()) + gc.getSpecialistInfo(self.iSpecialist).getHelp()
## Help Tag ##
		self.top.addPediaBodyText(screen, self.top.getNextWidgetName(), szSpecialText, self.X_EFFECTS+10, self.Y_EFFECTS+35, self.W_EFFECTS-20, self.H_MAIN_PANE-45, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)	

	def placeYield(self):
		screen = self.top.getScreen()
		self.top.addPediaPanel(screen, self.top.getNextWidgetName(), CyTranslator().getText("TXT_KEY_PEDIA_PROMOTION_UNITS", ()), "", False, True, self.top.X_ITEMS_PANE, self.Y_SPECIAL, self.W_AVAILABLE, self.H_SPECIAL, PanelStyles.PANEL_STYLE_MAIN_BLACK50 )
		self.top.addPediaPanel(screen, self.top.getNextWidgetName(), CyTranslator().getText("TXT_KEY_PEDIA_YIELDS", ()), "", False, True, self.X_YIELD, self.Y_SPECIAL, self.W_YIELD, self.H_SPECIAL, PanelStyles.PANEL_STYLE_MAIN_BLACK50 )
		self.top.addPediaPanel(screen, self.top.getNextWidgetName(), CyTranslator().getText("TXT_KEY_CIVILOPEDIA_HISTORY", ()), "", False, True, self.X_HISTORY, self.Y_SPECIAL, self.W_HISTORY, self.H_SPECIAL, PanelStyles.PANEL_STYLE_MAIN_BLACK50)
		panelName = self.top.getNextWidgetName()
		panelName2 = self.top.getNextWidgetName()
		screen.addScrollPanel(panelName, "", self.top.X_ITEMS_PANE - 2, self.Y_SPECIAL + 20, self.W_AVAILABLE + 4, self.H_SPECIAL - 46, PanelStyles.PANEL_STYLE_EMPTY)
		screen.addScrollPanel(panelName2, "", self.X_YIELD - 2, self.Y_SPECIAL + 20, self.W_YIELD + 4, self.H_SPECIAL - 46, PanelStyles.PANEL_STYLE_EMPTY)

		iY1 = 6
		iY2 = 6	
		
		lClass = []
		for i in xrange(gc.getNumUnitClassInfos()):
			item = gc.getUnitClassInfo(i).getDefaultUnitIndex()
			if CyGame().getActiveCivilizationType() > -1:
				item = gc.getCivilizationInfo(CyGame().getActiveCivilizationType()).getCivilizationUnits(i)
			if item == -1: continue
			Info = gc.getUnitInfo(item)
			if i == gc.getSpecialistInfo(self.iSpecialist).getGreatPeopleUnitClass():
				screen.setImageButton(self.top.getNextWidgetName(), Info.getButton(), self.X_LEADICON, self.Y_ICON + self.H_ICON/2 - 64/2, 64, 64, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, item, 1)
				screen.setButtonGFC(self.top.getNextWidgetName(), "", "", self.X_ARROW, self.Y_ARROW, self.H_ARROW, self.H_ARROW, WidgetTypes.WIDGET_GENERAL, -1, -1, ButtonStyles.BUTTON_STYLE_ARROW_RIGHT)
			if Info.getGreatPeoples(self.iSpecialist):
				if i in lClass: continue
				lClass.append(i)

		for item in xrange(gc.getNumCivicInfos()):
			Info = gc.getCivicInfo(item)
			if Info.isSpecialistValid(self.iSpecialist):
				sText = CyTranslator().getText("TXT_KEY_PEDIA_UNLIMITED", ())
				screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, Info.getButton(), 0, iY1, self.iSize, self.iSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, item, 1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName, u"<font=5>" + Info.getDescription() + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY1, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName, u"<font=5>" + sText + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY1 + self.iSize/2, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				iY1 += (self.iSize + 4)
			sText = ""
			for j in xrange(CommerceTypes.NUM_COMMERCE_TYPES):
				iCommerceChange = Info.getSpecialistExtraCommerce(j)
				if iCommerceChange != 0:
					sText += u"%+d%c" % (iCommerceChange, gc.getCommerceInfo(j).getChar())
			if len(sText):
				screen.setImageButtonAt(self.top.getNextWidgetName(), panelName2, Info.getButton(), 0, iY2, self.iSize, self.iSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_CIVIC, item, 1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName2, u"<font=5>" + Info.getDescription() + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY2, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName2, u"<font=5>" + sText + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY2 + self.iSize/2, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				iY2 += (self.iSize + 4)

		for item in xrange(gc.getNumSpecialistInfos()):
			Info = gc.getSpecialistInfo(item)
			if Info.getGreatPeopleUnitClass() in lClass:
				sText = u"%+d%s" % (Info.getGreatPeopleRateChange(), CyTranslator().getText("[ICON_GREATPEOPLE]", ()))
				screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, Info.getButton(), 0, iY1, self.iSize, self.iSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_SPECIALIST, item, 1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName, u"<font=5>" + Info.getDescription() + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY1, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName, u"<font=5>" + sText + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY1 + self.iSize/2, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				iY1 += (self.iSize + 4)

		for i in xrange(gc.getNumBuildingClassInfos()):
			item = gc.getBuildingClassInfo(i).getDefaultBuildingIndex()
			if CyGame().getActiveCivilizationType() > -1:
				item = gc.getCivilizationInfo(CyGame().getActiveCivilizationType()).getCivilizationBuildings(i)
			if item == -1: continue
			Info = gc.getBuildingInfo(item)
			iSlot = Info.getSpecialistCount(self.iSpecialist)
			if iSlot != 0:
				sText = u"%+d %s" % (iSlot, CyTranslator().getText("TXT_KEY_PEDIA_CAPACITY", ()))
				screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, Info.getButton(), 0, iY1, self.iSize, self.iSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, item, 1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName, u"<font=5>" + Info.getDescription() + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY1, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName, u"<font=5>" + sText + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY1 + self.iSize/2, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				iY1 += (self.iSize + 4)
			if Info.getGreatPeopleUnitClass() in lClass:
				sText = u"%+d%s" % (Info.getGreatPeopleRateChange(), CyTranslator().getText("[ICON_GREATPEOPLE]", ()))
				screen.setImageButtonAt(self.top.getNextWidgetName(), panelName, Info.getButton(), 0, iY1, self.iSize, self.iSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, item, 1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName, u"<font=5>" + Info.getDescription() + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY1, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName, u"<font=5>" + sText + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY1 + self.iSize/2, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				iY1 += (self.iSize + 4)
			sText = ""
			for j in xrange(YieldTypes.NUM_YIELD_TYPES):
				iYieldChange = Info.getSpecialistYieldChange(self.iSpecialist, j)
				if iYieldChange != 0:
					sText += u"%+d%c" % (iYieldChange, gc.getYieldInfo(j).getChar())
			if len(sText):
				screen.setImageButtonAt(self.top.getNextWidgetName(), panelName2, Info.getButton(), 0, iY2, self.iSize, self.iSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, item, 1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName2, u"<font=5>" + Info.getDescription() + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY2, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName2, u"<font=5>" + sText + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY2 + self.iSize/2, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				iY2 += (self.iSize + 4)

		# Read the loaded infos rather than scraping XML. This includes modular
		# buildings while avoiding lookups for types absent from this mod (e.g.
		# BUILDING_PALACE in Fallout).
		for item in xrange(gc.getNumBuildingInfos()):
			Info = gc.getBuildingInfo(item)
			sText = ""
			for j in xrange(CommerceTypes.NUM_COMMERCE_TYPES):
				iCommerceChange = Info.getSpecialistExtraCommerce(j)
				if iCommerceChange != 0:
					sText += u"%+d%c" % (iCommerceChange, gc.getCommerceInfo(j).getChar())
			if len(sText):
				screen.setImageButtonAt(self.top.getNextWidgetName(), panelName2, Info.getButton(), 0, iY2, self.iSize, self.iSize, WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, item, 1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName2, u"<font=5>" + Info.getDescription() + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY2, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				screen.setLabelAt(self.top.getNextWidgetName(), panelName2, u"<font=5>" + sText + u"</font>", CvUtil.FONT_LEFT_JUSTIFY, self.iSize + 4, iY2 + self.iSize/2, -0.1, FontTypes.SMALL_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
				iY2 += (self.iSize + 4)

	def placeText(self):
		screen = self.top.getScreen()
		szText = ""
		sStrategy = gc.getSpecialistInfo(self.iSpecialist).getStrategy()
		if len(sStrategy) and sStrategy.find("TXT_KEY") == -1:
			szText += CyTranslator().getText("TXT_KEY_CIVILOPEDIA_STRATEGY", ())
			szText += sStrategy + "\n\n"
			szText += CyTranslator().getText("TXT_KEY_CIVILOPEDIA_BACKGROUND", ())
		sPedia = gc.getSpecialistInfo(self.iSpecialist).getCivilopedia()
		if sPedia.find("TXT_KEY") == -1:
			szText += sPedia	
		self.top.addPediaBodyText(screen, self.top.getNextWidgetName(), szText, self.X_HISTORY + 10, self.Y_SPECIAL + 30, self.W_HISTORY - 20, self.H_SPECIAL - 30, WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
		
	def placeLinks(self, bRedraw):
		screen = self.top.getScreen()
		if bRedraw:
			screen.show("PlatyTable")
			return
		screen.addTableControlGFC("PlatyTable", 1, self.top.X_PANEL, 55, self.top.W_PANEL, screen.getYResolution() - 110, False, False, 24, 24, TableStyles.TABLE_STYLE_STANDARD);
		screen.enableSelect("PlatyTable", True)
		screen.setTableColumnHeader("PlatyTable", 0, "", self.top.W_PANEL)
		listSorted = self.top.sortSpecialists(self.top.iSortSpecialists)
		self.top.placePediaLinks(listSorted, CyTranslator().getText(self.top.sSpecialistIcon, ()), self.iSpecialist, WidgetTypes.WIDGET_PEDIA_JUMP_TO_SPECIALIST, -1)

	def handleInput (self, inputClass):
		return 0
