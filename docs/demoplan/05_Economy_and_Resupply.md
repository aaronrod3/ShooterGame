# 05 — Economy and Resupply

## Dependencies

- [01_Phase_Triggers_and_Map_Structure.md](01_Phase_Triggers_and_Map_Structure.md) — resupply windows open/close on phase transitions (`Resupply1`, `Resupply2`) owned by `AMissionStateManager`.
- [02_Enemy_AI_and_Waves.md](02_Enemy_AI_and_Waves.md) — ammo crates and fire-support strikes register AI perception stimuli defined in that file's Section 5.
- [04_Extraction_and_Exfil.md](04_Extraction_and_Exfil.md) — `UScoringSubsystem` (defined here) is the handoff target for extraction reward application.
- [06_Kit_Presets_and_Loadout.md](06_Kit_Presets_and_Loadout.md) — kit swap is a call-in-adjacent purchase gated by the same resupply-window system built here.
- [08_Multiplayer_and_Networking.md](08_Multiplayer_and_Networking.md) — every purchase and point award must be server-validated; read the authority model first.

---

## 1. Points Tracking System

### 1.1 Per-player points

`AShooterPlayerState` (existing class) gains:

```cpp
UPROPERTY(ReplicatedUsing = OnRep_PersonalPoints, BlueprintReadOnly, Category = "Economy")
int32 PersonalPoints = 0;

UFUNCTION() void OnRep_PersonalPoints();

UPROPERTY(BlueprintAssignable, Category = "Economy")
FOnPersonalPointsChanged OnPersonalPointsChanged;
```

This matches the naming CLAUDE.md's Economy section already declares ("PersonalPoints (per player)") — no renaming needed, this plan is filling in a field CLAUDE.md anticipated but that doesn't exist in `ShooterPlayerState.h` yet.

### 1.2 Shared squad fund

`AShooterGameState` (File 01) gains:

```cpp
UPROPERTY(ReplicatedUsing = OnRep_SquadSharedFund, BlueprintReadOnly, Category = "Economy")
int32 SquadSharedFund = 0;

UFUNCTION() void OnRep_SquadSharedFund();

UPROPERTY(BlueprintAssignable, Category = "Economy")
FOnSquadSharedFundChanged OnSquadSharedFundChanged;
```

Lives on `AShooterGameState`, not any one player's state, since it's squad-wide and must be visible/consistent regardless of which player is asking — matches CLAUDE.md's "SquadSharedFund" pooled-currency naming.

### 1.3 `UScoringSubsystem`

New files: `Source/ShooterGame/Framework/Subsystems/ScoringSubsystem.h/.cpp`

CLAUDE.md already states "All scoring through ScoringSubsystem (server-authoritative)" — this is the concrete implementation of that named-but-missing class. A `UWorldSubsystem` (not `UGameInstanceSubsystem` — scoring is per-session/per-mission, should not persist across a session boundary given "no save-and-return" per GameplayPlan.md).

```cpp
UCLASS()
class SHOOTERGAME_API UScoringSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    // Single entry point for all point awards — never mutate PersonalPoints/SquadSharedFund directly elsewhere.
    UFUNCTION(BlueprintCallable, Category = "Scoring")
    void AwardPoints(APlayerState* Player, EScoreEventType EventType, int32 OverrideAmount = -1);

    UFUNCTION(BlueprintCallable, Category = "Scoring")
    void ApplyExtractionReward(APlayerState* Player, const FExtractionReward& Reward); // File 04 handoff

private:
    int32 GetPointValueForEvent(EScoreEventType EventType) const; // reads UScoreValuesDataAsset
    void ContributeToSharedFund(int32 Amount); // splits award between personal + shared per event type
};

UENUM(BlueprintType)
enum class EScoreEventType : uint8
{
    KillStandard,
    Assist,
    KillSpecial,          // night variants, human AI officers — File 02's ThreatTag-elevated enemies
    TaskMilestone,
    WaveClearedSurvival,
    PhaseCompletion
};
```

### 1.4 `UScoreValuesDataAsset`

New file: `Source/ShooterGame/Framework/Subsystems/ScoreValuesDataAsset.h` — a single `UPrimaryDataAsset` holding a `TMap<EScoreEventType, int32>` plus a `SharedFundContributionRatio` (0..1, what fraction of each award also lands in `SquadSharedFund`). This keeps every point value tunable data, not code — directly serving the "planning pays off" pillar's need for frequent balance iteration without rebuilds, and mirrors File 02's `EscalationCurve` approach to keeping tuning knobs out of C++.

### 1.5 Award call sites

| Event | Called from |
|---|---|
| `KillStandard` / `KillSpecial` | `UHitZoneComponent` or the character death path already handling `TakeDamage()` — on enemy death, credit the last-damaging player (and, for `Assist`, any other player who damaged that enemy within a short window, tracked via a lightweight `TArray<TWeakObjectPtr, float>` "recent damagers" list on the enemy character) |
| `TaskMilestone` | `UObjectiveTaskComponent::Server_BeginTask`/completion (File 03) — split into a smaller milestone award at task start and the significant award at completion |
| `WaveClearedSurvival` | `AWaveSpawnManager` (File 02) when a zone's authored wave list is exhausted with the squad still alive |
| `PhaseCompletion` | `AMissionStateManager::Server_RequestPhaseAdvance` success path — award fires for `Phase1Infiltration`, `Phase2Objective`, `Phase4SecondObjective` completions per GameplayPlan.md ("Phase completion (1, 2, 4)") |

---

## 2. Resupply Window System

### 2.1 Ownership

Resupply windows are **phases**, not a bolt-on timer — `EMissionPhase::Resupply1` and `EMissionPhase::Resupply2` (File 01) already represent "a resupply window is open." This is a deliberate design choice: instead of a separate `bResupplyWindowOpen` bool that could desync from the phase state machine, the phase itself *is* the window, which eliminates an entire class of "window says open but phase says otherwise" bugs.

`AShooterGameState::ActiveWindowEndServerTime` (File 01 Section 1.2) is set by `AMissionStateManager` at the moment it transitions into `Resupply1`/`Resupply2`: `ActiveWindowEndServerTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds() + WindowDuration`, where `WindowDuration` is `EditAnywhere` on `AMissionStateManager` (two separate fields, `ResupplyWindow1Duration` default 75s and `ResupplyWindow2Duration` default 105s, splitting the difference within each of GameplayPlan.md's 60–90s / 90–120s ranges as a tunable starting point per Risk #2 in the Master Brief).

A repeating server timer on `AMissionStateManager` checks `GetServerWorldTimeSeconds() >= ActiveWindowEndServerTime` at ~1Hz while in a `Resupply*` phase and calls `Server_RequestPhaseAdvance` to the next phase on expiry — "Timer expires → window closes → unspent points are lost" is satisfied automatically, since `PersonalPoints`/`SquadSharedFund` simply are not reset or refunded, they just become unspendable once the phase changes (Section 2.2).

### 2.2 Purchase locking

Every purchase entry point (call-in requests, kit swap requests) performs the same server-side guard as its first check: `AShooterGameState::CurrentMissionPhase == EMissionPhase::Resupply1 || == EMissionPhase::Resupply2`. This check lives in one place — `ACallInManager::Server_RequestCallIn` (Section 3.3) and `AShooterPlayerState::Server_SwapToPreset` (File 06) — rather than being re-derived per call-in type, so there's exactly one place to get the gating condition right.

### 2.3 Resupply UI overlay

New widget: `UW_ResupplyOverlay` (HUD/Widgets/) — appears automatically on `AShooterGameState::OnMissionPhaseChanged` firing with a `Resupply*` phase, shows: countdown (derived from `ActiveWindowEndServerTime`, same server-time-based pattern as File 04's exfil countdown — no naive local countdown timers anywhere in this plan, for consistency), `SquadSharedFund` balance, `PersonalPoints` balance, call-in purchase buttons (Section 3), and hands off to `UW_ResupplyKitSwap` (File 06) for the preset-swap sub-panel. Auto-closes (or the buttons simply become non-functional and a "window closed" state replaces them) on phase transition away from `Resupply*`.

---

## 3. Call-In Purchase and Drop Scheduling System

### 3.1 `UCallInDefinition`

New files: `Source/ShooterGame/Mission/Economy/CallInDefinition.h/.cpp`

`UPrimaryDataAsset`, one instance per call-in type — mirrors the project's established "new content = new Data Asset, no C++ changes" pattern (`UItemDefinition`, `UWaveSetDataAsset`).

```cpp
UCLASS()
class SHOOTERGAME_API UCallInDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere) FText DisplayName;
    UPROPERTY(EditAnywhere) int32 CostFromSharedFund = 0;
    UPROPERTY(EditAnywhere) ECallInCostTier CostTier = ECallInCostTier::Low; // UI grouping only, cost itself is CostFromSharedFund
    UPROPERTY(EditAnywhere) float DropDelaySeconds = 30.f; // "arrive at the next drop window", see 3.4
    UPROPERTY(EditAnywhere) bool bRequiresSquadVote = false; // Section 3.5 — true for High-tier by default
    UPROPERTY(EditAnywhere) bool bAttractsEnemies = false;
    UPROPERTY(EditAnywhere) TSoftClassPtr<AActor> SpawnClass; // crate / strike / squad-member actor to spawn on delivery
    UPROPERTY(EditAnywhere) bool bRequiresPlayerProximitySafetyCheck = false; // artillery only, see Section 3.7
};
```

### 3.2 `ACallInManager`

New files: `Source/ShooterGame/Mission/Economy/CallInManager.h/.cpp`

Single server-only actor, analogous in role to `AWaveSpawnManager` — one place owning all in-flight call-in timers.

```cpp
UCLASS()
class SHOOTERGAME_API ACallInManager : public AActor
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Economy")
    void Server_RequestCallIn(UCallInDefinition* Definition, FVector TargetLocation, APlayerState* Requester);

protected:
    bool ValidateRequest(UCallInDefinition* Definition, APlayerState* Requester, FText& OutFailReason) const;
    void ScheduleDelivery(UCallInDefinition* Definition, FVector TargetLocation);
    void ExecuteDelivery(UCallInDefinition* Definition, FVector TargetLocation);

private:
    UPROPERTY() TArray<FPendingCallIn> PendingDeliveries; // for UI: "incoming in Xs" indicator
};
```

### 3.3 Validation (`ValidateRequest`)

In order: (1) requester is a valid player in the session, (2) mission phase is `Resupply1`/`Resupply2` (Section 2.2), (3) `AShooterGameState::SquadSharedFund >= Definition->CostFromSharedFund`, (4) if `bRequiresSquadVote`, a vote has passed (Section 3.5), (5) `TargetLocation` is within a designer-marked valid drop zone (reuses File 01's zone-tagged volumes, or a dedicated "drop zone" marker — for the demo, valid drop zones are the same transit corridor / resupply-adjacent markers already placed for File 01's map). On success, deduct cost immediately (`SquadSharedFund -= CostFromSharedFund`) — deducting on *request*, not on delivery, matches GameplayPlan.md's framing that spending is what happens during the window, delivery timing is a separate, later concern.

### 3.4 Drop scheduling — arrival at drop windows, not instantly

Per GameplayPlan.md: "Items purchased in a resupply window arrive at the next drop window (authored intervals during phases)... not instant." Concretely: `ACallInManager` also owns a repeating, phase-independent `FTimerHandle` ("drop window ticker") firing at an authored interval (`EditAnywhere`, e.g. every 45s during active phases). `ScheduleDelivery` does not start its own arbitrary-offset timer per request — instead it queues the request and `ExecuteDelivery` is only called on the *next* drop-window ticker firing that occurs at or after `RequestTime + Definition->DropDelaySeconds`. This two-part scheme (a minimum authored delay, snapped to the next global drop window) is what actually produces the "arrives at a scheduled interval, not instantly" feel GameplayPlan.md asks for, rather than merely a per-item countdown, which alone wouldn't align with phase pacing (Risk #4 in the Master Brief).

### 3.5 Squad vote / host approval

Simple majority-of-connected-players model for the demo (full "vote UI with individual ballots" is more infrastructure than 2 months supports for a system this secondary) — `bRequiresSquadVote == true` call-ins (High cost tier: chopper, artillery, temporary squad member) require either (a) host approval — the session host's player has a one-click approve, appropriate for small squads — or (b) if the project's session model treats all players as peers with no privileged host role (confirm in File 08), a simple "any player requests, all *other* connected players get a 10-second accept/reject prompt, majority of responses (ties default to reject) approves." Implement as `Server_CastCallInVote(bool bApprove)` RPC on `AShooterPlayerState`, tallied by `ACallInManager` before calling `ValidateRequest`'s vote check. For a 1-player solo test session, the vote step is automatically satisfied (no one else to vote) — this must be explicitly handled, not just "happens to work because the loop has zero other voters," to avoid a soft-lock where a solo player can never approve their own high-tier call-in.

### 3.6 Low/Medium tier call-ins (Week 5)

- **`ASupplyCrateBase`** (Mission/Economy/Actors/) — common base for ammo/medical crates: spawns at `TargetLocation`, replicated, destroyed after `DespawnDelay`.
- **`ASupplyCrate_Ammo`** — on `BeginPlay`, reports an AI hearing stimulus per File 02 Section 5.1 ("draws nearby enemies" — `bAttractsEnemies = true` on its `UCallInDefinition`); overlap-triggered resupply of the interacting player's reserve ammo via existing `UInventoryComponent`/ammo item stack logic (do not duplicate ammo-management logic here — call into the existing ammo item system).
- **`ASupplyCrate_Medical`** — heals + replenishes consumables on overlap; `DespawnDelay = 30.f` fixed per GameplayPlan.md.

### 3.7 Fire support call-ins (Week 7)

- **`AFireSupportStrikeBase`** — common base: `AuthoredDelay` (10s for mortar per GameplayPlan.md, longer for artillery), then `UGameplayStatics::ApplyRadialDamage` at the target point with a designer-set radius, plus a Niagara VFX cue.
- **`AFireSupportStrike_Mortar`** — Medium tier, 10-second delay, moderate radius, no proximity restriction beyond normal friendly-fire rules (`AShooterGameGameMode::bFriendlyFireEnabled`, existing field — mortar damage respects it exactly like player damage does).
- **`AFireSupportStrike_Artillery`** — High tier, `bRequiresPlayerProximitySafetyCheck = true`: `ValidateRequest` additionally rejects the request if any living player is within a `MinSafeDistance` (`EditAnywhere` on the definition) of `TargetLocation` at request time — "not usable near players" per GameplayPlan.md is enforced as a hard request-time validation failure, not merely a warning, since this is a griefing/accident vector even in a co-op-only game (a player could otherwise call artillery on a teammate's position).
- **`AFireSupportStrike_Chopper`** — High tier, a single strafing-run actor that moves along an authored spline/path across the marked corridor applying damage in a moving line rather than a single radial point — implemented as a simple `AActor` with a `USplineComponent` and a timeline-driven movement + periodic radial-damage tick along its path, not a full flyable vehicle.
- **`ADistractionFlare`** — Low tier, spawns at target, registers periodic AI stimuli per File 02 Section 5.2, self-destructs after 90s.

---

## 4. Kit Swap Economy Integration

Kit swap itself is fully specified in File 06 — this file's responsibility is only the economy plumbing: `Server_SwapToPreset` on `AShooterPlayerState` performs the same phase-gate check as Section 2.2, deducts `CostFromSharedFund`-equivalent (a fixed `KitSwapCost`, `EditAnywhere` on a small `UResupplyEconomyConfig` data asset alongside `UScoreValuesDataAsset`) from `SquadSharedFund` via `UScoringSubsystem`, then calls into File 06's actual loadout-swap logic. This keeps the "does this cost currency and is the window open" concern in the economy layer and the "how do I actually change your equipped gear" concern in the loadout layer — consistent with the separation used everywhere else in this plan (e.g., File 04's extraction point computing a reward but never mutating inventory directly).

---

## 5. UE5 References

- **UMG** — `UW_ResupplyOverlay`, call-in purchase buttons, drop-incoming indicators: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see User Interface → UMG UI Designer)
- **Replicated GameState** — `SquadSharedFund`, `ActiveWindowEndServerTime`: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Networking and Multiplayer → Actor Replication)
- **Data Assets (`UPrimaryDataAsset`)** — `UCallInDefinition`, `UScoreValuesDataAsset`: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Content → Data Assets)
- **`UGameplayStatics::ApplyRadialDamage`** — mortar/artillery/chopper strike damage: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Programming → Gameplay Statics)
- **Niagara** — strike VFX cues: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Visual Effects → Niagara)
- **RPCs (Server/Client/NetMulticast)** — `Server_RequestCallIn`, `Server_CastCallInVote`: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Networking and Multiplayer → RPCs)

---

## 6. Week-by-Week Tasks

**Week 5 (Aug 10 – Aug 16)**
- Implement `PersonalPoints`/`SquadSharedFund` fields + `OnRep` + delegates.
- Build `UScoringSubsystem`, `UScoreValuesDataAsset`, wire all Section 1.5 award call sites.
- Implement resupply-window-as-phase model (mostly File 01 territory, but the "unspent points are lost" behavior and the phase-gate purchase guard are built here).
- Build `UW_ResupplyOverlay` first pass (countdown + balances, no purchase buttons yet).
- Build `UCallInDefinition`, `ACallInManager` skeleton (`ValidateRequest` without vote logic yet, `ScheduleDelivery`/drop-window ticker).
- Build `ASupplyCrateBase`, `ASupplyCrate_Ammo`, `ASupplyCrate_Medical` — wire into `UW_ResupplyOverlay` purchase buttons.
- **Checkpoint (mid-plan):** a solo player can earn points, enter a resupply window, purchase an ammo crate, and see it arrive on a delay.

**Week 6 (Aug 17 – Aug 23)**
- Implement `Server_SwapToPreset` economy gating (Section 4), coordinating with File 06's Week 6 loadout work.
- Extraction reward application handoff (`ApplyExtractionReward`) tested against File 04's Week 6 exfil vehicle work.

**Week 7 (Aug 24 – Aug 30)**
- Implement squad vote/host approval (Section 3.5), including the solo-session edge case.
- Build `AFireSupportStrikeBase` and all four strike/flare subclasses (Section 3.7).
- Build `ADistractionFlare`, coordinate with File 02's Week 7 stimulus-response tuning.
- Tune drop-window interval, `WindowDuration` values, and all `UScoreValuesDataAsset` point values against Weeks 5–6 playtesting data.

---

## 7. Acceptance Criteria

- [ ] Every point award in Section 1.5 fires exactly once per qualifying event (no double-counting on, e.g., an assist also counting as a kill).
- [ ] `SquadSharedFund` is identical across host and all clients at all times.
- [ ] Purchases are rejected server-side (not just hidden client-side) if attempted outside a `Resupply*` phase — verify by attempting a call-in RPC directly via debug command during `Phase2Objective`.
- [ ] Unspent `SquadSharedFund`/`PersonalPoints` persist (are not refunded or cleared) but simply become unspendable once the resupply phase ends.
- [ ] A purchased call-in does not spawn instantly — it arrives no earlier than `DropDelaySeconds` after request, snapped to the next drop-window tick, verified by timing at least 3 requests made at different points within a window.
- [ ] Ammo crate spawn reliably triggers an AI perception hearing event that redirects at least one nearby wandering zombie (cross-checked against File 02's acceptance criteria).
- [ ] Medical crate despawns at exactly 30 seconds if unused.
- [ ] Artillery strike request is rejected if any living player is within `MinSafeDistance` of the target point.
- [ ] Chopper strike damages along its full authored path, not just at a single point.
- [ ] High-tier call-ins requiring a squad vote cannot be purchased solo without the solo-session auto-approval path explicitly firing (verify no soft-lock in a 1-player session).
- [ ] `UW_ResupplyOverlay` appears automatically on entering `Resupply1`/`Resupply2` and disappears/disables on phase exit, with zero manual triggering.

---

## 8. Estimated Effort

| Task Cluster | Estimate |
|---|---|
| Points/shared fund fields + `UScoringSubsystem` + `UScoreValuesDataAsset` + award call sites | 4 dev-days |
| Resupply window phase-gate + `UW_ResupplyOverlay` | 3 dev-days |
| `UCallInDefinition` + `ACallInManager` (validation, drop scheduling) | 4 dev-days |
| Ammo + medical crates | 2.5 dev-days |
| Squad vote / host approval + solo-session handling | 2 dev-days |
| Fire support strikes (mortar, artillery, chopper) + distraction flare | 5 dev-days |
| Kit swap economy gating (plumbing only, not the swap logic itself) | 1 dev-day |
| Tuning pass (window durations, drop intervals, point values) | 2 dev-days |
| **Total** | **23.5 dev-days** (spans Weeks 5, 6, and 7) |
