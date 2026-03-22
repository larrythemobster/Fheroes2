# fheroes2 Native Map Converter Plan

## Purpose

This document is the implementation checklist for making the native legacy map converter fully functional and reliable.

Target workflow:

1. Load legacy map-family files through engine code
2. Convert live engine state into FH2M
3. Save FH2M with enough metadata to round-trip correctly

Primary implementation areas:

- `src/fheroes2/game/fheroes2.cpp`
- `src/fheroes2/maps/map_format_helper.cpp`
- `src/fheroes2/maps/map_object_info.h`
- `src/fheroes2/maps/map_object_info.cpp`
- `src/fheroes2/maps/maps_fileinfo.h`
- `src/fheroes2/maps/maps_fileinfo.cpp`
- `src/fheroes2/world/world_loadmap.cpp`
- `src/fheroes2/world/world.h`
- `src/fheroes2/maps/map_format_info.h`
- `src/fheroes2/maps/map_format_info.cpp`

## Current Status Summary

### Already Present

- [x] CLI entry point for `--convert <directory>` exists in `src/fheroes2/game/fheroes2.cpp`
- [x] Reverse object mapping helper `getObjectGroupAndIndex(...)` exists
- [x] `Heroes::getHeroMetadata()` exists
- [x] `Castle::getCastleMetadata()` exists
- [x] `Maps::MapFormatHelper::saveMap(const fheroes2::World &, const std::string &)` exists
- [x] Legacy map header parsing already exists via `Maps::FileInfo::readMP2Map(...)`
- [x] Campaign gameplay overlay logic already exists elsewhere in campaign code

### Not Finished

- [x] Save bridge uses stale or invalid `ObjectGroup` names
- [x] Save bridge uses invalid `playerRace.assign(...)` on a fixed-size array
- [x] Converter always passes `true` to `LoadMapMP2`, which is wrong for some expansion files
- [x] Converter does not seed `Settings::currentMapInfo` before loading
- [x] Scenario metadata from `Maps::FileInfo` is transferred into FH2M
- [x] Multi-tile object anchor handling is safe enough for tested maps
- [x] Several FH2M metadata buckets are exported; remaining gaps are mostly broader validation or engine limitations
- [x] Rumors and daily events are exported
- [x] Campaign-specific overlays are applied during conversion
- [x] Temporary converter-specific debug print noise has been reduced to normal logging or removed

## Recommended Architecture

These design constraints should guide implementation so the converter stays small, deterministic, and easy to validate.

### A. Build a thin `ConversionContext` first

The converter should stop passing state around implicitly through the CLI loop.

Create one small per-file struct that owns all conversion inputs and decisions.

Suggested fields:

- [x] `sourcePath`
- [x] file extension and/or version
- [x] `Maps::FileInfo fileInfo`
- [x] whether load should run in SW-compatible or PoL-compatible mode
- [x] whether campaign overlays should apply
- [x] loaded `fheroes2::World`

Suggested responsibilities:

- [x] Construct context from source file path
- [x] Read `Maps::FileInfo` once
- [x] Decide compatibility mode once
- [x] Decide campaign policy once
- [x] Seed `Settings::currentMapInfo` from the context
- [x] Pass the context into later conversion stages instead of recomputing decisions in multiple places

Benefits:

- [x] Removes accidental CLI-loop state coupling
- [x] Makes `saveMap(...)` deterministic enough for tested maps
- [x] Keeps file-type decisions out of world/object serialization logic

### B. Mirror the FH2M load path instead of inventing a separate save model

The best specification for what the save path must output is the existing FH2M loader.

Implementation rule:

- [x] Treat `world_loadmap.cpp` FH2M load logic as the canonical contract for what the save path must emit
- [x] Prefer writing metadata exactly in the form expected by `loadResurrectionMap(...)`
- [x] Do not invent parallel abstractions unless the loader already expects them

Primary references:

- [x] `world_loadmap.cpp` load path around tile/object handling
- [x] `maps_fileinfo.cpp` legacy header parsing
- [x] `map_format_helper.cpp` current save bridge entry point

Benefits:

- [x] Reduces reasoning overhead
- [x] Makes round-trip behavior easier to validate
- [x] Aligns save behavior with existing engine expectations

### C. Split conversion into two explicit transforms

The converter should be structured as two separate steps.

#### 1. `Maps::FileInfo -> FH2M base map metadata`

This step owns:

- [x] map name
- [x] description
- [x] difficulty
- [x] player colors
- [x] human/computer availability
- [x] races
- [x] alliances
- [x] victory/loss conditions
- [x] campaign-related policy decisions

Reference:

- [x] `maps_fileinfo.cpp` legacy map header parsing

#### 2. `World -> FH2M tile/object metadata`

This step owns:

- [x] terrain
- [x] tile objects
- [x] anchor detection
- [x] object metadata buckets
- [x] rumors/daily events if sourced from world state

Reference:

- [x] `map_format_helper.cpp` current save bridge

Benefits:

- [x] Clean boundary between static scenario metadata and live world state
- [x] Easier testing
- [x] Easier refactoring of the save bridge later

### D. Implement save-side object metadata by `objectType`, not guessed group names

Do not branch on invented group names for signs, events, resources, or sphinxes.

Implementation rule:

- [x] Use the real `ObjectGroup` enum only
- [x] Once group is known, inspect `objectType` to decide which metadata bucket to fill
- [x] Mirror the existing load-side `switch` structure as closely as possible

Benefits:

- [x] Avoids stale enum mismatches
- [x] Keeps save behavior aligned with the loader
- [x] Prevents object classification drift over time

### E. Add one dedicated helper for anchor detection

Do not keep anchor placement logic inline inside `saveMap(...)`.

Add one focused helper such as:

- [x] `isAnchorTileForUid(...)`
- [x] or `findAnchorTileForUid(...)`

Responsibilities:

- [x] Determine the canonical tile that should emit a logical object
- [x] Ensure only one `TileObjectInfo` is created per logical object UID
- [x] Keep UID dedup logic secondary to anchor detection, not the other way around

Benefits:

- [x] Makes the riskiest placement logic testable in isolation
- [x] Prevents “first encountered tile wins” bugs
- [x] Makes multi-tile object handling easier to review

### F. Use this architectural order during implementation

- [x] Build `ConversionContext`
- [x] Implement `FileInfo -> FH2M base map metadata`
- [x] Implement anchor helper
- [x] Refactor `World -> FH2M tile/object metadata` around anchor detection
- [x] Mirror load-side objectType handling for metadata extraction
- [x] Add rumors/daily events after core object round-trip works

## Definition Of Done

- [x] Converter compiles cleanly
- [x] MP2-family files load with correct compatibility mode
- [x] Current map info is initialized before map load
- [x] FH2M base map metadata matches source map metadata for tested MP2/H2C maps
- [ ] Large multi-tile objects round-trip without drifting
- [ ] Ownership-sensitive objects preserve owner color across a broader validation set
- [ ] Specialized metadata buckets survive round-trip across a broader validation set
- [ ] Rumors and daily events survive round-trip across a broader validation set
- [x] Campaign files are supported for tested `.h2c` input
- [x] Converted FH2M files load in-engine and behave like the original maps for tested MP2/H2C inputs

## Phase 1: Fix Structural Blockers

### 1. Reconcile invalid `ObjectGroup` usage in save bridge

Problem:

- The save code refers to groups that do not exist in the real enum:
	- `ADVENTURE_SIGNS`
	- `ADVENTURE_EVENTS`
	- `ADVENTURE_SPHINX`
	- `ADVENTURE_MONSTERS`
	- `ADVENTURE_RESOURCES`

Reality:

- The actual enum contains:
	- `ADVENTURE_MISCELLANEOUS`
	- `ADVENTURE_TREASURES`
	- `ADVENTURE_WATER`
	- `MONSTERS`

Tasks:

- [x] Update `Maps::MapFormatHelper::saveMap(...)` to use the real `ObjectGroup` enum
- [x] Detect signs, events, sphinxes, bottles, resources, and similar objects by `objectType`, not by fake group names
- [x] Use `ADVENTURE_MISCELLANEOUS` for sign/event/sphinx/jail-style objects
- [x] Use `MONSTERS` for monster objects
- [x] Use `ADVENTURE_TREASURES` plus `objectType == MP2::OBJ_RESOURCE` for resources

Validation:

- [x] No stale enum names remain in `map_format_helper.cpp`
- [x] Save code builds against the actual enum in `map_object_info.h`

### 2. Fix `playerRace` handling

Problem:

- The save code calls `map.playerRace.assign(...)`, but `playerRace` is a `std::array<uint8_t, 6>`.

Tasks:

- [x] Remove invalid `.assign(...)` usage
- [x] Fill `playerRace` as a fixed-size array
- [x] Prefer populating races from `Maps::FileInfo::races` instead of hardcoded defaults

Validation:

- [x] No invalid array-as-vector APIs remain

### 3. Use correct load mode for each file type/version

Problem:

- The converter currently always calls `world.LoadMapMP2(inputFile, true)`
- That can reject Price of Loyalty objects when it should not

Tasks:

- [x] Read `Maps::FileInfo` before calling `LoadMapMP2`
- [x] Determine whether the file should be treated as SW-compatible or PoL-compatible
- [x] Pass the correct boolean into `LoadMapMP2`
- [x] Verify `.mx2` and `.hxc` do not go through a SW-only path

Validation:

- [x] SW map loads successfully
- [x] PoL map loads successfully
- [x] Converter does not incorrectly reject expansion objects for tested `.h2c`

## Phase 2: Initialize Runtime State Correctly

### 4. Seed `Settings::currentMapInfo` before loading each map

Problem:

- `World::LoadMapMP2` and related post-load logic consult `Settings::Get().getCurrentMapInfo()`
- The converter path does not currently initialize this state first

Tasks:

- [x] For each input file, construct `Maps::FileInfo` via `readMP2Map(...)`
- [x] Call `Settings::Get().setCurrentMapInfo(fi)` before `LoadMapMP2`
- [x] Ensure this happens for every file in the batch loop
- [x] Decide whether to clear or overwrite the map info between iterations

Validation:

- [x] Artifact-victory maps exclude the win artifact correctly during load
- [x] Hero/town win-loss condition resolution uses the right map info in tested maps

## Phase 3: Transfer Scenario Metadata Into FH2M

### 5. Populate map base metadata from `Maps::FileInfo`

Required fields to set on `Maps::Map_Format::MapFormat`:

- [x] `name`
- [x] `description`
- [x] `difficulty`
- [x] `availablePlayerColors`
- [x] `humanPlayerColors`
- [x] `computerPlayerColors`
- [x] `playerRace`
- [x] `victoryConditionType`
- [x] `isVictoryConditionApplicableForAI`
- [x] `allowNormalVictory`
- [x] `victoryConditionMetadata`
- [x] `lossConditionType`
- [x] `lossConditionMetadata`
- [x] `alliances`
- [x] `isCampaign` only if campaign-specific data is actually preserved

Implementation notes:

- [x] Use `Maps::FileInfo::readMP2Map(...)` as the source of truth for legacy header metadata
- [x] Translate MP2-style condition coordinates into FH2M tile-index metadata where needed
- [x] Preserve player availability exactly, not as hardcoded `0x3F`

Validation:

- [x] Converted FH2M reports the same map name in tested maps
- [x] Converted FH2M reports the same description in tested maps
- [x] Converted FH2M reports the same difficulty in tested maps
- [x] Player colors/races match the source map in tested maps

### 6. Convert special victory/loss conditions correctly

Tasks:

- [x] Defeat everyone: no special metadata
- [x] Capture town: save target tile index and owner color
- [x] Kill hero: save target tile index and owner color
- [x] Obtain artifact: save artifact id
- [x] Defeat other side: save alliances
- [x] Collect enough gold: save gold target
- [x] Loss town: save target tile index and owner color
- [x] Loss hero: save target tile index and owner color
- [x] Loss out of time: save day limit

Validation:

- [x] Converted FH2M validates correctly through loader-side special-condition checks in tested maps

## Phase 4: Fix Multi-Tile Object Placement

### 7. Save objects only from their anchor tile

Problem:

- Current UID dedup saves the first encountered part, not necessarily the anchor tile
- This can shift objects and break metadata lookup by tile index

Tasks:

- [x] Replace current first-hit UID dedup logic with anchor-aware logic
- [x] Define an anchor rule based on the tile main object part and object UID
- [x] Emit exactly one `TileObjectInfo` per logical object at the anchor tile
- [x] Keep UID dedup only after anchor validation

Validation:

- [x] Castles round-trip in the same location in tested maps
- [x] Heroes round-trip in the same location in tested maps
- [ ] Large trees/mountains do not drift
- [ ] Multi-tile decorations do not float or collapse

## Phase 5: Export Object Metadata Buckets Completely

### 8. Castle metadata

- [x] `Castle::getCastleMetadata()` already exists
- [x] Confirm save bridge associates castle metadata with the correct UID and anchor tile for tested maps
- [x] Confirm town color and castle/tent state survive round-trip for tested maps

### 9. Hero metadata

- [x] `Heroes::getHeroMetadata()` already exists
- [x] Confirm save bridge associates hero metadata with the correct UID and anchor tile for tested maps
- [x] Confirm jailed heroes are handled separately where needed

### 10. Sign and bottle metadata

Tasks:

- [x] Export `MapSign` text for sign objects under `ADVENTURE_MISCELLANEOUS`
- [x] Export bottle messages for bottle objects under `ADVENTURE_WATER`
- [x] Preserve language information where possible

Validation:

- [ ] Sign text survives round-trip
- [ ] Bottle text survives round-trip

### 11. Adventure event metadata

Tasks:

- [x] Export `message`
- [x] Export `humanPlayerColors`
- [x] Export `computerPlayerColors`
- [x] Convert runtime `isSingleTimeEvent` into FH2M `isRecurringEvent` correctly
- [x] Export `artifact`
- [x] Export `artifactMetadata` for spell scroll rewards
- [x] Export `resources`
- [x] Export `experience`
- [x] Export `secondarySkill`
- [x] Export `secondarySkillLevel`
- [x] Export primary stat rewards if represented in runtime state
- [x] Export monster reward fields if represented in runtime state
- [x] Export conditional event triggers and map logic represented by engine runtime objects
- [x] Document that separate AI queues / behavioral directives are not represented by current runtime or FH2M load logic

Validation:

- [ ] Adventure events behave the same after FH2M reload

Notes:

- [x] Legacy/runtime event logic that actually exists in engine structures is preserved: message, allowed human/computer colors, single-use vs recurring behavior, resources, artifact reward, experience, secondary skill reward, and FH2M primary-stat / monster reward fields.
- [x] There is no separate runtime representation for arbitrary scripted triggers, AI queues, or custom behavioral directives beyond the normal event/player-color gating already stored in `MapEvent` / `EventDate` and consumed by gameplay code.
- [x] Because the FH2M loader also has no dedicated AI-queue/script ingestion path, those concepts cannot be round-tripped without adding new engine/runtime support rather than converter-only changes.

### 12. Sphinx metadata

Tasks:

- [x] Export `riddle`
- [x] Export `answers`
- [x] Export `resources`
- [x] Export `artifact`
- [x] Export `artifactMetadata` when the reward is a spell scroll

Validation:

- [ ] Sphinx question, answers, and reward survive round-trip

### 13. Monster metadata

Tasks:

- [x] Use `MONSTERS` group, not a fake adventure subgroup
- [x] Export monster count
- [x] Export selected pool for random monster variants if available

Validation:

- [ ] Normal monster stacks preserve count
- [ ] Random monster selections preserve behavior where applicable

### 14. Resource metadata

Tasks:

- [x] Detect resources as `ADVENTURE_TREASURES` objects with `objectType == MP2::OBJ_RESOURCE`
- [x] Export resource count correctly

Validation:

- [ ] Loose resource piles preserve amount

### 15. Capturable ownership metadata

Problem:

- Loader applies ownership via `capturableObjectsMetadata`
- Current save bridge does not appear to export it

Tasks:

- [x] Detect capturable objects like mines, sawmills, alchemist labs, and lighthouses
- [x] Read current owner color from world state
- [x] Write `map.capturableObjectsMetadata[uid].ownerColor`

Validation:

- [ ] Mines preserve owner color after round-trip
- [ ] Lighthouses preserve owner color after round-trip

### 16. Artifact metadata

Tasks:

- [x] Export metadata for random ultimate artifact radius
- [x] Export spell scroll spell selection

Validation:

- [ ] Random ultimate artifact radius survives round-trip
- [ ] Spell scrolls preserve selected spell

### 17. Selection metadata

Tasks:

- [x] Export pyramid spell selection metadata
- [x] Export witch hut skill selection metadata
- [x] Export shrine spell selection metadata
- [x] Export any random-monster selection metadata if represented in FH2M

Validation:

- [ ] Pyramids reload with correct spell options
- [ ] Witch huts reload with correct skill options
- [ ] Shrines reload with correct spell options

## Phase 6: Export Global Map Content

### 18. Daily events

Tasks:

- [x] Add or use read access to world daily events
- [x] Convert `EventDate` into `Maps::Map_Format::DailyEvent`
- [x] Preserve message, player applicability, repeat period, first day, and resources

Validation:

- [ ] Daily events survive round-trip

### 19. Rumors

Tasks:

- [x] Add or use read access to world rumors
- [x] Export rumor strings into `map.rumors`

Validation:

- [ ] Tavern rumors survive round-trip

### 20. Language / translation data

Tasks:

- [x] Set `map.mainLanguage` sensibly
- [x] Preserve text content in the main language
- [x] Optionally populate translation buckets if it improves consistency

Validation:

- [ ] Signs, sphinxes, events, rumors, and daily events show correct text on FH2M reload

## Phase 7: Campaign Handling

### 21. Decide campaign conversion scope

Questions to resolve in implementation:

- [x] Should `.h2c` and `.hxc` conversion produce plain scenario FH2M only?
- [x] Or should it preserve campaign-specific gameplay overlays too?

### 22. If campaign-aware output is required

Tasks:

- [x] Determine campaign and scenario identity during conversion
- [x] Apply `CampaignData::updateScenarioGameplayConditions(...)` where appropriate
- [x] Preserve any scenario-specific alliance or rule modifications where available from `Maps::FileInfo`

Validation:

- [x] Campaign scenarios behave like the original when loaded from converted FH2M for tested `.h2c`

### 23. If campaign-aware output is not required

Tasks:

- [x] Restrict or document campaign conversion explicitly
- [x] Mark unsupported campaign metadata in CLI output or documentation

Current scope:

- [x] Campaign scenario conversion preserves the scenario-level rules and overlays that are represented through `Maps::FileInfo` and campaign gameplay condition updates.
- [x] Any campaign behavior not represented in `Maps::FileInfo`, live `World`, or FH2M loader structures is outside converter scope and would require engine support first.

## Phase 8: Cleanup

### 24. Remove debug print noise from FH2M serialization/deserialization

Tasks:

- [x] Remove temporary `std::cout` debug traces from `map_format_info.cpp`
- [x] Keep only proper logging if needed

Validation:

- [x] FH2M load/save no longer spams converter-specific debug output

## Suggested Implementation Order

- [x] Step 1: Read `Maps::FileInfo` before map load
- [x] Step 2: Seed `Settings::currentMapInfo`
- [x] Step 3: Pass correct compatibility mode into `LoadMapMP2`
- [x] Step 4: Reconcile stale `ObjectGroup` usage in save bridge
- [x] Step 5: Fix `playerRace` handling
- [x] Step 6: Populate all base map metadata from `FileInfo`
- [x] Step 7: Fix anchor-tile object emission
- [x] Step 8: Fill missing metadata buckets supported by current engine/runtime
- [x] Step 9: Export rumors and daily events
- [x] Step 10: Add campaign-aware handling or explicitly scope it out
- [x] Step 11: Remove temporary debug prints
- [ ] Step 12: Run round-trip validation set

## Validation Matrix

### Required Test Inputs

- [x] Small SW map with default rules
- [x] PoL map with expansion-only objects
- [ ] Map with custom heroes
- [ ] Map with custom castles
- [ ] Map with special victory/loss rules
- [ ] Map with signs, bottles, sphinxes, and adventure events
- [ ] Map with owned mines/lighthouses
- [ ] Map with shrines, witch huts, pyramids, and random monsters
- [ ] Map with rumors and daily events
- [x] Campaign file if campaign conversion remains enabled

### Round-Trip Checks Per Map

- [x] Terrain visuals match for tested maps
- [x] Object positions match for tested maps
- [x] Castle placement matches for tested maps
- [x] Hero placement matches for tested maps
- [x] Player setup matches for tested maps
- [ ] Mine ownership matches
- [ ] Resource counts match
- [ ] Monster counts match
- [ ] Sign text matches
- [ ] Sphinx content matches
- [ ] Adventure events match
- [ ] Rumors match
- [ ] Daily events match
- [x] Win/loss rules match for tested maps

## Agent Mode Instructions

When implementing, do not start by patching `saveMap(...)` in isolation.

Do this in order:

1. Read `Maps::FileInfo` from the input file
2. Set `Settings::currentMapInfo`
3. Decide correct `LoadMapMP2` mode
4. Fix stale `ObjectGroup` usage
5. Populate base scenario metadata
6. Fix anchor logic
7. Add missing metadata buckets
8. Add rumors and daily events
9. Handle campaign overlay policy
10. Validate by round-tripping converted FH2M files

## Notes

- The architecture is still correct in spirit: load through engine code, map through reverse object identity, and save through FH2M structures.
- The remaining work is mostly data completeness and correctness, not a total redesign.
- The two highest-risk areas are object anchoring and scenario metadata translation.
