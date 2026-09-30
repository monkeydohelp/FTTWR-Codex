# Optional, test-only Civ4 map-run instrumentation.
#
# The module is inert unless FTTW_DEVKIT_MARKER is present in the process
# environment. It is intentionally Python 2-compatible because Civ4 BtS
# embeds Python 2.4.
import os

try:
	_integer_types = (int, long)
except NameError:
	_integer_types = (int,)


def _json_string(value):
	text = str(value)
	text = text.replace("\\", "\\\\")
	text = text.replace('"', '\\"')
	text = text.replace("\r", "\\r")
	text = text.replace("\n", "\\n")
	return '"' + text + '"'


def _json_value(value):
	if value is None:
		return "null"
	if value is True:
		return "true"
	if value is False:
		return "false"
	if isinstance(value, _integer_types):
		return str(value)
	return _json_string(value)


def _json_dumps(data):
	items = []
	for key in sorted(data.keys()):
		items.append(_json_string(key) + ":" + _json_value(data[key]))
	return "{" + ",".join(items) + "}"



def _path():
	return os.environ.get("FTTW_DEVKIT_MARKER")


def enabled():
	return bool(_path())


def mark(event, **fields):
	path = _path()
	if not path:
		return
	data = {"event": event, "run_id": os.environ.get("FTTW_DEVKIT_RUN_ID", ""), "mod": os.environ.get("FTTW_DEVKIT_MOD", ""), "dll": os.environ.get("FTTW_DEVKIT_DLL", "")}
	data.update(fields)
	try:
		directory = os.path.dirname(path)
		if directory and not os.path.isdir(directory):
			os.makedirs(directory)
		stream = open(path, "a")
		try:
			stream.write(_json_dumps(data) + "\n")
		finally:
			stream.close()
	except Exception:
		# Instrumentation must never break map generation.
		return


def start(map_name):
	if enabled():
		mark("RUN_START", map=map_name, size=os.environ.get("FTTW_DEVKIT_SIZE", ""), seed=os.environ.get("FTTW_DEVKIT_SEED", ""))
		mark("MAP_SELECTED", map=map_name, size=os.environ.get("FTTW_DEVKIT_SIZE", ""), seed=os.environ.get("FTTW_DEVKIT_SEED", ""))
