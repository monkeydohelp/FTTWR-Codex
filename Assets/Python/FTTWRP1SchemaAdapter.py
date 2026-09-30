"""Python 2.4-compatible adapter for the native P1 manpower descriptor.

The native descriptor remains authoritative.  This module only transports its
versioned fields into a flat shape that the AI/UI-facing bridge can consume.
"""

ADAPTER_VERSION = "p1-manpower-flat-v2"
RAW_SCHEMA = "p1-manpower-v1-nested"
QUALITY_ORDER = ("BASIC", "ADVANCED", "VETERAN")


def _as_text(value):
	try:
		return unicode(value)
	except NameError:
		return str(value)


def _escape(value):
	text = _as_text(value)
	text = text.replace(u"\\", u"\\\\")
	text = text.replace(u'"', u'\\"')
	text = text.replace(u"\r", u"\\r")
	text = text.replace(u"\n", u"\\n")
	text = text.replace(u"\t", u"\\t")
	return text


def _int_after(raw, marker, start=0):
	if raw is None:
		return 0
	index = raw.find(marker, start)
	if index < 0:
		return 0
	index += len(marker)
	length = len(raw)
	while index < length and raw[index] in " \t\r\n\":":
		index += 1
	sign = 1
	if index < length and raw[index] == "-":
		sign = -1
		index += 1
	begin = index
	while index < length and raw[index] >= "0" and raw[index] <= "9":
		index += 1
	if begin == index:
		return 0
	try:
		return sign * int(raw[begin:index])
	except Exception:
		return 0


def _section_value(raw, section, field):
	if raw is None:
		return 0
	section_marker = '"' + section + '":{'
	section_start = raw.find(section_marker)
	if section_start < 0:
		return 0
	field_marker = '"' + field + '":'
	field_start = raw.find(field_marker, section_start + len(section_marker))
	if field_start < 0:
		return 0
	section_end = raw.find("}", section_start + len(section_marker))
	if section_end >= 0 and field_start > section_end:
		return 0
	return _int_after(raw, field_marker, field_start)


def _quality_map(raw, field):
	values = {}
	for quality in QUALITY_ORDER:
		values[quality] = _section_value(raw, quality.lower(), field)
	return values


def _quality_total(values):
	total = 0
	for quality in QUALITY_ORDER:
		total += int(values.get(quality, 0))
	return total


def _robot_map(raw):
	return {
		"sources": _section_value(raw, "robot", "sources"),
		"occupancy": _section_value(raw, "robot", "occupancy"),
		"available": _section_value(raw, "robot", "available"),
	}


def _population(city):
	# Population eligibility is native-owned.  The fallback is solely for an
	# older DLL so a stale candidate reports its provenance instead of failing.
	try:
		return int(city.getFTTWRPopulationAvailable()), "CyCity.getFTTWRPopulationAvailable"
	except Exception:
		try:
			return int(city.getPopulation()), "CyCity.getPopulation-fallback"
		except Exception:
			return 0, "unavailable"


def _city_id(raw, city):
	value = _int_after(raw, '"city_id":')
	if value:
		return value
	try:
		return int(city.getID())
	except Exception:
		return 0


def _owner_id(raw, player_id):
	value = _int_after(raw, '"player_id":')
	if value:
		return value
	try:
		return int(player_id)
	except Exception:
		return 0


def adapt_descriptor(raw_descriptor, city, player_id):
	"""Flatten the native descriptor while retaining its exact raw payload."""
	raw = _as_text(raw_descriptor or "{}")
	source_values = _quality_map(raw, "sources")
	occupancy_values = _quality_map(raw, "occupancy")
	available_values = _quality_map(raw, "available")
	robot = _robot_map(raw)
	network_component_id = _int_after(raw, '"network_component_id":')
	population_available, population_source = _population(city)
	return (
		"{"
		+ '"adapter_version":"' + _escape(ADAPTER_VERSION) + '",'
		+ '"raw_schema":"' + _escape(RAW_SCHEMA) + '",'
		+ '"owner_player_id":' + str(_owner_id(raw, player_id)) + ","
		+ '"city_id":' + str(_city_id(raw, city)) + ","
		+ '"network_component_id":' + str(network_component_id) + ","
		+ '"human_sources":' + str(_quality_total(source_values)) + ","
		+ '"human_occupancy":' + str(_quality_total(occupancy_values)) + ","
		+ '"human_available":' + str(_quality_total(available_values)) + ","
		+ '"human_basic_sources":' + str(source_values["BASIC"]) + ","
		+ '"human_basic_occupancy":' + str(occupancy_values["BASIC"]) + ","
		+ '"human_basic_available":' + str(available_values["BASIC"]) + ","
		+ '"human_advanced_sources":' + str(source_values["ADVANCED"]) + ","
		+ '"human_advanced_occupancy":' + str(occupancy_values["ADVANCED"]) + ","
		+ '"human_advanced_available":' + str(available_values["ADVANCED"]) + ","
		+ '"human_veteran_sources":' + str(source_values["VETERAN"]) + ","
		+ '"human_veteran_occupancy":' + str(occupancy_values["VETERAN"]) + ","
		+ '"human_veteran_available":' + str(available_values["VETERAN"]) + ","
		+ '"robot_sources":' + str(robot["sources"]) + ","
		+ '"robot_occupancy":' + str(robot["occupancy"]) + ","
		+ '"robot_available":' + str(robot["available"]) + ","
		+ '"recruit_bonus_identity":"BONUS_RECRUITS",'
		+ '"human_population_available":' + str(population_available) + ","
		+ '"population_source":"' + _escape(population_source) + '",'
		+ '"raw_native_descriptor":"' + _escape(raw) + '"'
		+ "}"
	)
