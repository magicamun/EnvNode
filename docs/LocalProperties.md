# Local properties: read-only Sensor, Actuator and Controller views

`IPropertyReader` is a transport-independent view of named, typed runtime state.
`SensorPropertyReader` exposes state measurements of one existing Sensor;
`ActuatorPropertyReader` exposes logical On/Off output state, and
`ControllerPropertyReader` exposes the stored Threshold evaluation reason.
These adapters neither acquire measurements, control hardware nor store a second
copy of the latest state. MQTT publication and the acquisition pipeline are unchanged.

## Contract

A `PropertyReference` combines component category, slot ID and a stable key.
Supported categories are `Sensor`, `Actuator` and `Controller`. Sensor keys come directly
from `MeasurementTypeMetadata` (for example `temperature`); display names are
not identifiers. Keys are case-sensitive. Slot references follow the slot when
its Sensor implementation is replaced, rather than identifying a physical unit.

`describe()` provides the expected value kind and canonical unit even before
the first measurement. It uses existing metadata and validates the bound
Sensor's identity and supported measurement types. Description strings are
static; reference strings need only remain valid during the call.

`read()` returns:

- `UnknownReference`: wrong category or component ID, unknown key, unsupported
  capability, or Sensor event rather than state.
- `NoValue`: the reference is supported but its current value is unavailable.
- `Available`: snapshot found, including snapshots marked invalid.

The snapshot copies the typed value, validity, quality, epoch timestamp,
monotonic acceptance time and equality-only revision token. `Available` does
not imply valid or fresh. Consumers decide freshness using their own age limit.
On unsuccessful reads the output is reset, so an earlier value cannot leak
through. A read returns a copy and cannot modify the authoritative cache.

PropertyValue wraps the existing MeasurementValue scalar storage and adds typed
enumeration values. PropertyValueKind distinguishes enum values from numbers
and Booleans. MeasurementValue and the MQTT measurement representation remain
unchanged. Free text properties are not implemented.
Event measurements such as rain gauge tips are deliberately excluded from
this state view.

## Usage

```cpp
SensorPropertyReader adapter(existingSensor, measurementResolver);
const IPropertyReader& properties = adapter;
const PropertyReference temperature(PropertyComponentKind::Sensor, 3, "temperature");
PropertyDescription description;
PropertySnapshot snapshot;
if (properties.describe(temperature, description)
    && properties.read(temperature, snapshot) == PropertyReadResult::Available
    && snapshot.valid) {
    float value = 0;
    if (snapshot.value.tryGetFloatingPoint(value)) {
        // value is in description.canonicalUnit; formatting is a later step.
    }
}
```

The bound Sensor and resolver must outlive the adapter. The adapter uses only
the abstract `ISensor` metadata methods and `IMeasurementResolver::latest()`;
it never calls begin/service/sample. Recreate it when rebuilding the Sensor
runtime. Calls follow the existing single-loop runtime model, with no new
concurrency guarantees.

The Measurements web page now includes a small Property diagnostic card.
The text formatter is used for the preview and its saved display page; there is
an optional SSD1309 OLED output and no MQTT change. Current pre-time-synchronization discard behavior remains:
the view cannot supply values that the measurement pipeline has not stored.

## Validation

`pio test -e native -f test_sensor_properties` covers metadata before the first
sample, temperature and Boolean reads, unsupported references, invalid values,
timestamp/quality preservation, live updates, cache clear, copy isolation and
event exclusion. Web diagnostic tests additionally cover live value updates,
missing/invalid samples, escaped references and monotonic clock wrap.
`test_measurement_input` remains the baseline for the existing resolver/cache contract.

## Web diagnostic

After flashing, open **Measurements** (`/measurements`) and scroll to
**Property diagnostic**. The page chooses the first supported state property
in active Sensor registration order, then MeasurementType order (Temperature
first when supported). No installation-specific slot is hard-coded.

The card displays the local reference, canonical value/unit, validity, quality
and monotonic age. Values use a decimal point and two fractional digits in this
small diagnostic view; presentation-unit preferences still apply to the existing
measurement table. Refresh the page or use the card's Refresh button to update.

Missing samples and invalid samples are explicitly distinguished. Without an
eligible Sensor the page says so. Age is informational; no new stale threshold
is introduced. Before time synchronization, a missing sample remains expected.

SensorManager supplies a borrowed const Sensor metadata view to PropertyResolver.
The resolver creates a temporary reader for each lookup; the renderer reads
exclusively through IPropertyReader. The existing single-loop runtime prevents
rebuilds during a request; no Sensor pointer is retained across requests.

## On/Off Actuator properties

`ActuatorPropertyReader(id, actuator)` binds an existing `IOnOffActuator` to
`actuator / <id> / state`. The value is Boolean (true = On, false = Off),
with no unit. It reads the current logical output state, not mechanical relay
feedback, and never initializes, switches or shuts down hardware. The borrowed
binding must be recreated after runtime rebuilds.

Known but uninitialized outputs return NoValue; mismatched references return
UnknownReference. Each successful read fetches the current state directly.

PropertySnapshot now explicitly marks optional metadata via hasQuality,
hasTimestamp, hasAcceptedMonotonicMs and hasRevision. Sensor snapshots set these
flags; On/Off outputs do not, because the capability records none of them.
Consumers must check the flags. Zero is not a substitute for unknown time.
The Boolean labels in PropertyDescription allow On/Off without changing the
Boolean value type or sensor True/False presentation.

Measurements now shows the first available On/Off Actuator as a second Property
diagnostic card. Refresh after changing its output in the existing Actuator
controls. The card shows On/Off and explicitly marks quality and age unavailable.
If no available On/Off Actuator exists, the page reports that instead. Selection
follows runtime order and is request-local, covering configured and module
Actuators without retaining bindings across rebuilds. Numeric references follow
the current runtime IDs; this diagnostic does not persist module references.

`test_actuator_properties` verifies live state reads, no write side effects,
uninitialized outputs, unknown references, optional metadata and the shared
HTML renderer. Sensor tests continue to verify time/quality availability.

## Threshold Controller reason

The first Threshold Controller appears as `controller / <id> / reason` on the
Measurements page. Refresh after a new input, stop/start or a stale timeout.
Without a Threshold Controller the page explicitly reports that none is available.

ThresholdController stores its reason where it evaluates the input; the Property
adapter does not reconstruct hysteresis from a current reading. The reason
provider is read-only and bindings are local to one web request.

| Stable code | Meaning |
| --- | --- |
| not_started | Controller has not been started |
| stopped | Controller has been stopped |
| invalid_configuration | Start rejected the configuration |
| no_measurement | No source snapshot |
| invalid_measurement | Invalid, incompatible or non-finite input |
| stale_measurement | Otherwise compatible input is too old |
| on_threshold | Last processed usable input reached the configured On threshold |
| off_threshold | Last processed usable input reached the configured Off threshold |
| hysteresis_hold | Last processed usable input was in-band with a prior decision |
| awaiting_threshold | In-band input before the first On/Off decision |

Threshold codes refer to the configured On/Off direction, so they work for both
OnAbove and OnBelow. Equality counts as reaching a threshold. Re-reading a
snapshot does not turn a threshold reason into a hold reason or repeat an output
command. New usable input updates the evaluation reason. Missing, invalid and
stale source status override it without erasing the existing On/Off decision.

This is the input evaluation reason, not actuator execution status. Target
unavailability or a failed output command remains separately available in the
existing Controller diagnostics. No manual override or irrigation-specific
reason is invented. Schmitt-trigger behavior, retries, MQTT topics and payloads
are unchanged. Stop records stopped even if applying Off fails; the operation
result still reports that failure.

An enum PropertyDescription exposes its finite options as stable code/display
text pairs with static lifetime. PropertyValue::tryGetEnumeration returns such
a pair; numeric and Boolean accessors reject it. Scalar accessors continue to
delegate to MeasurementValue. Enumeration construction requires a static-lived
option. No free-form C string value or enum index is used as the public code.

Reason snapshots are valid even for stopped or error reasons: validity means
the reason itself is known. No quality, timestamp, age or revision is invented;
all optional metadata flags stay false. An unsupported enum value returns NoValue.

Tests exercise both threshold directions, equality, the initial band, held
decisions, missing/invalid/stale input, stop/start, invalid configuration, enum
metadata and type safety, and actual Controller-to-Property-to-HTML reads.

## Shared PropertyResolver

`PropertyResolver(sensors, measurements, actuators, controllers)` implements
IPropertyReader and is the common entry point for consumers. A PropertyReference
(category, current runtime ID, stable key) is sufficient for describe/read.

The resolver selects SensorPropertyReader, ActuatorPropertyReader or
ControllerPropertyReader internally. It resolves each component from the current
runtime on every call; it retains only references to the managers/resolver, not
component pointers, adapter instances or copied values. Those dependencies must
outlive it. A single resolver instance can therefore survive a runtime rebuild.
Calls follow the existing single-loop execution model.

The Measurements diagnostic now uses this common resolver for both description
and value reads. It still chooses the first eligible reference of each category
from the runtime inventory; selection/enumeration is distinct from resolving a
known reference. Rendering and visible selection order remain unchanged.

Unknown categories, absent components or unsupported properties yield
UnknownReference (or false for describe), with output reset. A supported sensor
without a sample yields NoValue; an invalid stored sample remains Available with
valid=false. The Actuator runtime only exposes available On/Off capabilities, so
an unavailable or removed Actuator cannot be resolved. This matches the existing
diagnostic selection. Numeric IDs retain their existing runtime/slot semantics;
the resolver consumes typed references and does not persist identity. The preview
and persistent display configuration share a bounded string-reference parser.

`test_property_resolver` checks all three categories sharing the same numeric ID,
type/metadata preservation, missing and invalid samples, unknown-reference reset,
read-only rendering, Sensor replacement, Actuator removal/recreation and Controller
removal/recreation/implementation changes while retaining the same resolver.

```cpp
PropertyResolver resolver(sensorManager, measurementResolver, actuatorRuntime, controllerRuntime);
const IPropertyReader& properties = resolver;
PropertySnapshot snapshot;
const PropertyReference reference(PropertyComponentKind::Controller, 2, "reason");
const PropertyReadResult status = properties.read(reference, snapshot);
```

The resolver centralizes lookup for diagnostics and the six-line preview below.
It does not store display configuration or drive display hardware.

## Six-line text preview

Open **Display → Six-line text preview**. Each of the six rows has a
format and up to four source fields with shared suggestions. Fill sources from
left to right in placeholder order and click **Preview / refresh all lines**.
Leave sources empty for literal text or an empty line. Values are resolved
through PropertyResolver on each submission. The source list includes supported Sensor
state properties, available On/Off Actor states and Threshold Controller reasons.
Source labels include the component name and canonical unit where applicable.

Examples:

| Source type | Format | Example result |
| --- | --- | --- |
| Temperature | `Temperature: %.1f C` | `Temperature: 21.5 C` |
| Float | `Value: %8.2f` | Padded number with two decimal places |
| On/Off Actor | `Valve: %s` | `Valve: On` |
| Controller enum | `Reason: %s` | `Reason: On threshold reached` |
| Unsigned integer | `Count: %u` | `Count: 42` |

The formatter accepts zero to four ordered conversions, plus literal text and
`%%`. The number of supplied sources must exactly match the number of conversions.
For example, `Level: %.0f l %.0f%%` requires a volume source followed by a
percentage source. Literal text such as `RainControl` and blank lines use zero
sources. The previous one-reference formatter overload remains compatible.
Supported conversions are `%f` (floating point), `%u` (unsigned integer), and
`%s` (Boolean label or enum display text). Optional `-` left-aligns, optional
`0` pads numbers with zeros; width is limited to 64. Float precision is 0–6,
defaulting to printf's six decimal places. If both flags are supplied, left
alignment wins. String precision, dynamic widths, positional parameters, length
modifiers, other flags and other conversions are rejected.

Formats and results are limited to 128 bytes each. Widths are byte-based; the
formatter does not yet provide LCD font/glyph layout. Literal newlines and control
characters are rejected. Results are never silently truncated. Decimal formatting
uses a point and canonical values: adding a unit string does not convert units.

PropertyTextFormatter validates the entire format and source count first, then
checks each property type and value before formatting it. It invokes snprintf only with internal trusted
format patterns, never with the submitted format string. Unknown sources, missing
samples, invalid samples, type mismatches and output overflow produce explicit
errors; no partial or previous result is shown for that line. The other five
lines remain visible; an error marker identifies the affected row. No new staleness policy is added.

Preview uses a read-only GET request. Source and format remain in the URL and can
be bookmarked or refreshed. **Save on device** posts all six rows to `/display/save`;
**Load saved settings** submits a separate GET form containing only `loadDisplay=1`,
discards draft/URL edits and reloads the saved page even when already on Display.
The response is marked `Cache-Control: no-store`. Without URL
overrides, Display loads and renders the saved configuration. No reboot is needed. Form fields f0–f5 and s0_0–s5_3
encode the six rows and four source positions per row. Existing single-line
source/format URLs are loaded into the first row. Source strings have the syntax
`category/ID/key`; the parser requires a known category, nonzero uint16 ID and
a bounded alphanumeric/underscore key. Runtime availability is checked separately
by the resolver. HTML output escapes source names, inputs and formatted text.

Tests cover formatting, bounds, percent handling, unsupported/unsafe format
syntax, type checks, non-finite and invalid values, missing/unknown sources,
reference parsing, HTML escaping, preview updates and no reads before submission.
The resolver integration test also formats real Sensor, Actor and Controller
values through the same entry point used by the page.

The six-row layout reflects a 128×64 OLED with 10-pixel baseline spacing. The
web view preserves line breaks and spaces but does not claim pixel-accurate
font metrics, clipping or wrapping. Physical output is handled by the optional OLED driver. All six rows are
prepared during one request and emitted together. The single-loop runtime
prevents component updates between row reads; there is no background sampling
or output side effect in the formatter. A shared source datalist avoids repeating
the complete source inventory in 24 selectors on the ESP32.

Additional tests cover ordered mixed-type placeholders, swapped sources, zero
sources, missing/extra sources, cumulative output overflow, second-source errors,
six-row positioning, blank rows, escaped literal text and per-row error isolation.

## Persistent display page

`DisplayConfiguration` is independent of Web and the physical display driver.
`ConfigurationService` stores the complete page in the existing `weather` NVS
namespace under `display_page4`, in one versioned NVS blob. Older `display_page` strings
are read when no new blob exists. A successful
write updates the in-memory configuration; a failed write leaves it unchanged.
Version 4 stores enabled/bus/address, six format/source/Boolean-label rows, then
a bounded list of state translations keyed by row, source position and stable
state code. Versions 1–3 are accepted and missing fields get their defaults.
Version 1 leaves OLED output disabled; version 2 has no Boolean labels; version 3
preserves Boolean labels and hardware settings. Saving writes the new blob. Validation forbids control characters
in fields and bounds the complete record to 6144 bytes. Missing, malformed or
unknown-version records leave the default unconfigured page. Factory reset
clears it. An explicitly saved blank page remains blank, without an automatic
source suggestion.

Saving validates format syntax, limits, source syntax and placeholder counts.
Sources must be contiguous from position 1. This deliberately permits references
to currently unavailable components: current values and type compatibility are
checked when rendering, with errors isolated per line. The optional OLED output uses this same saved page. MQTT remains unchanged.

Native tests cover reload, reset, intentionally blank pages, boundary sizes,
malformed records, invalid input and failed storage writes preserving the old page.

## OLED output

Display offers **Display output**, **I2C bus** (with board-profile pins), and
**I2C address** (`0x3C` or `0x3D`). Choose Enabled and Save on device to apply
without reboot. Preview alone does not change the running display. Defaults and
migrated version-1 pages leave output disabled. Hardware settings are persisted
atomically with the text page; conflicting I2C address claims are rejected. On
load, a conflicting output is disabled while text and sensor configuration remain.

`DisplayService` depends on `ITextDisplay`, `IPropertyReader` and a monotonic clock.
`DisplayPageFormatter` evaluates each row for both web preview and physical output.
The service uses the current saved configuration and updates once per second,
with wrap-safe timing. It reinitializes on bus/address changes and powers down on
disable. Unknown/missing/invalid values replace that row with `[Line N error]`;
detailed errors are logged on transitions, with no per-second log flood.

`Ssd1309TextDisplay` uses the lab-confirmed SSD1309 NONAME2 full-buffer setup,
128×64 pixels, U8G2_R0, `u8g2_font_t0_11_tf`, and six 10-pixel baselines. UTF-8
text is decoded; glyph coverage is limited to the chosen font. Long lines clip
at the right edge; they do not wrap into the following row. U8g2 is pinned to
2.36.12; `U8X8_WITH_USER_PTR` is enabled globally for consistent library/driver
structure layout. The custom byte callback uses the already initialized `I2CBusManager`
bus without restarting it or changing its pins/clock. Wire's timeout is temporarily
bounded to 20 ms and restored after each operation. Transfers remain synchronous;
at 100 kHz a full frame takes roughly a tenth of a second, not a hard real-time
refresh guarantee.

Missing displays or failed transfers are logged and retried after 10 seconds;
there is no retry wait loop or reboot. The first failed transfer suppresses the
remaining frame transactions. Reconnection initializes and redraws the display.
A physically stuck shared bus can still affect other participants electrically.

Native tests cover enable/disable, live text changes, one-second refresh,
missing-display backoff/recovery, failed transfers, isolated row errors,
bus/address changes, timer wrap, strict settings parsing and version-1 migration.
The lab driver was confirmed on hardware by the user; production integration
still requires a hardware check after flashing.

## Boolean text overrides

Each selected Boolean source in the six-line editor has an expandable **Boolean text (optional)**
section with **True / On text** and **False / Off text**. For example, a `%s`
placeholder for an actuator may display `Zisterne` for true and `Hauswasser` for
false. Choose this mapping to match the actual valve wiring. The override belongs
to this source position in this display row: two occurrences of the same property
can use different labels. The underlying state, property metadata and MQTT output
are unchanged. Numeric and enum sources ignore Boolean overrides.

Each empty field independently falls back to the property's original text. Labels
are limited to 16 UTF-8 bytes, with no control characters; umlauts use two bytes.
The label length remains suited to the narrow OLED. Labels require a source. Literal `%` characters in labels are never parsed
as printf directives. HTML is escaped in both inputs and preview output.

Preview uses the edited labels without changing the OLED. Save on device persists
and applies them at the next display refresh, without restarting the driver.
Load saved settings restores both source references and labels.

Large web pages are sent as header, content and footer with a combined Content-Length,
without allocating a second complete page or a temporary concatenation of the body.
The Boolean help text is shared across all source fields to keep the editor compact.

## Display administration page

`/display` is the dedicated Display navigation item. It contains OLED hardware
settings, all six rows, preview, saving and loading. `/measurements` retains only
measurements and property diagnostics. Old Measurements URLs with preview fields
are still served by the Display handler; new forms target `/display`.

Boolean translation controls are initially shown only when property metadata
identifies the selected source as Boolean. Empty, unknown, numeric and enum
sources hide them. Changing a source updates visibility immediately using typed
metadata in the source inventory; without JavaScript, Preview refreshes it.
Hidden values are preserved, so switching source types does not silently erase
saved labels. Formatting continues to ignore Boolean labels for non-Boolean types.

## Locale-aware time and date

The Display source selector offers `system/1/date`, `system/1/time`, and
`system/1/datetime`. These are typed Text properties and use `%s`. TimePropertyReader
uses ITimeService's local civil time and the existing LocaleFormatter, respecting
the configured timezone/DST and locale (German, British or US date/time styles).
Until both synchronization and local time are valid it returns NoValue. A snapshot
owns its text; later reads never invalidate an earlier snapshot's string.
Combined date/time can exceed the OLED width; separate date and time rows remain
available. Formatting does not read or alter the hardware clock configuration.

## Actuator percentage

`actuator/ID/level` is available when that actuator exposes ILevelActuator. It is
an unsigned integer in percent, 0–100, rendered for example as `Level: %u%%`.
It reports the commanded logical output, not measured mechanical valve position.
Uninitialized output is NoValue; a pure On/Off relay does not advertise a level.
The existing Boolean `state` property and MQTT behavior remain unchanged.

## Controller state translations

Selecting an enum source such as `controller/ID/reason` reveals **State text
(optional)** with the source's named states. Enter an optional label for each
state; empty fields keep the original text. Selection changes update these fields
immediately. Time/text and numeric sources show neither Boolean nor state-label
editors. The page supports up to 32 nonempty state translations, each up to 16
UTF-8 bytes. Matching uses stable codes such as `hysteresis_hold`, never display
text or a numeric enum index. Labels remain specific to a row/source position.
They are literal text even when containing `%`; HTML is escaped. Preview and OLED
use the same formatter. None of these labels changes a controller decision.

The version-4 blob is capped at 6144 bytes and is written as one record before
updating the in-memory configuration. Failed writes retain previous settings.
An existing invalid new blob is not replaced by a stale old string on load.
Factory reset clears both formats. Existing Boolean labels, source references and
OLED connection settings survive migration.

### Retained controller decision

`controller/ID/decision` is an enum source for `%s`, with three stable codes:
`unknown`, `on`, `off`. The Display page offers the existing per-source text
translations for these three values (16 UTF-8 bytes each). For example, map the
appropriate decisions to `Ausreichend` and `Niedrig`, according to the configured
threshold direction. Empty translations use the default labels.

The decision is retained through the hysteresis band. Before the first threshold
crossing, after stop, or when the latest evaluation has missing, invalid or stale
input, the displayed decision is `unknown`. Source recovery restores the retained
decision, including within the hysteresis band. This presentation does not erase
the internal decision or change output behavior. It describes the controller's
logical decision, not successful valve movement. `reason` remains available for
detailed diagnostics. No storage migration or MQTT change is required.

### Configurable Values as display sources

Each live Value appears in the Display source list as `value/ID/state`, labelled
with its configured name. Use `%s`, for example `Modus: %s`. The default output is
the label of the selected option. Optional state translations use its stable code,
just like controller enum translations. The OLED picks up changes on its next
normal refresh; use Preview/refresh to update the Web representation.
Deleting the Value leaves the saved display reference intact and reports an
unknown source until that line is reconfigured. Reading a Value never changes it.
