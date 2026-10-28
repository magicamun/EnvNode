# EnvNodeIdentity

EnvNode-owned materials follow the repository LICENSE
(LicenseRef-EnvNode-Use-and-Redistribution-1.0).

Portable hardware identity records and verified provisioning storage, shared by
EnvNode and future applications. Existing header names and namespace EnvNode
are retained. No Arduino, Wire, board runtime, web or sensor dependencies.

- BoardIdentityTypes: stable profile IDs and revision data.
- BoardIdentity/Codec/Store: supported identity validation and 32-byte ENID records.
- BoardProvisioningService: explicit confirmation and verified write/readback.
- CompactCborCodec and HardwareDescriptor models/codecs: descriptor representation.
- HardwareDescriptorEnvelope/Store: CRC-protected redundant banks and generations.
- IIdentityStorage (also IBoardIdentityStorage): injected read/write transport.

The application retains active-board selection/fallback, bus selection and EEPROM
transport, module discovery, firmware compatibility checks and restart policy.
Moving these files does not alter supported revisions, validation, record layout,
EEPROM addresses, bank selection, write order or readback verification.

The library also owns pure ModuleSlot IDs/names, compatibility/provisioning result
types and status names, InstanceUuid's version-4 bit conversion and the deterministic
DuoRelay descriptor encoder. Compatibility evaluation, live module rediscovery and
the module-provisioning policy service remain in the EnvNode application. Existing
service headers include the shared result types for source compatibility.

`boardIdentityStatusName` is shared here with the status type; the application
resolver retains only source selection/status naming specific to resolution.
