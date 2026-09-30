from CvPythonExtensions import *


def onGameStart(argsList):
	# Keep the reference SuperForts startup contract: initialize the map's
	# canal/choke metadata once the game map is available.
	CyMap().calculateCanalAndChokePoints()
