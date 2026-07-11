# 02 — Enemy AI and Waves

## Dependencies

- [01_Phase_Triggers_and_Map_Structure.md](01_Phase_Triggers_and_Map_Structure.md) — this file's `AWaveSpawnManager` reads `AZoneDefinition::CurrentZoneState`, `PhaseWaveSets`, and zone-tagged spawn points defined there. Do not start this file's spawn-manager work until File 01's `AZoneDefinition` class exists (Week 1).
- [08_Multiplayer_and_Networking.md](08_Multiplayer_and_Networking.md) — spawning is server-only; read the authority model before writing `AWaveSpawnManager`.

---

## 1. Wave Spawn Manager

### 1.1 `AWaveSpawnManager`

New files: `Source/ShooterGame/Mission/Zone/WaveSpawnManager.h/.cpp`

A single server-only actor (world-placed once, or spawned by `AShooterGameGameMode`) that owns all enemy-spawn timers for the whole map. It does **not** subclass per zone — instead it holds a runtime map of active per-zone spawn state, keyed by the `AZoneDefinition` pointer, and a separate always-on transit-corridor timer (per File 01 Section 3).

```cpp
UCLASS()
class SHOOTERGAME_API AWaveSpawnManager : public AActor
{
    GENERATED_BODY()
public:
    // Bound to every AZoneDefinition::OnZoneStateChanged in the level at BeginPlay.
    UFUNCTION()
    void HandleZoneStateChanged(AZoneDefinition* Zone, EZoneState NewState);

protected:
    void BeginZoneWaves(AZoneDefinition* Zone);
    void StopZoneWaves(AZoneDefinition* Zone);
    void TickZoneSpawn(AZoneDefinition* Zone); // called by per-zone FTimerHandle
    void SpawnEnemyAtPoint(TSubclassOf<ACharacter> EnemyClass, ASpawnPoint* Point);

private:
    struct FActiveZoneSpawnState
    {
        FTimerHandle SpawnTimerHandle;
        int32 CurrentWaveIndex = 0;
        int32 WavesSurvivedSinceDoomed = 0; // drives the 15s doomed multiplier, see File 01 §5
        float CurrentRateMultiplier = 1.f;
    };
    TMap<TObjectPtr<AZoneDefinition>, FActiveZoneSpawnState> ActiveZoneStates;

    FTimerHandle TransitCorridorTimerHandle;
};
```

### 1.2 `UWaveSetDataAsset`

New files: `Source/ShooterGame/Enemy/Waves/WaveSetDataAsset.h/.cpp`

`UPrimaryDataAsset`, one per zone-per-phase combination (e.g. `DA_Wave_Zone1_Phase1`, `DA_Wave_Zone1_Phase2`). Matches the existing project convention of authoring gameplay content as Data Assets rather than hardcoded tables (see `UItemDefinition`, `UWeaponConfig`).

```cpp
USTRUCT(BlueprintType)
struct FWaveEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) TSubclassOf<ACharacter> EnemyClass;
    UPROPERTY(EditAnywhere) int32 Count = 5;
    UPROPERTY(EditAnywhere) float SpawnInterval = 2.f; // seconds between individual spawns within this wave
};

UCLASS()
class SHOOTERGAME_API UWaveSetDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere) TArray<FWaveEntry> Waves; // sequential — wave N+1 begins when wave N's spawn budget is exhausted
    UPROPERTY(EditAnywhere) TObjectPtr<UCurveFloat> EscalationCurve; // X = waves survived, Y = spawn-rate multiplier
    UPROPERTY(EditAnywhere) float BaseSpawnRadius = 1500.f; // how far from zone center spawn points are considered valid
};
```

New weapon/enemy content requiring no C++ changes is already a stated project value (see `UItemDefinition` doc comment in CLAUDE.md) — this design extends that same philosophy to wave content: a designer authors a new `UWaveSetDataAsset` instance and assigns it to `AZoneDefinition::PhaseWaveSets`, no code touched.

### 1.3 Escalation and doomed-state interaction

- While a zone is `Active`, `CurrentRateMultiplier` is read from `EscalationCurve` keyed by `CurrentWaveIndex`.
- On transition to `Doomed` (File 01 Section 5), `AWaveSpawnManager` switches that zone's spawn loop from "sequential authored waves" to "continuous spawn at `CurrentRateMultiplier`, no wave boundaries, no completion condition" — this is what "no ceiling" means concretely: the `FWaveEntry` list is no longer consulted, only `EnemyClass` pools are cycled continuously.
- The 15-second no-cap escalation timer (owned by this manager, state read from `AZoneDefinition`) increments `CurrentRateMultiplier` by a configurable step (`EditAnywhere` on `AZoneDefinition`, default +0.15/tick) indefinitely — there is intentionally no clamp, since doomed zones are meant to be un-winnable, only escapable.

---

## 2. Zombie AI — Existing Behavior and Required Extensions

Current state (confirmed in `Source/ShooterGame/Enemy/`): `AZombieCharacter` (health, hit-zone routing, melee, no decision logic per CLAUDE.md), `AZombieAIController` (owns Blackboard/BT decisions), `ZombieAnimInstance`, and two custom BT tasks (`BTTask_InvestigateWander`, `BTTask_WanderToPoint`). This is a working foundation — the wave system spawns instances of `AZombieCharacter` directly; no changes to its health/melee logic are required for the demo.

### 2.1 Night variant / special infected — data-driven, not subclassed

Rather than creating parallel `ANightZombieCharacter` subclasses (which would fragment hit-zone and damage logic already centralized in `UHitZoneComponent`), implement night variants as a `UZombieVariantDataAsset` applied at spawn time:

New files: `Source/ShooterGame/Enemy/ZombieVariantDataAsset.h/.cpp`

```cpp
UCLASS()
class SHOOTERGAME_API UZombieVariantDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere) float HealthMultiplier = 1.f;
    UPROPERTY(EditAnywhere) float MoveSpeedMultiplier = 1.f;
    UPROPERTY(EditAnywhere) FGameplayTag ThreatTag; // Threat.Elevated — read by File 05's scoring for "special enemy kills"
    UPROPERTY(EditAnywhere) TSoftObjectPtr<USkeletalMesh> VariantMesh; // cosmetic swap, e.g. darker/damaged skin
    UPROPERTY(EditAnywhere) bool bOnlySpawnsAtNight = true;
};
```

`AZombieCharacter` gains a single `UFUNCTION(BlueprintCallable) void ApplyVariant(UZombieVariantDataAsset* Variant)` called immediately post-spawn by `AWaveSpawnManager::SpawnEnemyAtPoint`, which scales `UHitZoneComponent`-driven max health, move speed, and swaps the mesh. `FWaveEntry` (Section 1.2) gains an optional `TObjectPtr<UZombieVariantDataAsset> Variant` field so a wave set can specify "this wave's zombies are night variants" without new spawn code.

**Time-of-day state:** the demo does not require a full day/night cycle system. A simple `bool bIsNightPhase` on `AShooterGameState`, flipped by a designer-placed trigger or tied to `Phase2Objective`/`Phase4SecondObjective` (per GameplayPlan.md: "Night variants... may arrive based on time-of-day state"), is sufficient — `AWaveSpawnManager` checks it before allowing a `bOnlySpawnsAtNight` variant wave to roll.

### 2.2 Aggression escalation

`AZombieAIController`'s existing Blackboard should gain one new key: `AggressionMultiplier` (float), pushed by `AWaveSpawnManager` whenever a zone's `CurrentRateMultiplier` changes (via a lightweight `Server_NotifyZoneAggressionChanged` broadcast, not a per-enemy poll). A BT Service (`BTService_ApplyAggressionMultiplier`, new) reads this key and scales acceptance radius / sprint-chance decorators already in the BT — this makes doomed-zone zombies feel more relentless without new pathing logic, just tighter decision thresholds.

---

## 3. Human AI Reinforcement

**[CREATE]** — no human enemy AI currently exists in `Source/ShooterGame/Enemy/`.

### 3.1 `AHumanEnemyCharacter` / `AHumanEnemyAIController`

New files: `Source/ShooterGame/Enemy/Character/HumanEnemyCharacter.h/.cpp`, `Source/ShooterGame/Enemy/Character/HumanEnemyAIController.h/.cpp`

`AHumanEnemyCharacter` reuses `UHitZoneComponent` (already explicitly shared between player and zombie per CLAUDE.md — "shared with AZombieCharacter") for consistent damage-multiplier behavior, but is armed (fires projectiles/hitscan via a simplified version of the player's weapon-fire path — does not need the full `UCombatComponent` state machine, just enough to fire on a timer at a target).

`AHumanEnemyAIController` decision differences from `AZombieAIController`:

| Behavior | Zombie | Human AI |
|---|---|---|
| Movement to target | Direct pathing (`BTTask_WanderToPoint`/investigate) | Cover-to-cover movement via EQS cover selection |
| Attack | Melee only | Ranged fire, uses `BTTask_SuppressTarget` |
| Positioning | None (approaches directly) | Seeks EQS-scored cover points before engaging |
| Squad behavior | None (independent) | Basic flanking: a `BTService` tags one human AI per encounter as "flanker," which biases its EQS cover query toward points lateral to the player's facing rather than the nearest cover |

### 3.2 New BT Tasks / Services / EQS

- `BTTask_TakeCover` — runs an `EQS_FindCoverNearTarget` query (scores points by: distance to target-visibility-blocking geometry via line trace, distance to self, and — for the tagged flanker — lateral offset from target facing), moves to the winning point.
- `BTTask_SuppressTarget` — fires at the target's last-known/perceived location at a configured rate; does not require guaranteed hits, this is suppression not precision.
- `BTService_TagFlanker` — runs once per encounter start; picks one human AI from the currently-perceived group (via `UAIPerceptionComponent`, see Section 6) to bias toward flank positioning.
- `EQS_FindCoverNearTarget` — new EQS query asset, generator: points around self within a radius; tests: trace-to-target visibility (prefer blocked = actual cover), distance to self (closer preferred), distance to target (mid-range preferred over point-blank or extreme range).

### 3.3 Scope guardrail (Risk #8 from Master Brief)

The demo's human-AI first pass explicitly does **not** include: multi-unit coordinated flanking (only a single tagged flanker per encounter, no team-level tactical coordinator), grenade use, or vehicle-mounted human AI. If Week 3 runs long, cut `BTService_TagFlanker` first — human AI without a flanker still reads as "using cover and suppressing," which meets the GameplayPlan.md bar ("cover usage, suppression, flanking" — flanking is the correct first cut given it's the most complex of the three).

---

## 4. Overrun Threshold and Doomed-Zone Spawn Loop

- `AZoneDefinition` (File 01) gains `float OverrunTimeLimit` (seconds, `EditAnywhere`) — the time budget for `Phase2Objective`'s task to complete once the zone enters `Active` state during Phase 2.
- A `BTService`-free, simpler mechanism is correct here since this is mission-state logic, not per-enemy AI: `UObjectiveTaskComponent` (File 03) starts a server timer when the task begins; if `OverrunTimeLimit` elapses before task completion, it calls `AZoneDefinition::Server_SetZoneState(EZoneState::Doomed)` directly — the zone becomes unwinnable (per GameplayPlan.md, "if task not completed in time, zone becomes unwinnable") rather than the mission hard-failing. This gives players a graceful failure path: they can still use the early-extraction system (File 04) rather than being killed outright.
- This is a **distinct trigger** for entering `Doomed` state from the "normal" trigger (task completes successfully → `Phase3Exfil` begins → zone goes doomed as part of the *intended* flow). Both paths converge on the same `EZoneState::Doomed`, and `AWaveSpawnManager` doesn't need to know which path caused it — this is exactly why decoupling zone state from phase-transition logic (File 01 Section 1.3) pays off.

---

## 5. Call-In Enemy Attraction and Distraction Flare

### 5.1 Enemy attraction (ammo/medical crates)

Both `ASupplyCrate_Ammo` and fire-support impact points (File 05) register an `AI Perception` **hearing stimulus** at their world location on spawn/impact, using `UAISense_Hearing::ReportNoiseEvent`. `AZombieAIController` and `AHumanEnemyAIController` already need a `UAIPerceptionComponent` configured with a hearing sense for this to work — if not already present on `AZombieAIController` (verify in Week 2), add it as part of this file's tasks rather than assuming it exists.

This is the correct, idiomatic UE5 mechanism for "crate lands, draws nearby enemies" — it reuses the engine's built-in stimulus/perception pipeline instead of a custom radius-broadcast system, and composes naturally with existing perception-driven zombie aggro.

### 5.2 Distraction flare pathing diversion

`ADistractionFlare` (File 05, spawned via call-in) reports a *louder, longer-duration* hearing stimulus at its landing point on impact, then again periodically (not continuously — every ~10s) for its 90-second lifetime, competing with the players' own noise stimuli (footsteps, gunfire) for AI attention priority. `UAIPerceptionComponent`'s dominant-sense scoring naturally handles "an enemy near the flare prefers it over a farther/quieter player" without bespoke code — this is why the perception system, not a custom aggro-radius solve, is the correct approach for both 5.1 and 5.2.

- A currently-fighting zombie should **not** abandon an adjacent player for a flare on the other side of the map — bound the flare's stimulus radius (`EditAnywhere` on `ADistractionFlare`) to a tuned value (~2000uu starting point) so this only diverts enemies that were already loosely wandering/investigating, not ones actively engaged. Confirm this behavior explicitly in acceptance testing (Section 9) since it's the one place this system could visibly break believability if mistuned.

---

## 6. UE5 References

- **Behavior Trees** — all new BT tasks/services (`BTTask_TakeCover`, `BTTask_SuppressTarget`, `BTService_TagFlanker`, `BTService_ApplyAggressionMultiplier`): https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see AI → Behavior Trees)
- **Environment Query System (EQS)** — `EQS_FindCoverNearTarget`: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see AI → Environment Query System)
- **AIController / Blackboard** — `AHumanEnemyAIController`, new Blackboard keys: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see AI → AI Controllers, Behavior Trees → Blackboard)
- **NavMesh (`RecastNavMesh`)** — cover points and flank points must resolve to navigable locations; confirm nav mesh coverage extends to all authored cover geometry before EQS testing: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see AI → Navigation Mesh)
- **AI Perception System (`UAIPerceptionComponent`, `UAISense_Hearing`)** — enemy attraction, distraction flare: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see AI → AI Perception)
- **Data Assets (`UPrimaryDataAsset`)** — `UWaveSetDataAsset`, `UZombieVariantDataAsset`: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Content → Data Assets)

---

## 7. Week-by-Week Tasks

**Week 1 (Jul 13 – Jul 19)** — parallel with File 01's foundation work
- Design `UWaveSetDataAsset` and `FWaveEntry` structs.
- Confirm/add `UAIPerceptionComponent` (hearing sense) to `AZombieAIController` if not already present.

**Week 2 (Jul 20 – Jul 26)**
- Build `AWaveSpawnManager`, bind to `AZoneDefinition::OnZoneStateChanged`.
- Author first `UWaveSetDataAsset` instances for Zone 1 Phase 1 (defended-position initial set + clearance waves).
- **Checkpoint A: Phase 1 loop playable solo** — spawn manager + Zone 1 clearance threshold (File 01) must both be functional by end of this week.

**Week 3 (Jul 27 – Aug 2)**
- Author Zone 1 Phase 2 wave set with escalation curve.
- Implement overrun threshold trigger path (Section 4) — coordinate with File 03, whose task component owns the timer start.
- Implement doomed-state continuous spawn loop switch.
- Build human AI first pass: `AHumanEnemyCharacter`, `AHumanEnemyAIController`, `BTTask_TakeCover`, `EQS_FindCoverNearTarget`. If time-constrained, land cover+suppress and defer `BTService_TagFlanker` (Section 3.3 guardrail).

**Week 4 (Aug 3 – Aug 9)**
- Implement `UZombieVariantDataAsset` + night-variant spawn path + `bIsNightPhase` flag.
- Implement enemy attraction stimulus reporting (ties into File 05's crate actors landing).
- Author Zone 2 wave sets (Phase 4).

**Week 7 (Aug 24 – Aug 30)**
- Implement distraction flare stimulus behavior (depends on `ADistractionFlare` existing from File 05 Week 5–6 work).
- Tuning pass on escalation curves and doomed-state multiplier step, informed by Weeks 2–6 playtesting.

---

## 8. Acceptance Criteria

- [ ] `AWaveSpawnManager` only spawns from a zone's tagged points while that zone is `Active` or `Doomed`.
- [ ] Transit corridor spawn rate is a separate, always-on, low-density timer independent of any zone state.
- [ ] A zone's spawn behavior correctly switches from authored sequential waves to uncapped continuous spawning the instant it enters `Doomed`.
- [ ] Doomed-zone spawn multiplier increases every 15 seconds indefinitely (verify by leaving a doomed zone active for 5+ minutes in a debug session and confirming spawn rate keeps climbing, not plateauing).
- [ ] Night-variant zombies only spawn when `bIsNightPhase` is true, and correctly apply health/speed/mesh overrides from `UZombieVariantDataAsset`.
- [ ] Human AI enemy visibly takes cover before engaging (not standing in the open) in at least 80% of encounters during playtesting.
- [ ] Human AI enemy fires at the player from cover (suppression) rather than only melee.
- [ ] Enemy attraction: spawning an ammo crate or fire-support impact within perception range of a wandering/investigating zombie reliably (observed across 5+ manual tests) redirects at least one nearby zombie toward the noise.
- [ ] A distraction flare diverts a wandering zombie's path but does **not** pull a zombie actively engaged with a player within melee/combat range.
- [ ] Overrun threshold correctly forces a zone into `Doomed` state if the Phase 2 task isn't completed in time, and this does not crash or softlock the mission state machine.

---

## 9. Estimated Effort

| Task Cluster | Estimate |
|---|---|
| `AWaveSpawnManager` + `UWaveSetDataAsset` | 4 dev-days |
| Escalation curve + doomed-state continuous spawn loop | 2 dev-days |
| Night variant system (`UZombieVariantDataAsset` + spawn integration) | 2 dev-days |
| Human AI character + controller + BT/EQS first pass (cover + suppress) | 6 dev-days |
| Human AI flanker tagging (stretch, cut first if needed) | 1.5 dev-days |
| Enemy attraction perception wiring | 1.5 dev-days |
| Distraction flare pathing diversion behavior | 1 dev-day |
| Wave content authoring (Zone 1 + Zone 2 wave sets, all phases) | 3 dev-days |
| **Total** | **21 dev-days** (spans Weeks 1–4 and Week 7 per schedule) |
