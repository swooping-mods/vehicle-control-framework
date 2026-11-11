# Vehicle Control Framework: behavior and implementation

Describes the exported implementation **0.1.0**. The linked source and metadata define this version; gameplay qualification is a separate requirement.

## Behavior

Brokers vehicle input for up to sixteen providers and sixty-four live cars. Hardware is evaluated first; highest priority wins with stable order on ties. Meaningful manual steer/throttle/brake/boost revokes a per-car exclusive assist lease, while neutral hardware permits an assist. Before each callback, prior injections are restored only where still owned, so every provider sees the same native baseline. Invalid/nonfinite proposals are rejected.

## State and lifecycle

Leases, rooted cars and remembered input fields are session state. Accepted input updates native throttle/steering/brake/boost and presentation before Vehicle Framework physics. An owning assist may request one-use unattended native physics and optional cleaning; requests must be renewed by a valid proposal and release on takeover/loss. Providers still own permissions, module ownership and route safety.

## Integration and configuration

Runtime dependencies: [vehicle-framework](https://github.com/swooping-mods/vehicle-framework).

The [module metadata](../source/mods/vehicle-control-framework/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Provides `vehicle-control-framework.controls` | 1 | 72 bytes |
| Requires `vehicle-framework.autonomous` | 1 | at least 48 bytes |

## Implementation sources

- [mod.c](../source/mods/vehicle-control-framework/mod.c)

## Verification boundary

No navigation planner or ownership grant. Unattended support excludes unsupported custom physics/hover and requires the genuine native owner/world/body. Input arbitration, restoration and consumer safety need matching game evidence.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
