# EnvNode Hardware Documentation

This page is the entry point for the EnvNode hardware documentation and KiCad reference designs.

## Specifications and Guidelines

- [EnvNode Module Interface – Hardware Specification](EnvNode_Module_Interface_Hardware_Specification.md) defines the shared electrical and mechanical expansion-module interface.
- [Hardware Design Guidelines](hardware_guidelines.md) contains general design rules used across EnvNode hardware.

## KiCad Reference Designs

- [Daughterboard Module Interface Design Block](../hardware/kicad/DesignBlocks/EnvNode_Module_Interface_Daughterboard/README.md) documents the reusable module-side connector block, its pinout, connector geometry, routing baseline, and update workflow.

## Module Templates

The HalfSize and FullSize EmptyModule projects under `hardware/kicad/Modules/` are reusable starting points for module development. Their detailed documentation and formal reference status will be added after both templates have been updated from the validated daughterboard design block and have passed their final ERC, DRC, and geometry checks.
