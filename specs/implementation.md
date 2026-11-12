# Vehicle Control Framework: behavior and implementation

Describes the retained payload **0.1.0**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.1.0** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.1.0** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

Brokers vehicle input for up to sixteen providers and sixty-four live cars. Hardware is evaluated first; highest priority wins with stable order on ties. Meaningful manual steer/throttle/brake/boost revokes a per-car exclusive assist lease, while neutral hardware permits an assist. Before each callback, prior injections are restored only where still owned, so every provider sees the same native baseline. Invalid/nonfinite proposals are rejected.

## State and lifecycle

Leases, rooted cars and remembered input fields are session state. Accepted input updates native throttle/steering/brake/boost and presentation before Vehicle Framework physics. An owning assist may request one-use unattended native physics and optional cleaning; requests must be renewed by a valid proposal and release on takeover/loss. Providers still own permissions, module ownership and route safety.

## Integration and configuration

Runtime dependencies: [vehicle-framework](https://github.com/swooping-mods/vehicle-framework).

The [module metadata](../source/mods/vehicle-control-framework/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Requires `vehicle-framework.autonomous` | 1 | at least 48 bytes |
| Provides `vehicle-control-framework.controls` | 1 | 72 bytes |

## Implementation sources

- [mod.c](../source/mods/vehicle-control-framework/mod.c)

## Verification boundary

No navigation planner or ownership grant. Unattended support excludes unsupported custom physics/hover and requires the genuine native owner/world/body. Input arbitration, restoration and consumer safety need matching game evidence.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
