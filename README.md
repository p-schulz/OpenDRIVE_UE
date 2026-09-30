# OpenDRIVE_UE
Unreal Engine 5.6 plugin for importing, creating and editing ASAM OpenDRIVE (.xodr) road networks.

## Modules

- **OpenDrive** (Runtime): the shared OpenDRIVE data model (`FOpenDriveMap`, geometry/elevation/
  superelevation/lane evaluation, routing) and the `UOpenDriveAsset` it is packaged in. This is the
  canonical implementation of OpenDRIVE for the Unreal ecosystem: other plugins that process `.xodr`
  files (e.g. [OpenSCENARIO_UE](https://github.com/p-schulz/OpenSCENARIO_UE)) depend on this module
  instead of carrying their own copy.
- **OpenDriveEditor** (Editor): a dedicated "OpenDRIVE" editor tool mode with dockable, independently
  positionable windows:
  - **Road List** — browse, select, add, duplicate and remove roads; edit the name/start pose/length of
    simple straight roads.
  - **Elevation** and **Superelevation** — 2D graph editors for the vertical/banking profile of the
    selected road (draggable control points; values are stored as the exact OpenDRIVE cubic-polynomial
    segments the format expects).
  - Import/export of `.xodr` files, asset creation from scratch, and a content-browser asset definition
    for `UOpenDriveAsset`.

## Working together with OpenSCENARIO_UE

Both plugins install side by side in a project's `Plugins/` folder. OpenSCENARIO_UE declares a plugin
dependency on OpenDrive and uses `UOpenDriveAsset`/`FOpenDriveMap` from it for road networks referenced by
scenarios; its own editor mode gained an "Edit Road Network" button that opens the active scenario's road
network directly in this plugin's Road List tab.

## Tests

`Plugins/OpenDrive/Tests/Standalone/run_tests.sh` compiles the plugin's engine-independent runtime code
(the whole `OpenDrive` module) against a small mock of the required Unreal Engine types and runs it —
no Unreal Engine installation required. It exercises parsing, geometry/elevation/superelevation
evaluation, the XML writer round trip, and every road/lane model-editing operation used by the editor
tool mode.
