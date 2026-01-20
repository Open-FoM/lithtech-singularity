# DEdit2 Agents

We are rebuilding `tools/DEdit` — the original World creation tools for the LithTech Jupiter engine — as `tools/DEdit2`.

**Purpose**
- Preserve and modernize the classic DEdit workflow used to build LithTech worlds.
- Keep behavior and data formats compatible with the original Jupiter-era toolchain.
- Document decisions, reverse‑engineering notes, and implementation status in a single place.

**Scope**
- Editor UI, data models, and export pipeline for world/level creation.
- Import/export of legacy formats where possible.
- Parity with core DEdit features first; enhancements follow once fidelity is solid.

**Non‑goals**
- Replacing the runtime engine or asset pipeline.
- Changing world formats or breaking compatibility without an explicit decision.

**Working approach**
- Start from behavior parity and reproducibility.
- Prefer documented, testable behavior over assumptions.
- Capture unknowns and TODOs inline with rationale.

**Undo/redo expectations**
- The undo stack already supports a Move action (reparenting) but the UI does not yet trigger it.
- When adding tree drag/drop or other reparenting behavior, make sure to record Move actions so Undo/Redo works.

