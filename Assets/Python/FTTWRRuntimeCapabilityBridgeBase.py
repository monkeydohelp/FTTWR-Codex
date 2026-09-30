# ACBase Test candidate-only FTTWR P1 manpower bridge.
# Python 2.4-compatible: do not import json or use newer syntax.
#
# The bridge is inert in ordinary games.  It only emits marker telemetry (and
# seeds the deliberately synthetic source fixture) when the Runtime Lab sets
# both FTTWR_CAPABILITY_RUNTIME=1 and FTTW_DEVKIT_MARKER.
import os

from CvPythonExtensions import CyGlobalContext, CyGame

gc = CyGlobalContext()
_IDENTITY_WRITTEN = False
_SEEN = {}
_FIXTURE_DONE = False


def _enabled():
	return os.environ.get("FTTWR_CAPABILITY_RUNTIME") == "1" and bool(os.environ.get("FTTW_DEVKIT_MARKER", ""))


def _write(line):
	path = os.environ.get("FTTW_DEVKIT_MARKER", "")
	if not path:
		return
	try:
		stream = open(path, "a")
		try:
			stream.write(line + "\n")
		finally:
			stream.close()
	except Exception:
		# Diagnostics must never make a game callback fail.
		pass


def _escape(value):
	try:
		if isinstance(value, unicode):
			value = value.encode("utf-8")
	except NameError:
		pass
	try:
		value = str(value)
	except Exception:
		value = "<unprintable>"
	return value.replace("\\", "\\\\").replace('"', '\\"').replace("\r", "\\r").replace("\n", "\\n")


def _value(value):
	if value is None:
		return "null"
	if value is True:
		return "true"
	if value is False:
		return "false"
	if isinstance(value, (int, long)):
		return str(value)
	return '"' + _escape(value) + '"'


def _object(fields):
	items = []
	for key in sorted(fields.keys()):
		items.append('"' + _escape(key) + '":' + _value(fields[key]))
	return "{" + ",".join(items) + "}"


def _mark(event, fields):
	data = {"event": event, "run_id": os.environ.get("FTTW_DEVKIT_RUN_ID", "")}
	data.update(fields)
	_write(_object(data))


def _identity():
	global _IDENTITY_WRITTEN
	if _IDENTITY_WRITTEN:
		return
	_IDENTITY_WRITTEN = True
	_mark("FTTWR_RUNTIME_IDENTITY", {
		"hook_version": 1,
		"bridge_version": "acbase-test-recruit-queue-v1",
		"contract_schema": "fttwr-recruit-capacity-p1-0.1",
		"mapping_version": "p1-manpower-v3",
		"formula_version": "connected-tier-copy-count-plus-native-commit-v3",
		"candidate_mod": os.environ.get("FTTW_DEVKIT_MOD", ""),
		"native_api": "getResurrectionManpowerDescriptor",
		"queue_integration": "native-combined-ledger-reservation-v1",
		"process_id": os.getpid(),
		"executable": "Civ4BeyondSword.exe",
	})


def _descriptor(city):
	try:
		# The canonical method is the only normal path.  The compatibility name
		# is retained for an older CyCity overlay, but is used only if the
		# canonical wrapper is not present.
		method = getattr(city, "getResurrectionManpowerDescriptor", None)
		if method is None:
			method = getattr(city, "getResurrectionManpowerCapabilityDescriptor", None)
		if method is None:
			return "missing-api", "{}"
		value = method()
		if value is None:
			return "error", "{}"
		return "ok", value
	except Exception:
		return "error", "{}"


def _row(city, player_id):
	status, value = _descriptor(city)
	return _object({
		"player_id": player_id,
		"city_id": city.getID(),
		"city_name": city.getName(),
		"consumer": "native-api",
		"api_status": status,
		"descriptor": value,
	})


def _seed_fixture():
	global _FIXTURE_DONE
	if _FIXTURE_DONE or os.environ.get("RESURRECTION_P1_MANPOWER_FIXTURE") != "1":
		return
	_FIXTURE_DONE = True
	try:
		bonus = gc.getInfoTypeForString("BONUS_RECRUITS")
		mine = gc.getInfoTypeForString("IMPROVEMENT_MINE")
		route = gc.getInfoTypeForString("ROUTE_ROAD")
		if bonus < 0 or mine < 0:
			_mark("FTTWR_MANPOWER_FIXTURE", {"status": "MISSING_INFO", "bonus_id": bonus, "mine_id": mine})
			return
		disconnected = os.environ.get("RESURRECTION_P1_MANPOWER_DISCONNECTED") == "1"
		for player_id in xrange(gc.getMAX_PLAYERS()):
			player = gc.getPlayer(player_id)
			if not player.isAlive() or not player.isHuman():
				continue
			if player.getNumCities() <= 0:
				try:
					start = player.getStartingPlot()
					if start is not None:
						player.initCity(start.getX(), start.getY())
				except Exception:
					pass
			if player.getNumCities() <= 0:
				_mark("FTTWR_MANPOWER_FIXTURE", {"status": "NO_CITY", "player_id": player_id})
				return
			city = player.getCity(0)
			center = city.plot()
			adjacent = gc.getMap().plot(center.getX() + 1, center.getY())
			if adjacent is None:
				adjacent = gc.getMap().plot(center.getX() - 1, center.getY())
			if adjacent is None:
				_mark("FTTWR_MANPOWER_FIXTURE", {"status": "NO_ADJACENT_PLOT", "player_id": player_id})
				return
			owner = city.getOwner()
			center.setOwner(owner)
			center.setBonusType(bonus)
			adjacent.setOwner(owner)
			adjacent.setBonusType(bonus)
			if not disconnected:
				adjacent.setImprovementType(mine)
				if route >= 0:
					adjacent.setRouteType(route)
			mode = "connected"
			if disconnected:
				mode = "disconnected"
			_mark("FTTWR_MANPOWER_FIXTURE", {
				"status": "PASS",
				"mode": mode,
				"player_id": player_id,
				"city_id": city.getID(),
				"copies_placed": 2,
			})
			return
		_mark("FTTWR_MANPOWER_FIXTURE", {"status": "NO_HUMAN"})
	except Exception:
		_mark("FTTWR_MANPOWER_FIXTURE", {"status": "ERROR"})


def _snapshot(phase):
	if not _enabled():
		return
	_identity()
	try:
		turn = CyGame().getGameTurn()
	except Exception:
		turn = -1
	key = (phase, turn)
	if _SEEN.get(key):
		return
	_SEEN[key] = True
	native_rows = []
	for player_id in xrange(gc.getMAX_PLAYERS()):
		player = gc.getPlayer(player_id)
		if not player.isAlive() or not player.isHuman():
			continue
		city, iterator = player.firstCity(False)
		while city and not city.isNone():
			native_rows.append(_row(city, player_id))
			city, iterator = player.nextCity(iterator, False)
	base = {
		"run_id": os.environ.get("FTTW_DEVKIT_RUN_ID", ""),
		"phase": phase,
		"game_turn": turn,
		"seed": "%s:%s" % (phase, turn),
		"city_count": len(native_rows),
		"parity_status": "deferred_no_independent_projection",
		"parity_reason": "The mechanics row is a normalized consumer of the one native descriptor read; no second bridge call is treated as independent parity.",
		"python_row_count": 0,
	}
	data = dict(base)
	data["event"] = "FTTWR_NATIVE_MANPOWER_CAPABILITY"
	text = _object(data)
	_write(text[:-1] + ',"rows":[' + ",".join(native_rows) + "]}")
	data["event"] = "FTTWR_PYTHON_MANPOWER_CAPABILITY"
	text = _object(data)
	_write(text[:-1] + ',"rows":[]}')


def _inprocess_probe():
	if os.environ.get("FTTWR_CAPABILITY_INPROCESS_PROBE") != "1" or not _enabled():
		return
	_seed_fixture()
	_snapshot("GameStartProbe")
	_mark("FTTWR_INPROCESS_CAPABILITY_PROBE", {
		"status": "PASS",
		"phase": "GameStartProbe",
		"synthetic": True,
		"input_policy": "disabled",
		"promotion_eligible": False,
	})


def on_game_start():
	_seed_fixture()
	_snapshot("GameStart")


def on_load_game():
	_inprocess_probe()
	_snapshot("save-reload")


def on_end_player_turn(game_turn, player_id):
	_snapshot("turn-boundary")
