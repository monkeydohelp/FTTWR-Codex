# advc.009b: New module for initialization/ reloading of modified BtS screens

# Calling these CvScreensInterface functions directly through BugUtil.lookupFunction doesn't work reliably (race condition) when reloading scripts. I don't understand why; perhaps some circular dependency.
# Queue screen initialization until BUG has parsed its configuration. During a
# Python reload, the screen module can still be in the middle of importing.
# BUG - Options - start  (moved from CvScreensInterface.py)
g_bBugAdvisorsInitialized = False

def _initBugAdvisors():
	global g_bBugAdvisorsInitialized
	import CvScreensInterface
	try:
		initFunction = CvScreensInterface.initBugAdvisors
	except AttributeError:
		return False
	initFunction()
	g_bBugAdvisorsInitialized = True
	return True

def init(): # called when parsing 'BUG Core.xml'
	import BugInit
	BugInit.addInit("ScreenInit", _initBugAdvisors)

def retry():
	if g_bBugAdvisorsInitialized:
		return
	try:
		bInitialized = _initBugAdvisors()
	except:
		import BugUtil
		BugUtil.error("Failed to initialize BUG advisors after reload")
		BugUtil.trace("BUG advisor initialization traceback")
		return
	if not bInitialized:
		import BugUtil
		BugUtil.error("CvScreensInterface.initBugAdvisors is unavailable after reload")
# BUG - Options - end

def deleteTechSplash(option=None, value=None): # called when parsing TechWindow.xml
	import CvScreensInterface
	CvScreensInterface.deleteTechSplash(option, value)
