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
The single-line formatter is a preview tool; there is no LCD driver, persistent
display configuration or MQTT change. Current pre-time-synchronization discard behavior remains:
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
form has its own bounded string-reference parser.

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

The resolver centralizes lookup for diagnostics and the single-line preview below.
It does not store display configuration or drive display hardware.

## Single-line text preview

Open **Measurements → Text line preview**, select one of the available sources,
enter a format and click **Preview / refresh**. The current value is resolved
through PropertyResolver each time. The source list includes supported Sensor
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

The formatter accepts exactly one conversion, plus literal text and `%%`.
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

PropertyTextFormatter validates the entire format and its compatibility with
the property before formatting. It invokes snprintf only with internal trusted
format patterns, never with the submitted format string. Unknown sources, missing
samples, invalid samples, type mismatches and output overflow produce explicit
errors; no partial or previous result is shown. No new staleness policy is added.

The form is a read-only GET request. Source and format remain in the URL and can
be bookmarked or refreshed; they are not saved in NVS. This is a preview tool,
not yet persistent display configuration. Source strings have the syntax
`category/ID/key`; the parser requires a known category, nonzero uint16 ID and
a bounded alphanumeric/underscore key. Runtime availability is checked separately
by the resolver. HTML output escapes source names, inputs and formatted text.

Tests cover formatting, bounds, percent handling, unsupported/unsafe format
syntax, type checks, non-finite and invalid values, missing/unknown sources,
reference parsing, HTML escaping, preview updates and no reads before submission.
The resolver integration test also formats real Sensor, Actor and Controller
values through the same entry point used by the page.
