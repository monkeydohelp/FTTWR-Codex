# ACBase Test: shared civic improvement-spawn help for the Civics screen and
# Civic Civilopedia.  The data comes from the existing CvImprovementInfo
# Python hooks, so this stays synchronized with the runtime civic checks.

from CvPythonExtensions import *

gc = CyGlobalContext()
localText = CyTranslator()


def _literalLink(info):
	return u"<link=literal>%s</link>" % info.getDescription()


def _formatRateModifier(iModifier):
	iDelta = iModifier - 100
	if iDelta > 0:
		return u"+%d%%" % iDelta
	return u"%d%%" % iDelta


def getCivicSpawnHelp(iCivic):
	"""Return localized help lines for improvement spawns gated by iCivic."""
	szHelp = u""
	for iImprovement in range(gc.getNumImprovementInfos()):
		kImprovement = gc.getImprovementInfo(iImprovement)
		iUnit = kImprovement.getSpawnUnitType()
		if iUnit < 0 or iUnit >= gc.getNumUnitInfos():
			continue
		# CvPlot::doSpawn and CvUnit::build apply civic requirements to
		# owner-controlled spawns. Barbarian spawns intentionally bypass them.
		if kImprovement.isSpawnBarbarian() or not kImprovement.getSpawnRequiresCivic(iCivic):
			continue

		szImprovement = _literalLink(kImprovement)
		szUnit = _literalLink(gc.getUnitInfo(iUnit))
		if kImprovement.isSpawnOnBuild():
			szHelp += u"\n" + localText.getText(
				"TXT_KEY_CIVIC_SPAWN_UNIT_ON_BUILD", (szImprovement, szUnit))
		else:
			szHelp += u"\n" + localText.getText(
				"TXT_KEY_CIVIC_SPAWN_UNIT_OVER_TIME", (szImprovement, szUnit))

		iModifier = kImprovement.getCivicSpawnRateModifier(iCivic)
		if iModifier >= 0 and iModifier != 100:
			szHelp += u"\n" + localText.getText(
				"TXT_KEY_CIVIC_SPAWN_RATE_MODIFIER",
				(szImprovement, _formatRateModifier(iModifier)))

	return szHelp


def getCivicSlaveryHelp(iCivic):
	"""Return the Archid Slavery mechanics summary for the Slavery civic."""
	# Keep this tied to the civic type rather than the active player's civic. The
	# Civics screen also previews civics that are not currently adopted.
	if iCivic != gc.getInfoTypeForString("CIVIC_SLAVERY"):
		return u""

	# The C++ sale path falls back to 100 when the define is missing/invalid;
	# mirror that fallback so the screen never advertises a zero-value sale.
	iSaleValue = gc.getDefineINT("SLAVERY_SLAVE_SALE_VALUE")
	if iSaleValue <= 0:
		iSaleValue = 100

	return u"\n" + localText.getText(
		"TXT_KEY_CIVIC_SLAVERY_MECHANICS_HELP", (iSaleValue,))
