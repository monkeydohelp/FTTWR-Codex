from CvPythonExtensions import *
import CvUtil
import ScreenInput
import CvScreenEnums
gc = CyGlobalContext()

class CvPediaUnitChart:
	def __init__(self, main):
		self.iGroup = -1
		self.top = main

	def interfaceScreen(self, iGroup):
		self.iGroup = iGroup
		self.top.deleteAllWidgets()				
		screen = self.top.getScreen()
		if not screen.isActive():
			self.top.setPediaCommonWidgets()

		if self.iGroup == -1:
			szHeader = CyTranslator().getText("TXT_PEDIA_NON_COMBAT", ())
		elif self.iGroup == -2:
			szHeader = CyTranslator().getText("TXT_KEY_PEDIA_ALL_GROUPS", ())
		else:
			szHeader = gc.getUnitCombatInfo(self.iGroup).getDescription().upper()
		szHeader = u"<font=4b>" + self.top.color4 + CyTranslator().getText(self.top.sUnitCombatIcon, ()) + szHeader + " " + CyTranslator().getText(self.top.sUnitCombatIcon, ()) + "</color></font>"
		screen.setLabel(self.top.getNextWidgetName(), "Background", szHeader, CvUtil.FONT_CENTER_JUSTIFY, screen.getXResolution()/2, self.top.Y_TITLE, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_GENERAL, -1, -1)
		
		screen.setText(self.top.getNextWidgetName(), "Background", self.top.MENU_TEXT, CvUtil.FONT_CENTER_JUSTIFY, screen.getXResolution()/2, screen.getYResolution() - 42, 0, FontTypes.TITLE_FONT, WidgetTypes.WIDGET_PEDIA_MAIN, self.top.PLATYPEDIA_UNIT_GROUP, -1)
		self.placeUnitTable()
		self.placeLinks(self.top.iLastScreen == CvScreenEnums.PEDIA_UNIT_CHART and screen.isActive())
		self.top.iLastScreen = CvScreenEnums.PEDIA_UNIT_CHART
		
	def placeUnitTable(self):
		screen = self.top.getScreen()

		szTable = self.top.getNextWidgetName()
		iNumColumns = 8 + 1
		screen.addTableControlGFC(szTable, iNumColumns - 1, self.top.X_ITEMS_PANE, self.top.Y_ITEMS_PANE + 6, self.top.W_ITEMS_PANE, self.top.H_ITEMS_PANE, True, False, 26, 26, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(szTable)
		screen.setTableColumnHeader(szTable, 0, "", self.top.W_ITEMS_PANE * 2/iNumColumns)
		screen.setTableColumnHeader(szTable, 1, "<font=4>" + CyTranslator().getText("TXT_KEY_PEDIA_DOMAIN", ()) + "</font>", self.top.W_ITEMS_PANE/iNumColumns)
		screen.setTableColumnHeader(szTable, 2, u"%c" % CyGame().getSymbolID(FontSymbols.STRENGTH_CHAR), self.top.W_ITEMS_PANE/iNumColumns)
		screen.setTableColumnHeader(szTable, 3, u"%c" % CyGame().getSymbolID(FontSymbols.MOVES_CHAR), self.top.W_ITEMS_PANE/iNumColumns)
		screen.setTableColumnHeader(szTable, 4, u"%c" % gc.getYieldInfo(YieldTypes.YIELD_PRODUCTION).getChar(), self.top.W_ITEMS_PANE/iNumColumns)
		screen.setTableColumnHeader(szTable, 5, "<font=4>" + CyTranslator().getText("TXT_KEY_PEDIA_AIR_RANGE", ()) + "</font>", self.top.W_ITEMS_PANE/iNumColumns)
		screen.setTableColumnHeader(szTable, 6, "<font=4>" + CyTranslator().getText("TXT_KEY_PEDIA_WITHDRAWAL", ()) + "</font>", self.top.W_ITEMS_PANE/iNumColumns)
		screen.setTableColumnHeader(szTable, 7, "<font=4>" + CyTranslator().getText("TXT_KEY_MISSION_BOMBARD", ()) + "</font>", self.top.W_ITEMS_PANE/iNumColumns)

		for j in xrange(gc.getNumUnitInfos()):
			if gc.getDefineINT("CIVILOPEDIA_SHOW_ACTIVE_CIVS_ONLY") and CyGame().isFinalInitialized():
				if not CyGame().isBuildingEverActive(j): continue
			UnitInfo = gc.getUnitInfo(j)
			if (self.iGroup == UnitInfo.getUnitCombatType() or self.iGroup == -2):
				if UnitInfo.getDomainType() == DomainTypes.DOMAIN_AIR:
					iStrength = UnitInfo.getAirCombat()
					iBomb = UnitInfo.getBombRate()
					iWithdrawal = UnitInfo.getEvasionProbability()
				else:
					iStrength = UnitInfo.getCombat()
					iBomb = UnitInfo.getBombardRate()
					iWithdrawal = UnitInfo.getWithdrawalProbability()
				
				if gc.getUnitInfo(j).getProductionCost() < 0:
					szCost = CyTranslator().getText("TXT_KEY_NON_APPLICABLE", ())
				else:
					szCost = UnitInfo.getProductionCost()
				sDomain = gc.getDomainInfo(gc.getUnitInfo(j).getDomainType()).getDescription()
				sDomain = sDomain[:sDomain.find(" ")]
				iRow = screen.appendTableRow(szTable)
				screen.setTableText(szTable, 0, iRow, u"<font=4>" + gc.getUnitInfo(j).getDescription() + u"</font>", gc.getUnitInfo(j).getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT, j, 1, CvUtil.FONT_LEFT_JUSTIFY)						
				screen.setTableText(szTable, 1, iRow, u"<font=4>" + sDomain + u"</font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				screen.setTableInt(szTable, 2, iRow, u"<font=4>" + unicode(iStrength) + u"</font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				screen.setTableInt(szTable, 3, iRow, u"<font=4>" + unicode(UnitInfo.getMoves()) + u"</font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				screen.setTableInt(szTable, 4, iRow, u"<font=4>" + unicode(szCost) + u"</font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				iRange = UnitInfo.getRangedCombatRange()
				if UnitInfo.getDomainType() == DomainTypes.DOMAIN_AIR:
					iRange = UnitInfo.getAirRange()
				screen.setTableInt(szTable, 5, iRow, u"<font=4>" + unicode(iRange) + u"</font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				screen.setTableInt(szTable, 6, iRow, u"<font=4>" + unicode(iWithdrawal) + u"</font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
				screen.setTableInt(szTable, 7, iRow, u"<font=4>" + unicode(iBomb) + u"</font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def placeLinks(self, bRedraw):
		screen = self.top.getScreen()
		if bRedraw:
			screen.show("PlatyTable")
			return
		screen.addTableControlGFC("PlatyTable", 1, self.top.X_PANEL, 55, self.top.W_PANEL, screen.getYResolution() - 110, False, False, 26, 26, TableStyles.TABLE_STYLE_STANDARD);
		screen.enableSelect("PlatyTable", True)
		screen.setTableColumnHeader("PlatyTable", 0, "", self.top.W_PANEL)

		iRow = screen.appendTableRow("PlatyTable")
		if self.iGroup == -2:
			screen.selectRow("PlatyTable", iRow, True)
		sText = "<font=4>" + self.top.color3 + CyTranslator().getText("TXT_KEY_PEDIA_ALL_GROUPS", ()) + "</font></color>"
		screen.setTableText("PlatyTable", 0, iRow, sText, ",Art/Interface/Buttons/Promotions/Combat5.dds,Art/Interface/Buttons/Warlords_Atlas_1.dds,5,10", WidgetTypes.WIDGET_PYTHON, 6781, -2, CvUtil.FONT_LEFT_JUSTIFY )
		iRow = screen.appendTableRow("PlatyTable")
		if self.iGroup == -1:
			screen.selectRow("PlatyTable", iRow, True)
		sText = "<font=4>" + self.top.color3 + CyTranslator().getText("TXT_PEDIA_NON_COMBAT", ()) + "</font></color>"
		screen.setTableText("PlatyTable", 0, iRow, sText, CyArtFileMgr().getInterfaceArtInfo("INTERFACE_BUTTONS_CANCEL").getPath(), WidgetTypes.WIDGET_PYTHON, 6781, -1, CvUtil.FONT_LEFT_JUSTIFY )

		listSorted = self.top.sortUnitGroups(0)
		self.top.placePediaLinks(listSorted, CyTranslator().getText(self.top.sUnitCombatIcon, ()), self.iGroup, WidgetTypes.WIDGET_PEDIA_JUMP_TO_UNIT_COMBAT, -1)

	def handleInput (self, inputClass):
		return 0
