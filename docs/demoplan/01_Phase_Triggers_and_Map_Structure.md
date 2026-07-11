# 01 — Phase Triggers and Map Structure

## Dependencies

- None upstream — this is the foundational file. Everything else in this plan depends on the classes defined here, particularly `EMissionPhase`, `AMissionStateManager`, `AShooterGameState`, and `AZoneDefinition`.
- Read alongside [08_Multiplayer_and_Networking.md](08_Multiplayer_and_Networking.md) — every class introduced here is designed server-authoritative from the start; do not build the single-player version first and retrofit replication later.

---

## 1. Phase State Machine

`CLAUDE.md` already names the mission state machine and its transition list under "Mission State Machine". This section makes it concrete and buildable. No state machine class currently exists in `Source/ShooterGame/` — this is a **[CREATE]**.

### 1.1 `EMissionPhase` enum

New file: `Source/ShooterGame/Types/MissionTypes.h`

```cpp
UENUM(BlueprintType)
enum class EMissionPhase : uint8
{
    PreMission,
    Phase1Infiltration,
    Resupply1,
    Phase2Objective,
    Phase3Exfil,
    Resupply2,
    Phase4SecondObjective,
    Phase4Exfil,
    PostMission
};
```

This is a plain `uint8` enum, not a `FGameplayTag` — phase order is fixed and linear, so an enum with `static_cast<uint8>` comparisons is simpler and cheaper to replicate than a tag container. GameplayTags remain the right tool for the *unordered* categorical data introduced later (zone tags, call-in categories) — see Section 6.

### 1.2 `AShooterGameState`

**Confirmed gap:** `AShooterGameGameMode` currently extends `AGameModeBase` directly with no paired `AGameStateBase` subclass in `Source/ShooterGame/Framework/`. This must be created before anything else in this plan.

New files: `Source/ShooterGame/Framework/GameState/ShooterGameState.h/.cpp`

```cpp
UCLASS()
class SHOOTERGAME_API AShooterGameState : public AGameStateBase
{
    GENERATED_BODY()
public:
    UPROPERTY(ReplicatedUsing = OnRep_MissionPhase, BlueprintReadOnly, Category = "Mission")
    EMissionPhase CurrentMissionPhase = EMissionPhase::PreMission;

    UPROPERTY(BlueprintAssignable, Category = "Mission")
    FOnMissionPhaseChanged OnMissionPhaseChanged;

    // Server time (GetServerWorldTimeSeconds) the current timed window (resupply or
    // extraction vulnerability) ends. 0 = no active window. Consumed by File 04/File 05.
    UPROPERTY(ReplicatedUsing = OnRep_ActiveWindowEndTime, BlueprintReadOnly, Category = "Mission")
    double ActiveWindowEndServerTime = 0.0;

protected:
    UFUNCTION()
    void OnRep_MissionPhase();

    UFUNCTION()
    void OnRep_ActiveWindowEndTime();
};
```

Register on `AShooterGameGameMode` via `GameStateClass = AShooterGameState::StaticClass()` in the constructor. Must also be added to `DefaultEngine.ini` / the project's GameMode default if the GameMode is set there rather than via world settings.

### 1.3 `AMissionStateManager`

New files: `Source/ShooterGame/Mission/MissionStateManager.h/.cpp`

A server-only actor (spawned by `AShooterGameGameMode` in `InitGame` or placed once in the persistent level) that owns *transition logic* — it decides *when* to advance, then writes the new phase to `AShooterGameState::CurrentMissionPhase`. Per CLAUDE.md's existing architectural rule ("State machine never calls directly into other systems"), `AMissionStateManager` never calls into `AWaveSpawnManager`, `ACallInManager`, etc. directly — it only mutates `AShooterGameState` and broadcasts `FOnMissionPhaseChanged`. Every other system (File 02's wave manager, File 03's objective terminals, File 05's resupply window) binds to that delegate and reacts independently. This keeps phase logic decoupled and testable in isolation.

```cpp
UCLASS()
class SHOOTERGAME_API AMissionStateManager : public AActor
{
    GENERATED_BODY()
public:
    // Called by zone/objective/extraction systems when their exit condition fires.
    // Validates the requested transition is the legal next phase before applying it.
    UFUNCTION(BlueprintCallable, Category = "Mission")
    void Server_RequestPhaseAdvance(EMissionPhase RequestedPhase);

private:
    TObjectPtr<AShooterGameState> CachedGameState;

    // Ordered legal-transition table, built once in BeginPlay — rejects out-of-order
    // requests (defends against a race between, e.g., two zones both reporting clearance).
    static const TMap<EMissionPhase, EMissionPhase> NextPhaseTable;
};
```

`Server_RequestPhaseAdvance` is intentionally *not* a raw setter — every phase change is gated by "is this the legal next phase," so a duplicate or late-arriving trigger (e.g. two players' clients both reporting zone clearance) can't corrupt state. This is the same defensive pattern already used by `ULoadoutComponent::CanEquipInSlot()` validating before mutation.

### 1.4 Who calls `Server_RequestPhaseAdvance`

| Transition | Triggered by |
|---|---|
| `PreMission → Phase1Infiltration` | `AShooterGameGameMode::RestartPlayer` / mission start sequence, once all players have spawned |
| `Phase1Infiltration → Resupply1` | `AZoneDefinition` (Zone 1) reaching its clearance threshold — see Section 2.3 |
| `Resupply1 → Phase2Objective` | Resupply window timer expiry (File 05) |
| `Phase2Objective → Phase3Exfil` | `UObjectiveTaskComponent` task completion (File 03) |
| `Phase3Exfil → Resupply2` | Pursuit radius cleared (Section 5) |
| `Resupply2 → Phase4SecondObjective` | Resupply window timer expiry (File 05) |
| `Phase4SecondObjective → Phase4Exfil` | `AZoneDefinition` (Zone 2) clearance **and** its `UObjectiveTaskComponent` completion (mirrors Phase 1+2 combined per GameplayPlan.md) |
| `Phase4Exfil → PostMission` | `AExfilVehicle` departure (File 04) |

---

## 2. Zone-Based Spawn Management System

### 2.1 `AZoneDefinition`

CLAUDE.md already documents this actor's intended shape under "Zone Spawning" — this section is the concrete C++ design for it. **Confirmed not yet in `Source/`.**

New files: `Source/ShooterGame/Mission/Zone/ZoneDefinition.h/.cpp`

```cpp
UENUM(BlueprintType)
enum class EZoneState : uint8
{
    Neutral,
    Active,
    Doomed,
    Cleared
};

UCLASS()
class SHOOTERGAME_API AZoneDefinition : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Zone")
    FGameplayTag ZoneTag; // e.g. Zone.Urban, Zone.Industrial — see Section 6

    UPROPERTY(EditAnywhere, Category = "Zone")
    EMissionPhase OwningPhase; // which phase this zone is "live" during

    // Per-phase wave composition — keyed so the same zone actor can be reused for
    // Phase 1 clearance waves and, on second use as Zone 2, Phase 4 waves.
    UPROPERTY(EditAnywhere, Category = "Zone")
    TMap<EMissionPhase, TObjectPtr<UWaveSetDataAsset>> PhaseWaveSets;

    UPROPERTY(EditAnywhere, Category = "Zone")
    int32 ClearanceThreshold = 15; // enemies killed OR area-control ticks, see 2.3

    UPROPERTY(EditAnywhere, Category = "Zone")
    float PursuitRadius = 3000.f; // uu, doomed-zone exit distance — File 02 Section on doomed loop

    UPROPERTY(ReplicatedUsing = OnRep_ZoneState, BlueprintReadOnly, Category = "Zone")
    EZoneState CurrentZoneState = EZoneState::Neutral;

    UPROPERTY(BlueprintAssignable, Category = "Zone")
    FOnZoneStateChanged OnZoneStateChanged;

    UFUNCTION(BlueprintCallable, Category = "Zone")
    void Server_SetZoneState(EZoneState NewState);

protected:
    UFUNCTION()
    void OnRep_ZoneState();
};
```

`AZoneDefinition` does **not** spawn enemies itself — it is pure state + configuration data placed in the level. Spawning is delegated to `AWaveSpawnManager` (File 02), which reads a zone's `PhaseWaveSets` and `CurrentZoneState`. This split keeps zone *state* (replicated, lightweight) separate from spawn *execution* (server-only, heavier logic, no need to exist on clients at all beyond the state enum).

### 2.2 Volume-based zone boundary

Each `AZoneDefinition` owns (via a component or a paired actor reference) a `UBoxComponent` or `ANavModifierVolume`-adjacent trigger volume marking its physical boundary, used for:
- Enemy density anchoring (Section 3) — spawn points are pre-placed `ATargetPoint` actors tagged to a specific zone and only considered "in scope" for `AWaveSpawnManager` while that zone is `Active` or `Doomed`.
- Player-presence detection for the clearance threshold's optional "area control" mode.
- Pursuit-radius distance checks in Phase 3 (Section 5).

### 2.3 Clearance threshold modes

`GameplayPlan.md` allows clearance by "enemy count, area control, or triggered event." Implement all three as a single `EClearanceMode` enum on `AZoneDefinition` so a designer picks per-zone without new code:

```cpp
UENUM(BlueprintType)
enum class EClearanceMode : uint8
{
    EnemyCount,     // kill N enemies spawned in this zone's initial defended-position set
    AreaControl,    // squad presence inside zone volume for N continuous seconds, no living enemies
    TriggeredEvent  // external Blueprint/C++ event fires (e.g. a scripted moment) — calls Server_SetZoneState directly
};
```

For the demo's minimum viable map (Section 4), use `EnemyCount` for Zone 1 (simplest to tune and verify) and `AreaControl` for Zone 2, so both modes get exercised before Milestone 2 closes.

---

## 3. Enemy Density Anchoring

- Every enemy spawn point in the level is a `ATargetPoint` (or a lightweight custom `ASpawnPoint` actor if per-point metadata like "prefer this point for special infected" is needed) tagged with the owning `AZoneDefinition`'s `ZoneTag`.
- `AWaveSpawnManager` (File 02) only activates spawn points belonging to the zone currently in `Active` or `Doomed` state for the current `EMissionPhase`.
- **Transit corridor** points use a separate, always-on low-density spawn table (`UWaveSetDataAsset` with a long interval and small pack size) that runs continuously regardless of phase — this is what gives the corridor its "sparse, but not empty" feel per the design pillars.
- Concretely: transit points are tagged `Zone.Transit` (Section 6) rather than belonging to a specific `AZoneDefinition`, so they're handled by a separate, simpler always-on timer in `AWaveSpawnManager` rather than the phase-gated per-zone logic.

---

## 4. Map Layout Recommendation — Minimum Viable Zone Structure

`GameplayPlan.md` calls for 3–5 zones in the full design. For the demo, **2 objective zones is the minimum viable structure** — it exercises every phase transition (Phase 1↔2↔3 at Zone 1, Phase 4 mirrors 1+2+3 at Zone 2) without requiring 3–5x the level art and encounter authoring effort a solo dev can't fit into 8 weeks.

Recommended demo map composition:

```
[Zone 1: Urban — Phase 1/2/3]  ──[Transit Corridor A]──  [Zone 2: Industrial — Phase 4]
        │                                                          │
   Insert point (boat/heli LZ)                          Fixed exfil LZ (Phase 4 vehicle)
        │                                                          │
  Field extraction point A                              Field extraction point B (+ emergency)
```

- **Zone 1 (Urban):** dense, defended-position layout for Phase 1 infiltration; contains the Phase 2 objective terminal; contains Resupply Window 1 location just outside the hottest area (per GameplayPlan.md, resupply opens "before first wave fully arrives" — so it should be reachable without walking back through the whole zone).
- **Transit Corridor A:** the connective tissue between zones; sparse spawn density; contains a field extraction point roughly at its midpoint so players who want to bail after Phase 1/before committing to Zone 2 have a nearby option; contains the Resupply Window 2 location.
- **Zone 2 (Industrial):** structurally mirrors Zone 1 but reuses `AreaControl` clearance (Section 2.3) for variety; contains the Phase 4 comms-tower upload terminal; contains the fixed exfil LZ and a secondary emergency extraction point some distance away (per GameplayPlan.md's "stranded players" fallback).

This satisfies "3–5 distinct objective zones" as a documented *stretch* target (see File 10) — Zones 3–5 (forest, radioactive blast zone) are explicitly deferred past the demo; the map should be blocked out so additional zones can be appended to the transit corridor later without restructuring the phase system, but building them is not part of this 2-month plan.

---

## 5. Hot Zone Escalation and Pursuit Radius (Phase 3 Doomed Zone)

- **Hot zone escalation:** as an `Active` zone's waves progress, `AWaveSpawnManager` reads an escalation curve off the current `UWaveSetDataAsset` (a `UCurveFloat` mapping "waves survived" → "spawn rate multiplier"). This is data, not code, so Risk #1 in the Master Brief (doomed-zone timing) can be retuned without a rebuild.
- **Doomed transition:** when `Phase2Objective → Phase3Exfil` fires, `AZoneDefinition::Server_SetZoneState(EZoneState::Doomed)` is called on Zone 1. Per CLAUDE.md's existing Zone Spawning note, doomed state means "no spawn ceiling; rate multiplier escalates every 15s after Phase 3 trigger" — implemented as a repeating `FTimerHandle` on `AWaveSpawnManager` that increments the multiplier every 15 seconds with no upper clamp, only stopped when the zone transitions to `Cleared`.
- **Pursuit radius:** `AZoneDefinition::PursuitRadius` defines a distance from the zone's center (or a designated "pursuit anchor" point near the zone exit) that all squad members must clear. `AZoneDefinition` (server-only tick, or a cheaper periodic timer rather than per-frame Tick) checks each living player pawn's distance; once every player is outside `PursuitRadius` simultaneously, it calls `Server_SetZoneState(EZoneState::Cleared)`, which despawns remaining Zone 1 enemies (handled by `AWaveSpawnManager` binding to `OnZoneStateChanged`) and triggers `Phase3Exfil → Resupply2`.
- Use a periodic timer (e.g. every 0.5s) rather than `Tick` for the distance check — this runs for the lifetime of the doomed zone and there's no reason to pay per-frame cost for a check that only needs to react within half a second.

---

## 6. GameplayTags for Zone State

Per CLAUDE.md's existing tag-management rule, every new tag category requires updating `ShooterGameplayTags.h`, the matching `.cpp`, and `Config/DefaultGameplayTags.ini` — all three, every time. This plan introduces one new tag family:

```cpp
// Zone Tags — identify a zone for spawn-point tagging and UI display
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Zone_Urban);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Zone_Industrial);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Zone_Transit);
```

Follow the existing `Slot.*` naming convention (`"Zone.Urban"`, `"Zone.Industrial"`, `"Zone.Transit"`). Do not reuse `Slot.*` tags for zones even though both live in the same header file — keep the root namespace distinct so tag queries can't accidentally cross-match.

---

## 7. UE5 References

- **GameplayTags** — used for zone identity and spawn-point tagging: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Gameplay Framework → Gameplay Tags)
- **Actor Replication / `AGameStateBase`** — `AShooterGameState` and `AZoneDefinition`'s `ReplicatedUsing` pattern: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Networking and Multiplayer → Actor Replication)
- **AI Perception System** — not used directly in this file, but zone spawn points feed the perception stimuli consumed in File 02; see NavMesh/AI docs: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation
- **NavMesh Configuration (`RecastNavMesh`)** — required for both zones and the transit corridor to support enemy pathing at the density levels described here; nav mesh bounds volumes must cover the full playable map before Week 2 wave testing: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see AI → Navigation Mesh)
- **`FTimerManager`** — zone escalation timers, pursuit-radius polling: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Programming → Gameplay Timers)

---

## 8. Week-by-Week Tasks

**Week 1 (Jul 13 – Jul 19)**
- Create `MissionTypes.h` with `EMissionPhase`, `EZoneState`, `EClearanceMode`.
- Create `AShooterGameState`, wire into `AShooterGameGameMode`.
- Create `AMissionStateManager` with the legal-transition table and `Server_RequestPhaseAdvance` stub (no real triggers wired yet).
- Create `AZoneDefinition` class (no spawning logic yet — state + config only).
- Add `Zone.*` GameplayTags (all three files per CLAUDE.md rule).
- Greybox the minimum viable map: Zone 1 volume, Zone 2 volume, transit corridor, insert point, placeholder exfil LZ.

**Week 2 (Jul 20 – Jul 26)**
- Place and tag spawn points per zone (`ATargetPoint` + zone tag).
- Implement `EClearanceMode::EnemyCount` on Zone 1 and wire `AZoneDefinition` → `AMissionStateManager::Server_RequestPhaseAdvance(Resupply1)` on threshold met.
- Implement periodic pursuit-radius check (used later in Week 4, build now while zone code is fresh).
- **Checkpoint A dependency:** this file's output (zone clearance firing a real phase transition) is required for File 02's Week 2 checkpoint.

**Week 3 (Jul 27 – Aug 2)**
- Implement `EClearanceMode::AreaControl` on Zone 2.
- Implement hot-zone escalation curve consumption (`UCurveFloat` read, multiplier applied) — coordinate with File 02, which owns the actual spawn timer.
- Implement doomed-state 15-second escalation timer on `AZoneDefinition`/`AWaveSpawnManager` boundary (this file owns the state transition, File 02 owns spawn execution reading that state).

**Week 4 (Aug 3 – Aug 9)**
- Wire pursuit-radius clearance to `Phase3Exfil → Resupply2` transition.
- Wire Zone 2 clearance + task completion (File 03 dependency) to `Phase4SecondObjective → Phase4Exfil`.
- Playtest full phase-transition chain end-to-end for the first time.

Weeks 5–8: no new tasks owned by this file — this system moves into **maintenance mode**, consumed by every other file's week-by-week plan. Revisit only if Week 8 acceptance testing (File 10) surfaces a phase-transition bug.

---

## 9. Acceptance Criteria

- [ ] `AShooterGameState::CurrentMissionPhase` is visible and identical on host and client at all times (verified via two-client PIE session).
- [ ] All 8 phase transitions in Section 1.4 fire in the correct order in one continuous session with zero manual console commands.
- [ ] `AMissionStateManager::Server_RequestPhaseAdvance` rejects an out-of-order phase request (test by manually calling it with a skip-ahead phase from a debug console command — the phase must not change).
- [ ] Zone 1 correctly cycles Neutral → Active → Doomed → Cleared exactly once per session.
- [ ] Zone 2 correctly cycles Neutral → Active → Cleared (no Doomed state required for Zone 2 in the demo — GameplayPlan.md's doomed-zone requirement is specific to the Phase 3 first-zone exfil).
- [ ] Transit corridor spawn density is visibly and measurably lower than either zone's `Active` density (verify via spawn-count logging over a fixed time window).
- [ ] Pursuit radius correctly requires **all** living squad members to clear the distance, not just one (test with 2 clients, one lagging behind).
- [ ] Escalation curve is data-driven and can be edited in the `UCurveFloat` asset without a C++ rebuild.

---

## 10. Estimated Effort

| Task Cluster | Estimate |
|---|---|
| `EMissionPhase` / `AShooterGameState` / `AMissionStateManager` | 3 dev-days |
| `AZoneDefinition` + clearance modes | 4 dev-days |
| Spawn-point tagging + density anchoring plumbing (execution lives in File 02, but the tagging/query contract is defined and built here) | 2 dev-days |
| Hot-zone escalation + doomed-state timer | 2 dev-days |
| Pursuit radius system | 1.5 dev-days |
| Minimum viable map greybox (art/level-design time, not engineering, but scheduled here since it blocks all testing) | 4 dev-days |
| GameplayTags plumbing | 0.5 dev-days |
| **Total** | **17 dev-days** (fits within Weeks 1–4 alongside File 02/03/04 work, which run partially in parallel per File 09's schedule) |
