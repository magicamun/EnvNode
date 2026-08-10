# 0012: Project identity is EnvNode

## Context

The project began as WeatherStation, but its architecture evolved into a reusable embedded platform. The original name became too narrow for the shared runtime, domain model, and hardware-resource model.

## Decision

The repository, project, and reusable platform are named EnvNode. WeatherStation remains the first application and use case built on EnvNode. Existing external APIs, including MQTT topics and Home Assistant stable identifiers, are intentionally not renamed by this decision.

## Consequences

Current platform terminology migrates to EnvNode. Historical ADRs remain unchanged and historically correct. Any MQTT or other external API migration is deferred to a separate, compatibility-aware decision.
