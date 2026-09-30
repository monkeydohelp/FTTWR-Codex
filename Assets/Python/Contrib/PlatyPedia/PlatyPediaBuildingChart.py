from CvPythonExtensions import *
import CvUtil
import ScreenInput
gc = CyGlobalContext()

class CvPediaBuildingChart:
	def __init__(self, main):
		self.top = main

	def interfaceScreen(self):		
		screen = self.top.getScreen()
		szTable = self.top.getNextWidgetName()
		iNumColumns = 12
		screen.addTableControlGFC(szTable, iNumColumns, self.top.X_ITEMS_PANE, self.top.Y_ITEMS_PANE + 6, self.top.W_ITEMS_PANE, self.top.H_ITEMS_PANE, True, False, 30, 30, TableStyles.TABLE_STYLE_STANDARD)
		screen.enableSort(szTable)
		screen.setTableColumnHeader(szTable, 0, "", self.top.W_ITEMS_PANE /4)
		iWidth = self.top.W_ITEMS_PANE * 3/4
		iNumColumns -= 1
		screen.setTableColumnHeader(szTable, 1, CyTranslator().getText("[ICON_PRODUCTION]", ()), iWidth/iNumColumns)
		screen.setTableColumnHeader(szTable, 2, CyTranslator().getText("[ICON_HAPPY]", ()), iWidth/iNumColumns)
		screen.setTableColumnHeader(szTable, 3, CyTranslator().getText("[ICON_HEALTHY]", ()), iWidth/iNumColumns)
		screen.setTableColumnHeader(szTable, 4, CyTranslator().getText("[ICON_GREATPEOPLE]", ()), iWidth/iNumColumns)
		for j in xrange(YieldTypes.NUM_YIELD_TYPES):
			screen.setTableColumnHeader(szTable, 5 + j, u"%c" % gc.getYieldInfo(j).getChar(), iWidth/iNumColumns)
		for j in xrange(CommerceTypes.NUM_COMMERCE_TYPES):
			screen.setTableColumnHeader(szTable, 5 + j + YieldTypes.NUM_YIELD_TYPES, u"%c" % gc.getCommerceInfo(j).getChar(), iWidth/iNumColumns)

		for i in xrange(gc.getNumBuildingInfos()):
			if gc.getDefineINT("CIVILOPEDIA_SHOW_ACTIVE_CIVS_ONLY") and CyGame().isFinalInitialized():
				if not CyGame().isBuildingEverActive(i): continue
			Info = gc.getBuildingInfo(i)
			if self.top.iSortBChart == 0:
				if not isNationalWonderClass(Info.getBuildingClassType()): continue
			elif self.top.iSortBChart == 1:
				if not isTeamWonderClass(Info.getBuildingClassType()): continue
			elif self.top.iSortBChart == 2:
				if not isWorldWonderClass(Info.getBuildingClassType()): continue
			iRow = screen.appendTableRow(szTable)
			screen.setTableText(szTable, 0, iRow, u"<font=5>" + self.top.color3 + Info.getDescription() + u"</color></font>", Info.getButton(), WidgetTypes.WIDGET_PEDIA_JUMP_TO_BUILDING, i, 1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableInt(szTable, 1, iRow, u"<font=5>" + self.top.color3 + str(Info.getProductionCost()) + u"</color></font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableInt(szTable, 2, iRow, u"<font=5>" + self.top.color3 + str(Info.getHappiness()) + u"</color></font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableInt(szTable, 3, iRow, u"<font=5>" + self.top.color3 + str(Info.getHealth()) + u"</color></font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			screen.setTableInt(szTable, 4, iRow, u"<font=5>" + self.top.color3 + str(Info.getGreatPeopleRateChange()) + u"</color></font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			for j in xrange(YieldTypes.NUM_YIELD_TYPES):
				screen.setTableInt(szTable, 5 + j, iRow, u"<font=5>" + self.top.color3 + str(Info.getYieldChange(j)) + u"</color></font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)
			for j in xrange(CommerceTypes.NUM_COMMERCE_TYPES):
				screen.setTableInt(szTable, 5 + YieldTypes.NUM_YIELD_TYPES + j, iRow, u"<font=5>" + self.top.color3 + str(Info.getObsoleteSafeCommerceChange(j) + Info.getCommerceChange(j)) + u"</color></font>", "", WidgetTypes.WIDGET_GENERAL, -1, -1, CvUtil.FONT_LEFT_JUSTIFY)

	def handleInput (self, inputClass):
		return 0
