# Fury Road map generator

from CvPythonExtensions import *
import CvUtil
import DevKitHarness
import CvMapGeneratorUtil
from CvMapGeneratorUtil import MultilayeredFractal
from CvMapGeneratorUtil import HintedWorld
from CvMapGeneratorUtil import TerrainGenerator
from CvMapGeneratorUtil import FeatureGenerator

# Everybody needs it:
gc = CyGlobalContext() ; map = CyMap() ; game = CyGame()

# Print routine for debugging.  Stubbed in release version.
def myPrint(s):
   return
   f = open ("c:\\terrain.txt", "a")
   f.write(s)
   f.close()

def beforeGeneration():
   DevKitHarness.start("Fury_Road")

def afterGeneration():
   DevKitHarness.mark("MAP_GENERATION_COMPLETE")

### BASIC ROUTINES

def getDescription():
   return "TXT_KEY_MAP_SCRIPT_FURY_ROAD_DESCR"

def isClimateMap():
   return 0

def isSeaLevelMap():
   return 0

def isAdvancedMap():
   "This map should show up in simple mode"
   return 0

def getGridSize(argsList):
   # Reduce grid sizes by one level.
   grid_sizes = {
      WorldSizeTypes.WORLDSIZE_DUEL:      (8,5),
      WorldSizeTypes.WORLDSIZE_TINY:      (10,6),
      WorldSizeTypes.WORLDSIZE_SMALL:      (13,8),
      WorldSizeTypes.WORLDSIZE_STANDARD:   (16,10),
      WorldSizeTypes.WORLDSIZE_LARGE:      (21,13),
      WorldSizeTypes.WORLDSIZE_HUGE:      (26,16)
   }

   if (argsList[0] == -1): # (-1,) is passed to function on loads
      return []
   [eWorldSize] = argsList
   return grid_sizes[eWorldSize]

def generatePangaea(): # isenchine changes based on BtS Pangaea map script: def generateAndysHintedPangaea
	global my_world
	# my_world = HintedWorld(8,4) # isenchine
	my_world = HintedWorld(16,8)

	mapRand = CyGlobalContext().getGame().getMapRand()

	# isenchine
	numBlocks = my_world.w * my_world.h
	numBlocksLand = int(numBlocks*1)
	cont = my_world.addContinent(numBlocksLand,mapRand.get(5, "Generate Plot Types PYTHON")+4,mapRand.get(3, "Generate Plot Types PYTHON")+2)
	if cont:
		for x in range(my_world.w):
			for y in (0, my_world.h - 1):
				my_world.setValue(x,y, 1) # force ocean at poles
		my_world.buildAllContinents()
		return my_world.generatePlotTypes(shift_plot_types=True)
	
	else: # Couldn't create continent: reverting to Fury Road Hinted Pangaea!
		for y in range(my_world.h):
			for x in range(my_world.w):
				if x in (0, my_world.w-1) or y in (0, my_world.h-1):
					my_world.setValue(x,y,0)
				else:
					my_world.setValue(x,y,200 + mapRand.get(55, ""))

		my_world.setValue(1, 1 + mapRand.get(3, ""), mapRand.get(64, ""))
		my_world.setValue(2 + mapRand.get(2, ""), 1 + mapRand.get(3, ""), mapRand.get(64, ""))
		my_world.setValue(4 + mapRand.get(2, ""), 1 + mapRand.get(3, ""), mapRand.get(64, ""))
		my_world.setValue(6, 1 + mapRand.get(3, ""), mapRand.get(64, ""))
		if (mapRand.get(2, "") == 0):
			my_world.setValue(2, 1 + mapRand.get(3, ""), mapRand.get(64, ""))
		else:
			my_world.setValue(5, 1 + mapRand.get(3, ""), mapRand.get(64, ""))

		my_world.buildAllContinents()
		return my_world.generatePlotTypes(shift_plot_types=True)
   
def getLargestLandAreaSize(plotTypes, map):
   # Score candidate arrays locally rather than rebuilding native map areas
   # for each attempt. Bonus placement later asserts if it encounters an
   # empty CvArea, so keep native area mutation out of this search loop.
   iWidth = map.getGridWidth()
   iHeight = map.getGridHeight()
   iNumPlots = iWidth * iHeight
   if iWidth <= 0 or iHeight <= 0 or len(plotTypes) != iNumPlots:
      return (0, 0)

   bWrapX = map.isWrapX()
   bWrapY = map.isWrapY()
   aiVisited = [False] * iNumPlots
   iTotalLandPlots = 0
   iLargestArea = 0

   for iStart in range(iNumPlots):
      if plotTypes[iStart] == PlotTypes.PLOT_OCEAN:
         continue
      iTotalLandPlots += 1
      if aiVisited[iStart]:
         continue

      aiVisited[iStart] = True
      aiStack = [iStart]
      iAreaSize = 0
      while len(aiStack) > 0:
         iCurrent = aiStack.pop()
         iAreaSize += 1
         iX = iCurrent % iWidth
         iY = iCurrent // iWidth

         for iDeltaY in (-1, 0, 1):
            iNeighborY = iY + iDeltaY
            if iNeighborY < 0 or iNeighborY >= iHeight:
               if not bWrapY:
                  continue
               iNeighborY = iNeighborY % iHeight

            for iDeltaX in (-1, 0, 1):
               if iDeltaX == 0 and iDeltaY == 0:
                  continue
               iNeighborX = iX + iDeltaX
               if iNeighborX < 0 or iNeighborX >= iWidth:
                  if not bWrapX:
                     continue
                  iNeighborX = iNeighborX % iWidth

               iNeighbor = iNeighborY * iWidth + iNeighborX
               if (not aiVisited[iNeighbor] and
                     plotTypes[iNeighbor] != PlotTypes.PLOT_OCEAN):
                  aiVisited[iNeighbor] = True
                  aiStack.append(iNeighbor)

      if iAreaSize > iLargestArea:
         iLargestArea = iAreaSize

   return (iLargestArea, iTotalLandPlots)

def generatePlotTypes():
   map = CyMap()
   bestPlotTypes = None
   bestCohesion = -1.0

   # The old unbounded loop could hang forever while looking for a map
   # with virtually every land tile on one continent. Keep the best of a
   # finite number of attempts instead.
   for iAttempt in range(32):
      plotTypes = generatePangaea()
      if plotTypes is None:
         continue
      iBiggestAreaPlots, iTotalLandPlots = getLargestLandAreaSize(plotTypes, map)
      if iTotalLandPlots <= 0:
         continue
      cohesion = float(iBiggestAreaPlots) / float(iTotalLandPlots)
      if cohesion > bestCohesion:
         bestCohesion = cohesion
         bestPlotTypes = plotTypes[:]
      if cohesion >= 0.9999:
         return plotTypes

   if bestPlotTypes is not None:
      return bestPlotTypes

   # generatePangaea normally always succeeds, but never return None to Civ IV.
   return generatePangaea()

def generateTerrainTypes():
   # 6/5/08: was 50, 50; too hard to expand
   terraingen = TerrainGenerator(iDesertPercent=30, iPlainsPercent=30)
   terrainTypes = terraingen.generateTerrain()
   return terrainTypes

# Adding fallout, or an existing peak, may introduce a blockage on
# a one square landbridge.  Detect that.
def makesBlockage(x, y):
   coast = gc.getInfoTypeForString("TERRAIN_COAST")
   blockLeft = false ; blockAbove = false
   # three across
   if not map.isPlot(x-1, y): blockLeft = true
   elif map.plot(x-1, y).getTerrainType() == coast: blockLeft = true
   if blockLeft:
      if not map.isPlot(x+1, y): return true
      elif map.plot(x+1, y).getTerrainType() == coast: return true
   # three down
   if not map.isPlot(x, y-1): blockAbove = true
   elif map.plot(x, y-1).getTerrainType() == coast: blockAbove = true
   if blockAbove:
      if not map.isPlot(x, y+1): return true
      elif map.plot(x, y+1).getTerrainType() == coast: return true
   return false

# After adding features with default generator, edit map
def addFeatures():
   featuregen = FeatureGenerator(iJunglePercent=50, iForestPercent=20)
   featuregen.addFeatures()
   fall = gc.getInfoTypeForString("FEATURE_FALLOUT")
   coast  = gc.getInfoTypeForString("TERRAIN_COAST")
   ocean = gc.getInfoTypeForString("TERRAIN_OCEAN")
   grass = gc.getInfoTypeForString("TERRAIN_PARADISE")
   plains = gc.getInfoTypeForString("TERRAIN_FERTILE_WASTELAND")
   forest = gc.getInfoTypeForString("FEATURE_FOREST")
   nofeat = FeatureTypes.NO_FEATURE
   nobonus = BonusTypes.NO_BONUS
   # Check if a mountain introduces a blockage and remove it.  Update database
   # at end of loop with no-op, to rebuild area index in particular
   for iPlotLoop in range(map.numPlots()):
      pPlot = map.plotByIndex(iPlotLoop)
      if pPlot.isImpassable():
         if makesBlockage(pPlot.getX(), pPlot.getY()):
            pPlot.setTerrainType(plains, false, false)
   if map.numPlots() > 0:
      pPlot = map.plotByIndex(0)
      pPlot.setTerrainType(pPlot.getTerrainType(), true, true)
   # Remove islands.  Third try, it seems findBiggestArea dynamically
   # updates during removal.  So list all the plots and remove after.
   biggestArea = map.findBiggestArea(false)
   if biggestArea is None or biggestArea.isNone():
      return
   iArea = biggestArea.getID()
   lIslands = []
   for iPlotLoop in range(map.numPlots()):
      pPlot = map.plotByIndex(iPlotLoop)
      if pPlot.getArea() == iArea: continue
      if pPlot.getTerrainType() == ocean: continue
      if pPlot.getTerrainType() == coast: continue
      lIslands.append(pPlot)
   myPrint("\nStart removal of %d plots\n" % len(lIslands))
   for pPlot in lIslands:
      pPlot.setTerrainType(ocean, true, true)
      pPlot.setFeatureType(nofeat, -1)
      myPrint("Remove %d,%d\n" % (pPlot.getX(), pPlot.getY()))
   # Remove half the forest, convert half the grassland to plains
   for iPlotLoop in range(map.numPlots()):
      pPlot = map.plotByIndex(iPlotLoop)
      if pPlot.getFeatureType() == forest:
         if game.getMapRandNum(1000, "") < 500:
            pPlot.setFeatureType(nofeat, -1)
      if pPlot.getTerrainType() == grass:
         if game.getMapRandNum(1000, "") < 100:
            pPlot.setTerrainType(plains, 1, 0)
   # Convert 1% of non-ocean, non-mountain to fallout
   for iPlotLoop in range(map.numPlots()):
      pPlot = map.plotByIndex(iPlotLoop)
      if pPlot.getTerrainType() == ocean: continue
      if pPlot.isImpassable(): continue
      if makesBlockage(pPlot.getX(), pPlot.getY()): continue
      if game.getMapRandNum(1000, "") < 10:
         pPlot.setFeatureType(fall, -1)
         pPlot.setBonusType(nobonus)

### RUINS

# Find ruin center locations 4-8 plots away from all players
def findRuinCenters ():
   tundra = gc.getInfoTypeForString("TERRAIN_PARADISE")
   ice = gc.getInfoTypeForString("FEATURE_ICE")
   xmax = map.getGridWidth() ; ymax = map.getGridHeight ()
   biggestArea = map.findBiggestArea(false)
   if biggestArea is None or biggestArea.isNone():
      return [], []
   iArea = biggestArea.getID()
   ra = [] ; px = [] ; py = [] ; rx = [] ; ry = [] ; cx = [] ; cy = []
   # Create list of player locations
   for iPlay in range(gc.getMAX_CIV_PLAYERS()):
      pPlay = gc.getPlayer(iPlay)
      if (pPlay.isAlive()):
         pPlot = pPlay.getStartingPlot()
         if pPlot is not None and not pPlot.isNone():
            px.append(pPlot.getX())
            py.append(pPlot.getY())
   if len(px) == 0:
      return [], []
   # Create array, mark plots between 5-8 distance from all players
   for ix in range(xmax):
      ra.append([])
      for iy in range(ymax):
         ra[ix].append(0)
         pPlot = map.plot(ix, iy)
         # Reject if bad terrain or on island
         if pPlot.isImpassable(): continue
         if pPlot.isWater(): continue
         if pPlot.getTerrainType() == tundra: continue
         if pPlot.getFeatureType() == ice: continue
         if pPlot.getArea() != iArea: continue
         dmin = 10000
         for (x, y) in zip (px, py):
            d = max (abs (x - ix), abs (y - iy))
            if (d < dmin): dmin = d
         if (dmin > 4) and (dmin < 9):
            ra[ix][iy] = 1
            cx.append(ix) ; cy.append(iy)
   # cxy now has list of all nonzero points in ra
   while 1:
      # Pick a point; clear neighborhood; rebuild candidate list
      if len(cx) == 0: break
      n = game.getMapRandNum(len(cx), "")
      rx.append(cx[n]) ; ry.append(cy[n])
      for ix in range(cx[n]-6, cx[n]+7):
         if (ix < 0) or (ix >= xmax): continue
         for iy in range(cy[n]-6, cy[n]+7):
            if (iy < 0) or (iy >= ymax): continue
            ra[ix][iy] = 0
      cx = [] ; cy = []
      for ix in range(xmax):
         for iy in range(ymax):
            if ra[ix][iy] > 0:
               cx.append(ix) ; cy.append(iy)
   return rx, ry

# Pick a ruin off the list, add this bonus type, return shorter list
def addRuinBonus(rx, ry, usedx, usedy, type):
   if len(rx) > 0:
      n = game.getMapRandNum(len(rx), "")
      ix = rx[n] ; iy = ry[n]
      pPlot = map.plot(ix, iy)
      pPlot.setBonusType(type)
      pPlot.setRouteType(gc.getInfoTypeForString("ROUTE_HIGHWAY"))
      del rx[n] ; del ry[n]
      usedx.append(ix) ; usedy.append(iy)
   return rx, ry, usedx, usedy

# Fill in ruin structures around this center point, return list of adds
def fillRuinCenter (cx, cy):
   ruin  = gc.getInfoTypeForString("FEATURE_RUINS")
   fall  = gc.getInfoTypeForString("FEATURE_FALLOUT")
   muni  = gc.getInfoTypeForString("BONUS_ADVANCED_MUNITIONS")
   gun = gc.getInfoTypeForString("BONUS_MUNITIONS")
   junk  = gc.getInfoTypeForString("BONUS_JUNK")
   build   = gc.getInfoTypeForString("BONUS_BUILDING_MATERIALS")
   scrap  = gc.getInfoTypeForString("BONUS_SCRAP_METAL")
   tech  = gc.getInfoTypeForString("BONUS_PRE_WAR_TECH")
   can  = gc.getInfoTypeForString("BONUS_CANNED_FOOD")
   cola  = gc.getInfoTypeForString("BONUS_NUKA_COLA")
   booze  = gc.getInfoTypeForString("BONUS_PRE_WAR_BOOZE")
   goods  = gc.getInfoTypeForString("BONUS_PRE_WAR_GOODS")
   fusion  = gc.getInfoTypeForString("BONUS_FUSION_CELLS")
   road  = gc.getInfoTypeForString("ROUTE_HIGHWAY")
   nobonus = BonusTypes.NO_BONUS
   xmax = map.getGridWidth() ; ymax = map.getGridHeight ()
   rx = [] ; ry = []
   # Center, plus most of the squares around it are ruins
   for ix in range(cx-1, cx+2):
      if (ix < 0) or (ix >= xmax): continue
      for iy in range(cy-1, cy+2):
         if (iy < 0) or (iy >= ymax): continue
         type = ruin
         if (ix != cx) or (iy != cy):
            # Non-center may be empty or fallout
            n = game.getMapRandNum(1000, "")
            if n < 150: type = fall
            elif n < 600: continue
         pPlot = map.plot(ix, iy)
         if pPlot.isImpassable(): continue
         if pPlot.isWater(): continue
         pPlot.setFeatureType(type, -1)
         if type == fall:
            pPlot.setBonusType(nobonus)
         else:
            rx.append(ix) ; ry.append(iy)
            pPlot.setRouteType(road)
   usedx = [] ; usedy = []
   # One of the ruin squares probably has either a silo or airbase, never both
   n = game.getMapRandNum(1000, "")
   if n < 400:
      (rx, ry, usedx, usedy) = addRuinBonus(rx, ry, usedx, usedy, build)
   elif n < 800:
      (rx, ry, usedx, usedy) = addRuinBonus(rx, ry, usedx, usedy, scrap)
   # Could be two muni; could be one depot

   if game.getMapRandNum(1000, "") < 900:
      (rx, ry, usedx, usedy) = addRuinBonus(rx, ry, usedx, usedy, junk)
   if game.getMapRandNum(1000, "") < 700:
      (rx, ry, usedx, usedy) = addRuinBonus(rx, ry, usedx, usedy, gun)
   if game.getMapRandNum(1000, "") < 600:
      (rx, ry, usedx, usedy) = addRuinBonus(rx, ry, usedx, usedy, can)   
   if game.getMapRandNum(1000, "") < 500:
      (rx, ry, usedx, usedy) = addRuinBonus(rx, ry, usedx, usedy, goods)
   if game.getMapRandNum(1000, "") < 300:
      (rx, ry, usedx, usedy) = addRuinBonus(rx, ry, usedx, usedy, cola)
   if game.getMapRandNum(1000, "") < 200:
      (rx, ry, usedx, usedy) = addRuinBonus(rx, ry, usedx, usedy, muni)
   if game.getMapRandNum(1000, "") < 100:
      (rx, ry, usedx, usedy) = addRuinBonus(rx, ry, usedx, usedy, tech)
   return usedx, usedy

### SUBURBS

# Add a suburb at sx, sy.  Using distances in ra, draw a road back home
def addSuburbRoad(cx, cy, sx, sy, ra):
   ruin = gc.getInfoTypeForString("FEATURE_RUINS")
   road = gc.getInfoTypeForString("ROUTE_HIGHWAY")
   pPlot = map.plot(sx+cx-4, sy+cy-4)
   pPlot.setFeatureType(ruin, -1)
   pPlot.setRouteType(road)
   r = ra[sx][sy]
   while r > 0:
      found = 0
      for x1 in range (sx-1,sx+2):
         for y1 in range (sy-1,sy+2):
            if x1<0 or x1>8 or y1<0 or y1>8: continue
            if ra[x1][y1] == r - 1: found = 1
            if found: break;
         if found: break;
      if not found:
         return
      map.plot(x1+cx-4, y1+cy-4).setRouteType(road)
      sx = x1 ; sy = y1
      r = r - 1

# Given a center point and a list of ruin points, add suburbs and roads
def addSuburbsRoads (cx, cy, rxs, rys):
   ra = []
   iInvalid = 1000 ; iUnreach = 1001
   biggestArea = map.findBiggestArea(false)
   if biggestArea is None or biggestArea.isNone():
      return
   iArea = biggestArea.getID()
   # Create 9x9 local array
   for x1 in range(0,9):
      ra.append([])
      for y1 in range(0,9):
         ra[x1].append(iUnreach)
         # Set invalid if water, offgrid or impassable
         x = cx + x1 - 4 ; y = cy + y1 - 4
         if not map.isPlot(x, y):
            ra[x1][y1] = iInvalid
            continue
         pPlot = map.plot(x, y)
         # Reject if bad terrain or on island
         if pPlot.isImpassable() or pPlot.isWater(): ra[x1][y1] = iInvalid
         if pPlot.getArea() != iArea: ra[x1][y1] = iInvalid
   # Mark center and ruin points as distance 0 and add to dixy lists
   dixs = [] ; diys = [] ; dixs.append([]) ; diys.append([])
   ra[4][4] = 0 ; dixs[0].append(4) ; diys[0].append(4)
   for (x1, y1) in zip (rxs, rys):
      localX = x1-cx+4 ; localY = y1-cy+4
      if localX < 0 or localX > 8 or localY < 0 or localY > 8:
         continue
      ra[localX][localY] = 0
      dixs[0].append(localX) ; diys[0].append(localY)
   # Extend out to range 3
   for r in range (1,4):
      dixs.append([]); diys.append([]);
      for (x1, y1) in zip (dixs[r-1], diys[r-1]):
         for x2 in range (x1-1,x1+2):
            for y2 in range (y1-1,y1+2):
               if x2<0 or x2>8 or y2<0 or y2>8: continue
               if ra[x2][y2] == iUnreach:
                  ra[x2][y2] = r
                  dixs[r].append(x2) ; diys[r].append(y2)
   # Pick a small number of candidates
   candx = dixs[2] + dixs[3] ; candy = diys[2] + diys[3]
   x = game.getMapRandNum(1000, "")
   if x < 200: numcands = 0
   elif x < 500: numcands = 1
   elif x < 900: numcands = 2
   else: numcands = 3
   for n in range(numcands):
      if len(candx) == 0: break
      posn = game.getMapRandNum(len(candx), "")
      x = candx[posn] ; y = candy[posn]
      addSuburbRoad(cx, cy, x, y, ra)
      del candx[posn] ; del candy[posn]

### HIGHWAYS

# Sort list of dist, int, int from closest to farthest
def highwaySort(l1, l2):
   d1 = l1[0] ; d2 = l2[0]
   if d1 > d2: return 1
   elif d1 < d2: return -1
   else: return 0

# Connect these two cities
def addHighway(x1, y1, x2, y2):
   biggestArea = map.findBiggestArea(false)
   if biggestArea is None or biggestArea.isNone() or not map.isPlot(x1, y1) or not map.isPlot(x2, y2):
      return false
   iArea = biggestArea.getID()
   iInvalid = 1000 ; iUnreach = 1001
   ra = [] 
   xmax = map.getGridWidth() ; ymax = map.getGridHeight ()
   # Build array with unreachable except where invalid
   for x in range(xmax):
      ra.append([])
      for y in range(ymax):
         ra[x].append(iUnreach)
         pPlot = map.plot(x, y)
         if pPlot.isImpassable() or pPlot.isWater(): ra[x][y] = iInvalid
         if pPlot.getArea() != iArea: ra[x][y] = iInvalid
   # Extend out to range 15 or we arrive at destination
   oldx = [x1] ; oldy = [y1] ; ra[x1][y1] = 0
   bFound = false
   for r in range (1,16):
      newx = [] ; newy = []
      for (x, y) in zip (oldx, oldy):
         for xc in range (x-1,x+2):
            for yc in range (y-1,y+2):
               if not map.isPlot(xc, yc): continue
               if ra[xc][yc] == iUnreach:
                  ra[xc][yc] = r
                  if (xc == x2) and (yc == y2):
                     bFound = true
                     break
                  newx.append(xc) ; newy.append(yc)
            if bFound: break
         if bFound: break
      if bFound:
         break
      oldx = newx ; oldy = newy
   if not bFound: return false
   # Drive back from x2, y2 along decreasing cost path.  At each
   # point break tie by shortest *geometrical* distance to origin
   road  = gc.getInfoTypeForString("ROUTE_HIGHWAY")
   r = ra[x2][y2] ; sx = x2 ; sy = y2
   while r > 0:
      cands = []
      for xc in range (sx-1,sx+2):
         for yc in range (sy-1,sy+2):
            if not map.isPlot(xc, yc): continue
            if ra[xc][yc] == r - 1:
               dx = xc - x1 ; dy = yc - y1
               cands.append([(dx*dx)+(dy*dy), xc, yc])
      if len(cands) == 0:
         return false
      candsort = sorted(cands, highwaySort)
      xc = candsort[0][1] ; yc = candsort[0][2]
      map.plot(xc, yc).setRouteType(road)
      sx = xc ; sy = yc
      r = r - 1
   return true

# Given list of ruin city centers, connect with roads
def addHighways(rx, ry):
   dists = [] ; used = []
   # Make list of (distance, index1, index2) for each pair of cities
   for i in range(len(rx)-1):
      for j in range(i+1, len(rx)):
         dx = abs (rx[i] - rx[j]) ; dy = abs (ry[i] - ry[j])
         if dy > dx: dx = dy
         dists.append([dx,i,j])
   # Make sure each city is connected once
   for i in range(len(rx)): used.append(false)
   for d12 in sorted(dists, highwaySort):
      i = d12[1] ; j = d12[2]
      if (used[i]) and (used[j]): continue
      if addHighway(rx[i], ry[i], rx[j], ry[j]):
         used[i] = true ; used[j] = true

# Delete 30% of highways so players must repair them
def blowRoads():
   road  = gc.getInfoTypeForString("ROUTE_HIGHWAY")
   for iPlotLoop in range(map.numPlots()):
      pPlot = map.plotByIndex(iPlotLoop)
      if pPlot.getRouteType() != road: continue
      if game.getMapRandNum(1000, "") < 300:
         pPlot.setRouteType(-1)

# Delete goody huts and fallout which are adjacent to start locations
def blowHutsFall():
   iGoody = gc.getInfoTypeForString("IMPROVEMENT_SURVIVOR_VILLAGE")
   iFall  = gc.getInfoTypeForString("FEATURE_FALLOUT")
   noimpr = ImprovementTypes.NO_IMPROVEMENT
   nofeat = FeatureTypes.NO_FEATURE
   xmax = map.getGridWidth() ; ymax = map.getGridHeight ()
   for iPlay in range(gc.getMAX_CIV_PLAYERS()):
      pPlay = gc.getPlayer(iPlay)
      if (pPlay.isAlive()):
         pPlot = pPlay.getStartingPlot()
         if pPlot is None or pPlot.isNone():
            continue
         cx = pPlot.getX() ; cy = pPlot.getY()
         for ix in range(cx-1, cx+2):
            if (ix < 0) or (ix >= xmax): continue
            for iy in range(cy-1, cy+2):
               if (iy < 0) or (iy >= ymax): continue
               pPlot = map.plot(ix, iy)
               if pPlot.getImprovementType() == iGoody:
                  pPlot.setImprovementType(noimpr)
               if pPlot.getFeatureType() == iFall:
                  pPlot.setFeatureType(nofeat, -1)

### TOP LEVEL ENTRY POINTS

# Find low food start points, add one bonus
def fixFood():
   grass  = gc.getInfoTypeForString("TERRAIN_FERTILE_WASTELAND")
   coast  = gc.getInfoTypeForString("TERRAIN_COAST")
   fall   = gc.getInfoTypeForString("FEATURE_FALLOUT")
   wheat  = gc.getInfoTypeForString("BONUS_CABBAGE")
   corn   = gc.getInfoTypeForString("BONUS_CORN")
   cow    = gc.getInfoTypeForString("BONUS_BRAHMIN")
   pig    = gc.getInfoTypeForString("BONUS_IGUANA")
   xmax = map.getGridWidth() ; ymax = map.getGridHeight ()
   landfix = [wheat, corn, cow, pig]
   for iPlay in range(gc.getMAX_CIV_PLAYERS()):
      pPlay = gc.getPlayer(iPlay)
      possfix = []
      if (pPlay.isAlive()):
         pPlot = pPlay.getStartingPlot()
         if pPlot is None or pPlot.isNone():
            continue
         cx = pPlot.getX() ; cy = pPlot.getY()
         lBest = [] ; lFix = []
         for ix in range(cx-2, cx+3):
            if (ix < 0) or (ix >= xmax): continue
            for iy in range(cy-2, cy+3):
               if (iy < 0) or (iy >= ymax): continue
               if (cx == ix) and (cy == iy): continue
               if (abs(cx-ix) == 2) and (abs(cy-iy) == 2): continue
               pPlot = map.plot(ix, iy)
               if pPlot.getFeatureType() == fall: continue
               terr = pPlot.getTerrainType()
               bonus = pPlot.getBonusType(-1)
               if (bonus == wheat) or (bonus == corn):  lBest.append(3)
               elif (bonus == cow) or (bonus == pig):  lBest.append(3)
               else:
                  # Possible fix spot
                  if terr == grass and not pPlot.isHills():
                     if pPlot.isFreshWater(): lBest.append(1)
                     fix = landfix[game.getMapRandNum(len(landfix), "")]
                     lFix.append([ix, iy, fix])
         lBest = sorted(lBest)
         top = len(lBest)
         if top > 4: top = 4
         extra = 0
         for i in range(top): extra += lBest[i]
         myPrint("Player %d start %d,%d: extra %d\n" % (iPlay, cx, cy, extra))
         while extra < 4:
            if (len(lFix) == 0):
               myPrint("  No fix locations!")
               break
            else:
               posn = game.getMapRandNum(len(lFix), "")
               (ix, iy, fix) = lFix[posn]
               del lFix[posn]
               myPrint("   Fix with %d at %d,%d\n" % (fix, ix, iy))
               pPlot = map.plot(ix, iy)
               pPlot.setBonusType(fix)
               pPlot.setFeatureType(FeatureTypes.NO_FEATURE, -1)
               extra += 3

# My main entry point to add new things
def normalizeAddExtras():
   rx, ry = findRuinCenters()
   for (ix, iy) in zip (rx, ry):
      (usedx, usedy) = fillRuinCenter (ix, iy)
      addSuburbsRoads(ix, iy, usedx, usedy)
   addHighways(rx, ry)
   blowRoads()
   blowHutsFall()
   fixFood()

# Prevent starting on bad terrain types
def findStartingPlot(argsList):
   [playerID] = argsList
   player = gc.getPlayer(playerID)
   biggestArea = map.findBiggestArea(False)
   if biggestArea is None or biggestArea.isNone(): return -1

   # Never let two civs receive the same tile. The normal found-value score
   # discourages crowding, but is only a penalty and not a hard reservation.
   aOtherStarts = []
   for iPlay in range(gc.getMAX_CIV_PLAYERS()):
      if iPlay == playerID: continue
      pPlay = gc.getPlayer(iPlay)
      if not pPlay.isAlive(): continue
      pStart = pPlay.getStartingPlot()
      if pStart is None or pStart.isNone(): continue
      aOtherStarts.append((pStart.getX(), pStart.getY()))

   tundra = gc.getInfoTypeForString("TERRAIN_DECAYED_URBAN_WASTELAND")
   desert = gc.getInfoTypeForString("TERRAIN_WASTELAND")
   snow = gc.getInfoTypeForString("TERRAIN_PARADISE")
   fallout = gc.getInfoTypeForString("FEATURE_FALLOUT")
   ice = gc.getInfoTypeForString("FEATURE_ICE")
   iPreferredDistance = max(1, player.startingPlotRange())
   aiMinDistances = []
   for iDistance in (iPreferredDistance, max(1, (3 * iPreferredDistance) / 4),
         max(1, iPreferredDistance / 2), 1):
      if iDistance not in aiMinDistances:
         aiMinDistances.append(iDistance)

   def findAtDistance(iMinDistance, bAvoidPoorTerrain):
      def isValid(playerID, x, y):
         pPlot = map.plot(x, y)
         if pPlot.getArea() != biggestArea.getID(): return false
         if pPlot.isWater() or pPlot.isImpassable(): return false
         if bAvoidPoorTerrain and pPlot.getTerrainType() in (tundra, desert, snow):
            return false
         if pPlot.getFeatureType() == fallout or pPlot.getFeatureType() == ice:
            return false
         for (iStartX, iStartY) in aOtherStarts:
            if plotDistance(x, y, iStartX, iStartY) < iMinDistance:
               return false
         return true
      return CvMapGeneratorUtil.findStartingPlot(playerID, isValid)

   # Try the engine's preferred spacing first, then relax it for crowded maps.
   # The final pass still forbids occupied tiles; only the poor-terrain filter
   # is relaxed if the map has too few otherwise suitable locations.
   for iMinDistance in aiMinDistances:
      iStartPlot = findAtDistance(iMinDistance, true)
      if iStartPlot >= 0: return iStartPlot
   return findAtDistance(1, false)

# Stub out routines which make the start position less bleak
def normalizeRemoveBadFeatures(): return
def normalizeRemoveBadTerrain(): return
def normalizeAddFoodBonuses(): return
def normalizeAddGoodTerrain(): return
