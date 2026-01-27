# DEdit2 Authoring Tools Port Shortlist (Legacy DEdit)

## Scope
Prioritized shortlist of legacy DEdit authoring features to port into DEdit2, focused on core world authoring workflows.

## Priority 1: Core World Authoring (Foundational)
These unlock the basic brush/object workflow and are prerequisites for most other authoring tools.

### Brush & Geometry Authoring
- **Add primitives**: Box, Cylinder, Pyramid, Sphere, Dome, Plane (centered on marker)
- **Edit modes**: Brush edit mode, Geometry edit mode, Object edit mode
- **Transform**: Rotate selection, Scale selection, Mirror X/Y/Z, Scale all geometry
- **Geometry cleanup**: Cleanup geometry, Remove extra edges, Join tagged vertices
- **Carve**: Use selection as carving brush
- **Vertex tools**: Snap vertices
- **Patch editing**: Patch edit mode, Set patch resolution

### Selection & Node Ops
- **Selection modes**: Single-node, Multi-node
- **Selection commands**: Select all, Select none, Select inverse
- **Grouping**: Group selection (create container), Select container
- **Visibility**: Hide selected, Unhide selected, Hide inverse, Hide node/children, Unhide all
- **Freeze**: Freeze selected, Unfreeze selected, Unfreeze all
- **Node actions**: Rename, Set active parent, Move tagged nodes here, Go to next tagged node

### Object Placement & Scene Tools
- **Object browser** (add/inspect objects in world)
- **Bind to object** (attach brush selection to object)
- **Camera tools**: Camera to object, Center selection on marker
- **Marker tools**: Place marker at camera, Center views on marker
- **Object selection filter**

### Prefab Workflows (Core)
- **Save selection as prefab**
- **Disconnect prefab**
- **Replace selected prefabs**
- **Select prefab in list**

### Viewport & View Controls (Authoring-Critical)
- **View modes**: Perspective, Top, Bottom, Front, Back, Left, Right
- **Shade modes**: Wireframe, Flat, Textured, Lightmaps-only
- **Overlays**: Show grid, Show marker, Show normals, Show objects, Show class icons
- **Layout**: Maximize active view, Reset view

## Priority 2: Authoring Adjacent (Next Wave)
These improve usability but can follow once core workflows are stable.

### Texture/UV Authoring (Core Adjacent)
- **UV tools**: Map texture coordinates, Reset texture coords, Fit texture to poly, Map texture to view
- **Texture ops**: Apply texture, Remove texture, Select texture, Replace textures in selection

### Navigation & Productivity
- **Navigator**: Store positions, Go to next/previous, Organize positions
- **Advanced selection**: Advanced selection dialog, filters

### World Maintenance
- **World info**
- **World processing** (basic hooks to processing pipeline)

## Dependencies & Notes
- **Brush/geometry tools** require stable brush representation, mesh editing, and undo/redo integration.
- **Prefab tools** require robust serialization and instancing semantics in DEdit2.
- **Selection/visibility/freeze** need consistent scene node filtering and viewport render filtering.
- **Viewport modes** depend on camera + renderer support (already partially present in DEdit2).

## Suggested Port Order
1. Primitives + selection basics + edit modes
2. Transform tools + visibility/freeze
3. Prefab save/disconnect/replace
4. View mode + shade mode parity
5. Geometry cleanup + carve + patch tools
6. UV/texture authoring tools (if material pipeline ready)
