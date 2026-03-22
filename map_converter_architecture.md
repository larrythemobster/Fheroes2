# fheroes2 Internal Map Converter Architecture

This document details the architecture of the native `.mp2` to `.fh2m` conversion utility integrated directly into the fheroes2 game engine. This approach replaces the standalone converter with a more robust, "aware" system that leverages the engine's internal logic.

## 1. Native Data Flow

The conversion process follows a three-stage "Load -> Map -> Save" pipeline:

1.  **Stage 1: Native Loading (`World::LoadMapMP2`)**
    The engine's existing combat-tested loader is used to parse legacy `.mp2` files. This ensures that every nuance of the 16-bit binary format (tiles, UIDs, quantities, castle layouts) is correctly interpreted into the engine's `World` object.
    - **Benefit**: Zero duplication of parsing logic; if the map loads in-game, it can be converted.
    - **UID Handling**: Object UIDs are preserved, which is critical for linking external metadata (Signs, Events) to the physical object on the map.

2.  **Stage 2: Reverse Object Mapping (`getObjectGroupAndIndex`)**
    The engine stores objects as "Parts" (individual sprites/ICN entries), while the modern `.fh2m` format needs "Objects" (Editor groups and indices).
    - **Mechanism**: A reverse lookup map in `map_object_info.cpp` identifies the `ObjectGroup` and `index` for any given game sprite (`icnType` + `icnIndex`).
    - **Visual Accuracy**: By using the `icnIndex`, the converter can distinguish between different visual variants of the same object (e.g., a "Grass Tree" vs a "Snow Tree"), ensuring the converted map is visually identical to the original.

3.  **Stage 3: Metadata Extraction & Serialization**
    Rich gameplay data is extracted directly from the live engine objects.
    - **Heroes**: `Heroes::getHeroMetadata()` extracts primary/secondary skills, artifacts, and spells.
    - **Castles**: `Castle::getCastleMetadata()` extracts built buildings and custom names.
    - **Adventure Objects**: `Maps::MapFormatHelper::saveMap` performs `dynamic_cast` on engine objects (like `MapSign` or `MapEvent`) to extract localized text and rewards.
    - **Final Step**: The `Maps::MapFormatHelper::saveMap` bridge populates the `MapFormat` struct and invokes `Maps::Map_Format::saveMap` to write the final `.fh2m` file.

## 2. Key Integration Points

### Reverse Identity Map
Located in [map_object_info.cpp](file:///c:/Users/DehKu/OneDrive/Documents/Code/fheroes2/src/fheroes2/maps/map_object_info.cpp), the `objectGroupIndexByIcn` map is populated during engine initialization. It provides the "identity" of a sprite, allowing the converter to know exactly which Editor object corresponds to what the engine is rendering.

### Terrain Preservation
The converter uses `tile.getTerrainImageIndex()` and `tile.getTerrainFlags()` directly. Since `fheroes2` uses the original HoMM2 tileset indices, this ensures that even complex, hand-drawn terrain transitions are preserved 100% accurately without needing a separate mapping table.

### CLI Integration
The [fheroes2.cpp](file:///c:/Users/DehKu/OneDrive/Documents/Code/fheroes2/src/fheroes2/game/fheroes2.cpp) entry point is extended with a `--convert <directory>` flag. This triggers a batch processing loop that:
1. Scans for `.mp2`, `.mpx`, `.h2c`, `.mx2`, and `.hxc` files.
2. Loads them into the engine's internal `World` state.
3. Invokes the save bridge to generate `.fh2m` outputs.

## 3. Advantages over Standalone Converter

*   **High Fidelity**: Leveraging engine classes for Heroes and Castles ensures that no gameplay data is lost.
*   **Logic Reuse**: Automatically benefits from engine bugfixes in map loading or object representation.
*   **Accuracy**: Correctly handles multi-tile object anchoring by using the engine's UID-based "Main Part" logic, avoiding the "floating object" issues of simpler converters.
