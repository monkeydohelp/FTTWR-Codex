# ACBase Test DPF status panel.
#
# This module observes the native CyPlayer DPF query surface and renders a
# compact, resolution-aware HUD panel.  Pending choices are submitted only
# through the authoritative CyPlayer dpfSubmitChoice command; Python never
# mutates DPF state directly.  Python 2.4-compatible: avoid newer syntax and
# libraries.

from CvPythonExtensions import *
import CvUtil
import CvScreenEnums

try:
    import DPFPlayerContent
except Exception:
    DPFPlayerContent = None


gc = CyGlobalContext()

_PANEL = "DPFStatusPanel"
_TOGGLE_PANEL = "DPFStatusTogglePanel"
_TOGGLE = "DPFStatusToggle"
_TOGGLE_HIT = "DPFStatusToggleHit"
_TOGGLE_LABEL = "DPFStatusToggleLabel"
_TOGGLE_ART = "INTERFACE_GENERAL_MENU_ICON"
# Stable IDs follow Civ4 documented Python action-button routing.
_TOGGLE_DATA1 = 9471
_TOGGLE_DATA2 = -1
_CHOICE_PREFIX = "DPFStatusChoice"
_CHOICE_DATA1 = 9472
_MAX_CHOICE_BUTTONS = 8
_DPF_WIDGET_TYPE = None

# ACBase Test reward variables for the first choice/effect vertical slice.
# They are queried read-only; native DPF remains authoritative.
_DPF_REWARD_DEFINITION = 1
_DPF_VAR_NETWORK_CAPACITY = 2
_DPF_VAR_PROTECTION_CHARGES = 3
_DPF_VAR_NETWORK_EXPOSED = 4

def _choice_widget_name(index):
    return _CHOICE_PREFIX + str(index + 1)

def getWidgetHelp(argsList):
    """Return hover help for the DPF toggle and pending-choice controls."""
    try:
        eWidgetType, iData1, iData2, bOption = argsList
        if (_is_dpf_widget_type(eWidgetType) and
                iData1 == _TOGGLE_DATA1 and iData2 == _TOGGLE_DATA2):
            return u"Open or close DPF status"
        if (_is_dpf_widget_type(eWidgetType) and
                iData1 == _CHOICE_DATA1 and int(iData2) >= 0):
            try:
                active_player = int(CyGame().getActivePlayer())
                if active_player >= 0:
                    player = gc.getPlayer(active_player)
                    name = _safe_call(player,
                        "dpfGetPendingChoiceOptionName", (int(iData2),), "")
                    description = _safe_call(player,
                        "dpfGetPendingChoiceOptionDescription", (int(iData2),), "")
                    if name and description:
                        return u"%s: %s" % (_as_unicode(name),
                            _as_unicode(description))
                    if name:
                        return _as_unicode(name)
            except Exception:
                pass
            return u"Choose this DPF option"
    except Exception:
        pass
    return u""

def _widget_help_callback(eWidgetType, iData1, iData2, bOption):
    """Adapt WidgetUtil's four-argument callback contract to getWidgetHelp."""
    return getWidgetHelp((eWidgetType, iData1, iData2, bOption))

def _is_dpf_widget_type(widget):
    """Accept the registered DPF widget and the safe legacy fallback."""
    if widget == WidgetTypes.WIDGET_PYTHON:
        return True
    try:
        return widget == getattr(WidgetTypes, "WIDGET_DPF_STATUS")
    except Exception:
        return False

def _dpf_widget_type():
    """Get one custom WidgetTypes value and register its native help callback.

    Civ4's widget-help dispatcher only invokes dynamic callbacks for a
    registered WidgetTypes value. The DPF module is imported before BUG's
    configuration pass, so registration is lazy and happens when the HUD is
    first built. If an older runtime cannot load WidgetUtil, the documented
    WIDGET_PYTHON route remains a safe compatibility fallback.
    """
    global _DPF_WIDGET_TYPE
    if _DPF_WIDGET_TYPE is not None:
        return _DPF_WIDGET_TYPE
    try:
        import WidgetUtil
        if hasattr(WidgetTypes, "WIDGET_DPF_STATUS"):
            widget = getattr(WidgetTypes, "WIDGET_DPF_STATUS")
        else:
            widget = WidgetUtil.createWidget("WIDGET_DPF_STATUS")
        WidgetUtil.setWidgetHelpFunction(widget, _widget_help_callback)
        _DPF_WIDGET_TYPE = widget
        return widget
    except Exception:
        return WidgetTypes.WIDGET_PYTHON
_TITLE = "DPFStatusTitle"
_SUMMARY = "DPFStatusSummary"
_CHOICE_STATUS = 5
_ROW_PREFIX = "DPFStatusRow"
_MAX_VISIBLE_ROWS = 4

_STATUS_NAMES = {
    0: "DORMANT",
    1: "ACTIVE",
    2: "BLOCKED",
    3: "FAILED",
    4: "COMPLETED",
    5: "WAITING_FOR_CHOICE",
}


def _display_content(definition_id, stage_id, status, player=None):
    """Resolve native XML presentation first, then compatibility labels.

    The native query surface is read-only. Python supplies labels only when an
    older candidate DLL does not expose the optional presentation methods.
    """
    content = None
    if DPFPlayerContent is not None:
        try:
            content = DPFPlayerContent.get_progression(definition_id)
        except Exception:
            content = None

    fallback = (u"Progression", _STATUS_NAMES.get(status, u"In progress"),
                u"Follow the next available action.", u"Improvements",
                u"Features lost")
    if content is not None:
        stage_info = content.get("stages", {}).get(stage_id, None)
        if stage_info is None:
            stage_info = {}
        fallback = (content.get("name", u"Progression"),
                    stage_info.get("status",
                        _STATUS_NAMES.get(status, u"In progress")),
                    stage_info.get("hint",
                        u"Follow the next available action."),
                    content.get("event_1", u"Events"),
                    content.get("event_2", u"Setbacks"))

    if player is not None:
        try:
            native_name = player.dpfGetInstanceDisplayName(definition_id)
        except Exception:
            native_name = ""
        try:
            native_status = player.dpfGetInstanceStatusName(definition_id)
        except Exception:
            native_status = ""
        try:
            native_stage = player.dpfGetInstanceStageName(definition_id)
        except Exception:
            native_stage = ""
        try:
            native_hint = player.dpfGetInstanceNextHint(definition_id)
        except Exception:
            native_hint = ""
        if native_name or native_status or native_stage or native_hint:
            native_fixture_name = _starts_with(native_name, u"DPF_TEST_")
            native_fixture_stage = _starts_with(native_stage, u"Stage ")
            native_fixture_hint = _starts_with(native_hint,
                u"Complete the first required action")
            if content is not None:
                # The fallback DLL fixtures expose internal test names and a
                # generic hint while the optional XML presentation fields are
                # unavailable. Keep native state/counters, but show the named
                # player content rather than leaking implementation IDs.
                resolved_name = fallback[0]
                if native_name and not native_fixture_name:
                    resolved_name = native_name
                resolved_stage = fallback[1]
                if native_stage and not native_fixture_stage:
                    resolved_stage = native_stage
                elif native_status and not native_stage:
                    resolved_stage = native_status
                if status == _CHOICE_STATUS and native_status:
                    resolved_stage = native_status
                resolved_hint = fallback[2]
                if native_hint and not native_fixture_hint:
                    resolved_hint = native_hint
                return (resolved_name, resolved_stage, resolved_hint,
                        fallback[3], fallback[4])
            resolved_status = native_stage or native_status or fallback[1]
            if status == _CHOICE_STATUS and native_status:
                resolved_status = native_status
            return (native_name or fallback[0],
                    resolved_status, native_hint or fallback[2], fallback[3], fallback[4])
    return fallback


def _safe_call(obj, name, args=(), default=None):
    try:
        method = getattr(obj, name)
    except Exception:
        return default
    try:
        return method(*args)
    except Exception:
        return default


def _int_value(value, default=None):
    try:
        return int(value)
    except Exception:
        return default


def _as_unicode(value):
    """Convert engine/Python values without requiring modern Python features."""
    try:
        if isinstance(value, unicode):
            return value
    except Exception:
        pass
    try:
        return unicode(value)
    except Exception:
        try:
            return str(value)
        except Exception:
            return u""


def _starts_with(value, prefix):
    try:
        return bool(value) and value.startswith(prefix)
    except Exception:
        return False


def _clip_text(text, max_chars):
    """Keep a HUD string inside its approximate pixel budget."""
    text = _as_unicode(text)
    try:
        limit = int(max_chars)
    except Exception:
        limit = 0
    if limit <= 0:
        return u""
    if len(text) <= limit:
        return text
    if limit <= 3:
        return text[:limit]
    clipped = text[:limit - 3]
    split = clipped.rfind(u" ")
    if split >= max(8, limit // 2):
        clipped = clipped[:split]
    return clipped.rstrip() + u"..."


def _event_label(name, default):
    label = _as_unicode(name)
    if not label:
        label = default
    lowered = label.lower()
    if lowered == u"improvements":
        return u"Imp."
    if lowered == u"features lost":
        return u"Lost"
    return label


def _value_char_limit(panel_width):
    try:
        width = int(panel_width)
    except Exception:
        width = 520
    # SMALL_FONT is approximately ten pixels per character at the supported
    # UI scales. Keep a little room for the left/right panel insets.
    return max(44, int((width - 42) / 10))


def _format_value_text(hint, event_1_name, event_1_value, event_1_delta,
        event_2_name, event_2_value, event_2_delta, max_chars,
        reward_text=u""):
    first_label = _event_label(event_1_name, u"Events")
    second_label = _event_label(event_2_name, u"Setbacks")
    counters = u"%s %s | %s %s" % (
        first_label, _delta_text(event_1_value, event_1_delta),
        second_label, _delta_text(event_2_value, event_2_delta))
    reward_text = _as_unicode(reward_text)
    if reward_text:
        counters = reward_text + u" | " + counters
    try:
        hint_budget = int(max_chars) - len(counters) - 3
    except Exception:
        hint_budget = 0
    if hint_budget < 10:
        counters = u"E1 %s | E2 %s" % (
            _delta_text(event_1_value, event_1_delta),
            _delta_text(event_2_value, event_2_delta))
        hint_budget = int(max_chars) - len(counters) - 3
    if hint_budget < 8:
        return _clip_text(counters, max_chars)
    hint_text = _clip_text(hint, hint_budget)
    if not hint_text:
        return _clip_text(counters, max_chars)
    return _clip_text(hint_text + u" | " + counters, max_chars)


def _bool_value(value):
    return bool(value)


def _hide(screen):
    try:
        screen.hide(_PANEL)
    except Exception:
        pass
    try:
        screen.hide(_TITLE)
    except Exception:
        pass
    try:
        screen.hide(_SUMMARY)
    except Exception:
        pass
    for index in range(_MAX_VISIBLE_ROWS):
        for suffix in ("", "Values"):
            try:
                screen.hide(_ROW_PREFIX + str(index) + suffix)
            except Exception:
                pass
    for index in range(_MAX_CHOICE_BUTTONS):
        try:
            screen.hide(_choice_widget_name(index))
        except Exception:
            pass


def _hide_all(screen):
    _hide(screen)
    for name in (_TOGGLE_PANEL, _TOGGLE, _TOGGLE_HIT, _TOGGLE_LABEL):
        try:
            screen.hide(name)
        except Exception:
            pass

def _layout(screen):
    """Return a responsive panel and toggle layout for the active resolution."""
    width = _int_value(_safe_call(screen, "getXResolution", (), 1024), 1024)
    height = _int_value(_safe_call(screen, "getYResolution", (), 768), 768)
    margin = max(6, int(width * 0.008))
    panel_width = min(600, max(520, int(width * 0.46)))
    panel_width = min(panel_width, max(220, width - 2 * margin))
    line_height = max(18, int(height * 0.025))
    toggle_width = min(80, max(64, int(width * 0.0625)))
    toggle_height = max(22, int(height * 0.032))
    choice_height = max(30, min(38, line_height + 10))
    panel_height = 74 + (_MAX_VISIBLE_ROWS * 2 * line_height) + 10 + choice_height + 8
    # Keep the panel below the top-left resource HUD and above the lower-left
    # selection panel at common resolutions. All coordinates are resolution-derived.
    top = max(112, int(height * 0.18))
    max_top = max(70, height - panel_height - max(24, int(height * 0.08)))
    top = min(top, max_top)
    toggle_y = max(4, int(height * 0.006))
    return (margin, top, panel_width, panel_height, line_height,
            toggle_width, toggle_height, toggle_y, width, height)

def _set_text(screen, name, text, x, y, font=FontTypes.SMALL_FONT,
        justify=CvUtil.FONT_LEFT_JUSTIFY):
    try:
        rendered = u"<color=255,255,255,255>%s</color>" % text
        screen.setText(name, "Background", rendered, justify,
                x, y, -0.2, font, WidgetTypes.WIDGET_GENERAL, -1, -1)
        screen.setHitTest(name, HitTestTypes.HITTEST_NOHIT)
        screen.show(name)
    except Exception:
        pass

def _toggle_art_path():
    """Resolve the same menu icon art used by Civ4's native clickable button."""
    try:
        art_info = ArtFileMgr.getInterfaceArtInfo(_TOGGLE_ART)
        path = art_info.getPath()
        if path:
            return path
    except Exception:
        pass
    # Base-game fallback; this is the path behind MainMenuButton.
    return "Art/Interface/Buttons/General/Menu_button.dds"


def _set_toggle(screen, x, y, panel_width, toggle_width, toggle_height, toggle_y,
        create=True):
    # Keep one native image button outside the panel hit region.
    screen_width = _int_value(_safe_call(screen, "getXResolution", (), 1024), 1024)
    toggle_x = max(6, int(screen_width * 0.142))
    if toggle_x + toggle_width > screen_width - 6:
        toggle_x = max(6, screen_width - toggle_width - 6)
    try:
        screen.hide(_TOGGLE_PANEL)
        screen.setHitTest(_TOGGLE_PANEL, HitTestTypes.HITTEST_NOHIT)
    except Exception:
        pass
    # Match Civ4's working DynTraits construction: the graphic itself is the
    # full hit surface, with the Python widget/data pair as the input contract.
    # Create it once per MainInterface lifecycle. Recreating a live control on
    # every 250 ms refresh can orphan its cursor/click state and leave help text
    # latched after the pointer leaves it.
    button_surface = not create
    if create:
        button_surface = False
        widget_type = _dpf_widget_type()
        try:
            screen.setImageButton(_TOGGLE, _toggle_art_path(),
                toggle_x, toggle_y, toggle_width, toggle_height,
                widget_type, _TOGGLE_DATA1, _TOGGLE_DATA2)
            button_surface = True
        except Exception:
            # Keep a native button fallback if a theme rejects image construction.
            try:
                screen.setButtonGFC(_TOGGLE, u"DPF", "",
                    toggle_x, toggle_y, toggle_width, toggle_height,
                    widget_type, _TOGGLE_DATA1, _TOGGLE_DATA2,
                    ButtonStyles.BUTTON_STYLE_STANDARD)
                button_surface = True
            except Exception:
                pass
    # Retire every earlier decorative/label surface. The image button above is
    # the only visible and clickable control.
    for name in (_TOGGLE_PANEL, _TOGGLE_HIT, _TOGGLE_LABEL):
        try:
            screen.hide(name)
            screen.setHitTest(name, HitTestTypes.HITTEST_NOHIT)
        except Exception:
            pass
    if button_surface:
        try:
            screen.show(_TOGGLE)
            screen.enable(_TOGGLE, True)
            screen.moveToFront(_TOGGLE)
        except Exception:
            pass
def _ensure_widgets(main_interface):
    try:
        screen = main_interface.screen
    except Exception:
        return None
    _register_toggle_input(main_interface)
    _register_choice_input(main_interface)
    layout = _layout(screen)
    x, y, panel_width, panel_height, line_height, toggle_width, toggle_height, toggle_y, width, height = layout
    if (getattr(main_interface, "_dpf_status_widgets_ready", False) and
            getattr(main_interface, "_dpf_status_layout", None) == layout):
        _set_toggle(screen, x, y, panel_width, toggle_width, toggle_height, toggle_y,
            False)
        return (screen, layout)
    try:
        screen.addPanel(_PANEL, u"", u"", True, False,
            x, y, panel_width, panel_height,
            PanelStyles.PANEL_STYLE_HUD_HELP)
    except Exception:
        pass
    try:
        screen.setStyle(_PANEL, "Panel_Game_HudBL_Style")
        screen.setHitTest(_PANEL, HitTestTypes.HITTEST_NOHIT)
    except Exception:
        pass
    # Create labels once; setText below updates them on every redraw.
    _set_text(screen, _TITLE, u"", x + 10, y + 22, FontTypes.SMALL_FONT)
    _set_text(screen, _SUMMARY, u"", x + 10, y + 45, FontTypes.SMALL_FONT)
    for index in range(_MAX_VISIBLE_ROWS):
        row_y = y + 70 + (index * 2 * line_height)
        _set_text(screen, _ROW_PREFIX + str(index), u"", x + 10, row_y,
                FontTypes.SMALL_FONT)
        _set_text(screen, _ROW_PREFIX + str(index) + "Values", u"",
                x + 22, row_y + line_height, FontTypes.SMALL_FONT)
    main_interface._dpf_status_screen = screen
    main_interface._dpf_status_layout = layout
    main_interface._dpf_status_choice_signature = None
    main_interface._dpf_status_widgets_ready = True
    _set_toggle(screen, x, y, panel_width, toggle_width, toggle_height, toggle_y,
        True)
    return (screen, layout)

def _delta_text(value, delta):
    if delta is None:
        return str(value)
    return "%d(%+d)" % (value, delta)


def _query_rows(player):
    count = _int_value(_safe_call(player, "dpfGetInstanceCount", (), None), None)
    if count is None:
        return None
    count = max(0, min(count, 64))
    rows = []
    for index in range(count):
        definition_id = _int_value(_safe_call(
            player, "dpfGetInstanceDefinitionIdAt", (index,), None), None)
        if definition_id is None:
            continue
        status = _int_value(_safe_call(
            player, "dpfGetInstanceStatus", (definition_id,), 0), 0)
        stage = _int_value(_safe_call(
            player, "dpfGetInstanceStage", (definition_id,), 0), 0)
        variable = _int_value(_safe_call(
            player, "dpfGetInstanceVariable", (definition_id, 1), 0), 0)
        reward_capacity = 0
        reward_protection = 0
        reward_exposed = 0
        if definition_id == _DPF_REWARD_DEFINITION:
            reward_capacity = _int_value(_safe_call(
                player, "dpfGetInstanceVariable",
                (definition_id, _DPF_VAR_NETWORK_CAPACITY), 0), 0)
            reward_protection = _int_value(_safe_call(
                player, "dpfGetInstanceVariable",
                (definition_id, _DPF_VAR_PROTECTION_CHARGES), 0), 0)
            reward_exposed = _int_value(_safe_call(
                player, "dpfGetInstanceVariable",
                (definition_id, _DPF_VAR_NETWORK_EXPOSED), 0), 0)
        event_1 = _int_value(_safe_call(
            player, "dpfGetInstanceEventCount", (definition_id, 1), 0), 0)
        event_2 = _int_value(_safe_call(
            player, "dpfGetInstanceEventCount", (definition_id, 2), 0), 0)
        rows.append({
            "definition": definition_id,
            "status": status,
            "stage": stage,
            "variable": variable,
            "reward_capacity": reward_capacity,
            "reward_protection": reward_protection,
            "reward_exposed": reward_exposed,
            "event_1": event_1,
            "event_2": event_2,
        })
    return rows


def _query_pending_choice(player):
    """Read the native pending choice for player-facing status text."""
    if not _bool_value(_safe_call(player, "dpfHasPendingChoice", (), False)):
        return None
    definition_id = _int_value(_safe_call(
        player, "dpfGetPendingChoiceDefinitionId", (), -1), -1)
    request_id = _int_value(_safe_call(
        player, "dpfGetPendingChoiceRequestId", (), 0), 0)
    option_count = _int_value(_safe_call(
        player, "dpfGetPendingChoiceOptionCount", (), 0), 0)
    option_count = max(0, min(option_count, 8))
    options = []
    option_ids = []
    option_descriptions = []
    for index in range(option_count):
        option_id = _int_value(_safe_call(
            player, "dpfGetPendingChoiceOptionIdAt", (index,), -1), -1)
        if option_id < 0:
            continue
        option_name = _safe_call(
            player, "dpfGetPendingChoiceOptionName", (option_id,), "")
        if not option_name:
            option_name = u"Option %d" % option_id
        option_description = _safe_call(
            player, "dpfGetPendingChoiceOptionDescription", (option_id,), "")
        options.append(_as_unicode(option_name))
        option_ids.append(option_id)
        option_descriptions.append(_as_unicode(option_description))
    return {"definition": definition_id, "request": request_id,
            "options": options, "option_ids": option_ids,
            "option_descriptions": option_descriptions}


def _reward_text(row):
    """Return concise player-facing outcome state for the test progression."""
    if row.get("definition", -1) != _DPF_REWARD_DEFINITION:
        return u""
    parts = []
    capacity = _int_value(row.get("reward_capacity", 0), 0)
    protection = _int_value(row.get("reward_protection", 0), 0)
    exposed = _int_value(row.get("reward_exposed", 0), 0)
    if capacity > 0:
        parts.append(u"Capacity %d" % capacity)
    if protection > 0:
        parts.append(u"Protection %d" % protection)
    if exposed:
        parts.append(u"Exposed")
    return u" | ".join(parts)


def _reward_summary(rows):
    for row in rows:
        if row.get("definition", -1) != _DPF_REWARD_DEFINITION:
            continue
        text = _reward_text(row)
        if text:
            return text
    return u"none"


def _choice_button_geometry(layout, index, count):
    """Return a compact two-column button rectangle inside the panel."""
    x, y, panel_width, panel_height, line_height, toggle_width, toggle_height, toggle_y, width, height = layout
    count = max(1, min(int(count), _MAX_CHOICE_BUTTONS))
    button_height = max(30, min(38, line_height + 10))
    margin = 10
    gap = 6
    available = max(120, panel_width - (2 * margin) - (gap * (count - 1)))
    button_width = max(60, int(available / count))
    button_x = x + margin + (index * (button_width + gap))
    button_y = y + panel_height - button_height - 8
    return button_x, button_y, button_width, button_height


def _choice_button_char_limit(width):
    try:
        return max(8, int((int(width) - 12) / 9))
    except Exception:
        return 24


def _hide_choice_buttons(screen):
    for index in range(_MAX_CHOICE_BUTTONS):
        try:
            screen.hide(_choice_widget_name(index))
        except Exception:
            pass


def _update_choice_buttons(main_interface, pending_choice):
    """Create/show native text buttons for the current pending choice."""
    try:
        screen = main_interface.screen
        layout = main_interface._dpf_status_layout
    except Exception:
        return
    if pending_choice is None:
        _hide_choice_buttons(screen)
        main_interface._dpf_status_choice_signature = None
        return
    option_ids = pending_choice.get("option_ids", [])
    option_names = pending_choice.get("options", [])
    descriptions = pending_choice.get("option_descriptions", [])
    count = min(len(option_ids), len(option_names), _MAX_CHOICE_BUTTONS)
    if count <= 0:
        _hide_choice_buttons(screen)
        main_interface._dpf_status_choice_signature = None
        return
    request_id = _int_value(pending_choice.get("request", 0), 0)
    signature = (request_id, tuple(option_ids[:count]),
                 tuple(option_names[:count]), tuple(descriptions[:count]),
                 tuple(layout))
    configured = True
    if getattr(main_interface, "_dpf_status_choice_signature", None) != signature:
        for index in range(count):
            button_name = _choice_widget_name(index)
            button_x, button_y, button_width, button_height = _choice_button_geometry(
                layout, index, count)
            label = _clip_text(option_names[index],
                _choice_button_char_limit(button_width))
            try:
                screen.setButtonGFC(button_name, label, "",
                    button_x, button_y, button_width, button_height,
                    _dpf_widget_type(), _CHOICE_DATA1,
                    option_ids[index], ButtonStyles.BUTTON_STYLE_STANDARD)
                # MainInterface standard buttons need an explicit existing HUD
                # style to render their full native clickable surface.
                screen.setStyle(button_name, "Button_Metal_Style")
            except Exception:
                configured = False
        if configured:
            main_interface._dpf_status_choice_signature = signature
    for index in range(_MAX_CHOICE_BUTTONS):
        button_name = _choice_widget_name(index)
        if index < count:
            try:
                screen.show(button_name)
                screen.enable(button_name, True)
                screen.moveToFront(button_name)
            except Exception:
                pass
        else:
            try:
                screen.hide(button_name)
            except Exception:
                pass


def _refresh(main_interface, reset=False):
    try:
        screen = main_interface.screen
    except Exception:
        return
    if reset:
        main_interface._dpf_status_previous = None
        main_interface._dpf_status_previous_turn = None
        main_interface._dpf_status_deltas = {}
        main_interface._dpf_status_player = -1
        main_interface._dpf_status_loaded = False
        main_interface._dpf_status_last_current = None
        main_interface._dpf_status_widgets_ready = False
        main_interface._dpf_status_screen = None
        main_interface._dpf_status_layout = None
        main_interface._dpf_status_choice_signature = None
        main_interface._dpf_status_open = False

    visibility = _safe_call(CyInterface(), "getShowInterface", (), None)
    if visibility in (InterfaceVisibility.INTERFACE_HIDE_ALL,
            InterfaceVisibility.INTERFACE_MINIMAP_ONLY,
            InterfaceVisibility.INTERFACE_ADVANCED_START):
        _hide_all(screen)
        return
    try:
        if CyGame().isPitbossHost() or CyInterface().isCityScreenUp():
            _hide_all(screen)
            return
    except Exception:
        pass

    active_player = _int_value(_safe_call(
        CyGame(), "getActivePlayer", (), -1), -1)
    if active_player < 0 or active_player >= gc.getMAX_PLAYERS():
        _hide_all(screen)
        return
    player = gc.getPlayer(active_player)
    if not _bool_value(_safe_call(player, "dpfIsEnabled", (), False)):
        _hide_all(screen)
        return
    rows = _query_rows(player)
    if rows is None:
        _hide_all(screen)
        return

    prepared = _ensure_widgets(main_interface)
    if prepared is None:
        return
    screen, layout = prepared
    x, y, panel_width, panel_height, line_height, toggle_width, toggle_height, toggle_y, width, height = layout
    if not getattr(main_interface, "_dpf_status_open", True):
        _hide(screen)
        for name in (_TOGGLE,):
            try:
                screen.show(name)
            except Exception:
                pass
        return
    game_turn = _int_value(_safe_call(CyGame(), "getGameTurn", (), 0), 0)
    pending = _int_value(_safe_call(
        player, "dpfGetPendingEventCount", (), 0), 0)
    trace_count = _int_value(_safe_call(
        player, "dpfGetTraceCount", (), 0), 0)
    trait_id = gc.getInfoTypeForString("TRAIT_DPF_TEST_SHARED")
    trait_claim = _bool_value(_safe_call(
        player, "dpfHasTraitClaim", (trait_id,), False))
    trait_text = u"ON"
    if not trait_claim:
        trait_text = u"OFF"

    pending_choice = _query_pending_choice(player)
    _update_choice_buttons(main_interface, pending_choice)

    current = {}
    for row in rows:
        current[row["definition"]] = (
            row["variable"], row["event_1"], row["event_2"],
            row["status"], row["stage"])
    if getattr(main_interface, "_dpf_status_player", active_player) != active_player:
        main_interface._dpf_status_previous = None
        main_interface._dpf_status_previous_turn = None
        main_interface._dpf_status_deltas = {}
        main_interface._dpf_status_last_current = None
    main_interface._dpf_status_player = active_player
    previous = getattr(main_interface, "_dpf_status_previous", None)
    previous_turn = getattr(main_interface, "_dpf_status_previous_turn", None)
    deltas = getattr(main_interface, "_dpf_status_deltas", {})
    last_current = getattr(main_interface, "_dpf_status_last_current", None)
    if previous is None or previous_turn is None or previous_turn != game_turn:
        deltas = {}
        if previous is not None and previous_turn is not None:
            for definition_id in current:
                if definition_id not in previous:
                    continue
                old = previous[definition_id]
                new = current[definition_id]
                deltas[definition_id] = (
                    new[0] - old[0], new[1] - old[1], new[2] - old[2])
        main_interface._dpf_status_previous = current
        main_interface._dpf_status_previous_turn = game_turn
        main_interface._dpf_status_deltas = deltas
        main_interface._dpf_status_last_current = current
    elif last_current != current:
        # Keep the turn-start baseline while reflecting a state change that
        # occurs before the engine increments the visible game turn.
        deltas = {}
        for definition_id in current:
            if definition_id not in previous:
                continue
            old = previous[definition_id]
            new = current[definition_id]
            deltas[definition_id] = (
                new[0] - old[0], new[1] - old[1], new[2] - old[2])
        main_interface._dpf_status_deltas = deltas
        main_interface._dpf_status_last_current = current

    _set_text(screen, _TITLE, u"Dynamic Progress   Turn %d" % game_turn,
            x + 10, y + 22, FontTypes.SMALL_FONT)
    reward_state = _reward_summary(rows)
    if trait_claim:
        reward_state = u"trait active"
    choice_text = u""
    if pending_choice is not None:
        choice_text = u"   Choice required (%d)" % pending_choice["request"]
    _set_text(screen, _SUMMARY,
            u"Progressions: %d   Events waiting: %d   Rewards: %s%s" %
            (len(rows), pending, reward_state, choice_text),
        x + 10, y + 45, FontTypes.SMALL_FONT)

    visible_count = min(len(rows), _MAX_VISIBLE_ROWS)
    if len(rows) > _MAX_VISIBLE_ROWS:
        visible_count = _MAX_VISIBLE_ROWS - 1
    value_limit = _value_char_limit(panel_width)
    for index in range(_MAX_VISIBLE_ROWS):
        row_name = _ROW_PREFIX + str(index)
        values_name = row_name + "Values"
        row_y = y + 70 + (index * 2 * line_height)
        if index >= visible_count:
            try:
                screen.hide(row_name)
                screen.hide(values_name)
            except Exception:
                pass
            continue
        row = rows[index]
        name, status_name, hint, event_1_name, event_2_name = _display_content(
            row["definition"], row["stage"], row["status"], player)
        _set_text(screen, row_name,
                u"%s - %s" % (name, status_name),
                x + 10, row_y, FontTypes.SMALL_FONT)
        delta = deltas.get(row["definition"], None)
        if delta is None:
            variable_delta = event_1_delta = event_2_delta = None
        else:
            variable_delta, event_1_delta, event_2_delta = delta
        if (pending_choice is not None and
                row["definition"] == pending_choice["definition"] and
                pending_choice["options"]):
            value_text = _clip_text(
                u"Choose: " + u" / ".join(pending_choice["options"]),
                value_limit)
        else:
            value_text = _format_value_text(hint, event_1_name,
                row["event_1"], event_1_delta, event_2_name,
                row["event_2"], event_2_delta, value_limit,
                _reward_text(row))
        _set_text(screen, values_name, value_text,
            x + 22, row_y + line_height, FontTypes.SMALL_FONT)

    if len(rows) > _MAX_VISIBLE_ROWS:
        overflow_index = _MAX_VISIBLE_ROWS - 1
        overflow_y = y + 70 + (overflow_index * 2 * line_height)
        _set_text(screen, _ROW_PREFIX + str(overflow_index),
                u"+%d more progressions" % (len(rows) - visible_count),
            x + 10, overflow_y, FontTypes.SMALL_FONT)
        try:
            screen.hide(_ROW_PREFIX + str(overflow_index) + "Values")
        except Exception:
            pass

    for name in (_PANEL, _TITLE, _SUMMARY):
        try:
            screen.show(name)
        except Exception:
            pass
    for index in range(_MAX_VISIBLE_ROWS):
        for suffix in ("", "Values"):
            try:
                screen.show(_ROW_PREFIX + str(index) + suffix)
            except Exception:
                pass
    for name in (_TOGGLE,):
        try:
            screen.show(name)
        except Exception:
            pass
    for name in (_PANEL, _TITLE, _SUMMARY):
        try:
            screen.moveToFront(name)
        except Exception:
            pass
    for index in range(_MAX_VISIBLE_ROWS):
        for suffix in ("", "Values"):
            try:
                screen.moveToFront(_ROW_PREFIX + str(index) + suffix)
            except Exception:
                pass
    for index in range(_MAX_CHOICE_BUTTONS):
        try:
            screen.moveToFront(_choice_widget_name(index))
        except Exception:
            pass

# Keep the DPF button above panel chrome and status text.
    for name in (_TOGGLE,):
        try:
            screen.moveToFront(name)
        except Exception:
            pass


def _safe_refresh(main_interface, reset=False):
    try:
        _refresh(main_interface, reset)
    except Exception:
        try:
            _hide(main_interface.screen)
        except Exception:
            pass

def _show_toggle(screen):
    """Show the DPF control and keep it in front of HUD chrome."""
    for name in (_TOGGLE,):
        try:
            screen.show(name)
            screen.moveToFront(name)
        except Exception:
            pass

def _is_clicked(input_class):
    """Recognize Civ4's click enum and its documented numeric value."""
    try:
        code = input_class.getNotifyCode()
        return (code == NotifyCode.NOTIFY_CLICKED or code == 11)
    except Exception:
        return False



def _is_cursor_off(input_class):
    """Recognize Civ4's cursor-leave notification for the DPF control."""
    try:
        return input_class.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_OFF
    except Exception:
        return False

def _is_cursor_on(input_class):
    """Recognize Civ4's cursor-enter notification for the DPF control."""
    try:
        return input_class.getNotifyCode() == NotifyCode.NOTIFY_CURSOR_MOVE_ON
    except Exception:
        return False

def _is_toggle_event(input_class):
    """Recognize DPF events by the native data contract or widget name."""
    return (_is_toggle_data_input(input_class) or
            _is_toggle_input(input_class))

def _set_help_text(main_interface, text):
    """Set Civ4's shared native help strip for a hovered DPF control."""
    try:
        value = _as_unicode(text)
    except Exception:
        value = text or ""
    try:
        main_interface.screen.setHelpTextString(value)
        return
    except Exception:
        pass
    try:
        CyInterface().setHelpTextString(value)
    except Exception:
        pass

def _clear_toggle_help(main_interface):
    """Clear the shared help string when the DPF button loses the cursor."""
    _set_help_text(main_interface, "")

def _toggle_hover_text(main_interface):
    """Return the native help text for the DPF open/close control."""
    return u"Open or close DPF status"

def _choice_hover_text(main_interface, input_class):
    """Resolve a pending choice's player-readable native hover help."""
    try:
        active_player = _int_value(_safe_call(
            CyGame(), "getActivePlayer", (), -1), -1)
        if active_player < 0 or active_player >= gc.getMAX_PLAYERS():
            return u"Choose this DPF option"
        player = gc.getPlayer(active_player)
        pending_choice = _query_pending_choice(player)
        if pending_choice is None:
            return u"Choose this DPF option"
        option_id = _choice_option_id(input_class, pending_choice)
        if option_id < 0:
            return u"Choose this DPF option"
        option_ids = pending_choice.get("option_ids", [])
        option_names = pending_choice.get("options", [])
        descriptions = pending_choice.get("option_descriptions", [])
        index = -1
        try:
            index = option_ids.index(option_id)
        except Exception:
            pass
        name = u""
        description = u""
        if index >= 0 and index < len(option_names):
            name = _as_unicode(option_names[index])
        if index >= 0 and index < len(descriptions):
            description = _as_unicode(descriptions[index])
        if name and description:
            return u"%s: %s" % (name, description)
        if name:
            return name
    except Exception:
        pass
    return u"Choose this DPF option"
def _toggle_input_action(main_interface, input_class):
    """Handle one DPF click; return zero for hover events."""
    if not _is_clicked(input_class):
        return 0
    try:
        CvUtil.pyPrint("DPFStatusPanel: toggle click")
    except Exception:
        pass
    main_interface._dpf_status_open = not getattr(
        main_interface, "_dpf_status_open", True)
    if main_interface._dpf_status_open:
        _safe_refresh(main_interface, False)
    else:
        try:
            _hide(main_interface.screen)
        except Exception:
            pass
        _show_toggle(main_interface.screen)
    return 1

def _register_toggle_input(main_interface):
    """Register exact and legacy-suffixed IDs in Civ4's native input map."""
    try:
        input_map = main_interface.MainInterfaceInputMap
    except Exception:
        return
    try:
        action = main_interface._dpf_status_toggle_action
    except Exception:
        def action(input_class, owner=main_interface):
            return _toggle_input_action(owner, input_class)
        try:
            main_interface._dpf_status_toggle_action = action
        except Exception:
            pass
    for name in (_TOGGLE, _TOGGLE_HIT, _TOGGLE_LABEL,
                _TOGGLE + "1", _TOGGLE_HIT + "1", _TOGGLE_LABEL + "1"):
        try:
            input_map[name] = action
        except Exception:
            pass

def _is_toggle_data_input(input_class):
    """Recognize the stable data1/data2 pair used by native Python buttons."""
    try:
        return (input_class.getData1() == _TOGGLE_DATA1 and
                input_class.getData2() == _TOGGLE_DATA2)
    except Exception:
        return False


def _is_toggle_input(input_class):
    try:
        function_name = input_class.getFunctionName()
        try:
            function_name = str(function_name)
        except Exception:
            pass
        for base_name in (_TOGGLE, _TOGGLE_HIT, _TOGGLE_LABEL):
            if function_name == base_name:
                return True
            if function_name.startswith(base_name):
                tail = function_name[len(base_name):]
                if tail == "" or tail == "1" or tail.isdigit():
                    return True
    except Exception:
        pass
    return False


def _is_choice_data_input(input_class):
    try:
        return (input_class.getData1() == _CHOICE_DATA1 and
                _int_value(input_class.getData2(), -1) >= 0)
    except Exception:
        return False


def _is_choice_input(input_class):
    try:
        function_name = str(input_class.getFunctionName())
        if not function_name.startswith(_CHOICE_PREFIX):
            return False
        tail = function_name[len(_CHOICE_PREFIX):]
        return tail.isdigit() and int(tail) > 0
    except Exception:
        return False


def _is_choice_event(input_class):
    return _is_choice_data_input(input_class) or _is_choice_input(input_class)


def _choice_option_id(input_class, pending_choice):
    option_ids = pending_choice.get("option_ids", [])
    data_id = _int_value(_safe_call(input_class, "getData2", (), -1), -1)
    if data_id in option_ids:
        return data_id
    try:
        function_name = str(input_class.getFunctionName())
        tail = function_name[len(_CHOICE_PREFIX):]
        index = int(tail) - 1
        if index >= 0 and index < len(option_ids):
            return option_ids[index]
    except Exception:
        pass
    return -1


def _choice_input_action(main_interface, input_class):
    """Submit one pending DPF choice through the native command surface."""
    if not _is_clicked(input_class):
        return 0
    try:
        active_player = _int_value(_safe_call(
            CyGame(), "getActivePlayer", (), -1), -1)
        if active_player < 0 or active_player >= gc.getMAX_PLAYERS():
            return 1
        player = gc.getPlayer(active_player)
        pending_choice = _query_pending_choice(player)
        if pending_choice is None:
            return 1
        option_id = _choice_option_id(input_class, pending_choice)
        request_id = _int_value(pending_choice.get("request", 0), 0)
        if option_id < 0 or request_id <= 0:
            return 1
        accepted = _bool_value(_safe_call(player, "dpfSubmitChoice",
            (request_id, option_id), False))
        try:
            CvUtil.pyPrint("DPFStatusPanel: choice request %d option %d accepted %d" %
                (request_id, option_id, int(accepted)))
        except Exception:
            pass
        if accepted:
            main_interface._dpf_status_choice_signature = None
            _clear_toggle_help(main_interface)
            _safe_refresh(main_interface, False)
    except Exception:
        pass
    return 1


def _register_choice_input(main_interface):
    """Register stable choice widget names with the main HUD input map."""
    try:
        input_map = main_interface.MainInterfaceInputMap
    except Exception:
        return
    try:
        action = main_interface._dpf_status_choice_action
    except Exception:
        def action(input_class, owner=main_interface):
            return _choice_input_action(owner, input_class)
        try:
            main_interface._dpf_status_choice_action = action
        except Exception:
            pass
    for index in range(_MAX_CHOICE_BUTTONS):
        name = _choice_widget_name(index)
        for candidate in (name, name + "1"):
            try:
                input_map[candidate] = action
            except Exception:
                pass


def handle_input(main_interface, input_class):
    """Handle DPF input before global screen-utils routing."""
    try:
        if _is_toggle_event(input_class):
            if _is_cursor_off(input_class):
                _clear_toggle_help(main_interface)
                return 1
            if _is_cursor_on(input_class):
                _set_help_text(main_interface, _toggle_hover_text(main_interface))
                return 1
            if _is_clicked(input_class):
                return _toggle_input_action(main_interface, input_class)
        if _is_choice_event(input_class):
            if _is_cursor_off(input_class):
                _clear_toggle_help(main_interface)
                return 1
            if _is_cursor_on(input_class):
                _set_help_text(main_interface,
                    _choice_hover_text(main_interface, input_class))
                return 1
            if _is_clicked(input_class):
                return _choice_input_action(main_interface, input_class)
    except Exception:
        pass
    return 0

def install(CvMainInterfaceModule):
    """Install idempotent wrappers around the existing main HUD lifecycle."""
    try:
        interface_class = CvMainInterfaceModule.CvMainInterface
    except Exception:
        return
    if getattr(interface_class, "_dpf_status_panel_installed", False):
        return
    original_interface_screen = interface_class.interfaceScreen
    original_redraw = interface_class.redraw
    original_update_screen = interface_class.updateScreen
    original_handle_input = interface_class.handleInput

    def interface_screen(self, _original=original_interface_screen):
        result = _original(self)
        _safe_refresh(self, True)
        return result

    def redraw(self, _original=original_redraw):
        result = _original(self)
        _safe_refresh(self, False)
        return result

    def update_screen(self, _original=original_update_screen):
        result = _original(self)
        _safe_refresh(self, False)
        return result

    def handle_input(self, input_class, _original=original_handle_input):
        try:
            if _is_toggle_event(input_class):
                if _is_cursor_off(input_class):
                    _clear_toggle_help(self)
                    return 1
                if _is_cursor_on(input_class):
                    _set_help_text(self, _toggle_hover_text(self))
                    return 1
                if _is_clicked(input_class):
                    return _toggle_input_action(self, input_class)
            if _is_choice_event(input_class):
                if _is_cursor_off(input_class):
                    _clear_toggle_help(self)
                    return 1
                if _is_cursor_on(input_class):
                    _set_help_text(self, _choice_hover_text(self, input_class))
                    return 1
                if _is_clicked(input_class):
                    return _choice_input_action(self, input_class)
        except Exception:
            pass
        return _original(self, input_class)

    interface_class.interfaceScreen = interface_screen
    interface_class.redraw = redraw
    interface_class.updateScreen = update_screen
    interface_class.handleInput = handle_input
    interface_class._dpf_status_panel_installed = True




