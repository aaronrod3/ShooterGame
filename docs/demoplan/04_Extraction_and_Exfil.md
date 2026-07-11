# 04 — Extraction and Exfil

## Dependencies

- [01_Phase_Triggers_and_Map_Structure.md](01_Phase_Triggers_and_Map_Structure.md) — reward tier calculation reads `AShooterGameState::CurrentMissionPhase`; `AExfilVehicle` departure fires `Phase4Exfil → PostMission`.
- [03_Objectives_and_Task_System.md](03_Objectives_and_Task_System.md) — Phase 4 task completion is what starts `AExfilVehicle`'s countdown.
- [05_Economy_and_Resupply.md](05_Economy_and_Resupply.md) — carried loot/points referenced in reward calculation come from that file's `PersonalPoints`/inventory state.
- [08_Multiplayer_and_Networking.md](08_Multiplayer_and_Networking.md) — early extraction must not end the session for remaining players; read the session/squad-membership model before building `AFieldExtractionPoint`.

---

## 1. Field Extraction Point Actor

### 1.1 `AFieldExtractionPoint`

New files: `Source/ShooterGame/Mission/Extraction/FieldExtractionPoint.h/.cpp`

Implements `IInteractable` (existing interface, same as `AObjectiveTerminal` in File 03). Available at any point in the mission per GameplayPlan.md ("Any player can individually extract at a field extraction point at any time") — unlike the objective terminal, this actor has **no** zone/phase lock on `CanInteract`.

```cpp
UCLASS()
class SHOOTERGAME_API AFieldExtractionPoint : public AActor, public IInteractable
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Extraction")
    float ExtractionDuration = 30.f; // GameplayPlan.md: "~30 seconds, player remains vulnerable"

    UPROPERTY(EditAnywhere, Category = "Extraction")
    bool bIsEmergencyExtraction = false; // Phase 4 secondary fallback, see Section 4

    virtual void Interact_Implementation(ACharacter* InstigatingCharacter) override;
    virtual bool CanInteract_Implementation(ACharacter* InstigatingCharacter) const override;
    virtual FText GetInteractPromptText_Implementation(ACharacter* InstigatingCharacter) const override;

private:
    // Per-player extraction state — multiple players can extract independently and
    // concurrently at the same point, so this is keyed per-instigator, not a single
    // actor-wide state (see Section 1.3).
    UPROPERTY(ReplicatedUsing = OnRep_ActiveExtractions)
    TArray<FActiveExtraction> ActiveExtractions;

    UFUNCTION() void OnRep_ActiveExtractions();
    void TickExtraction(APlayerState* PlayerState); // per-player FTimerHandle
    void CompleteExtraction(APlayerState* PlayerState);
    void CancelExtraction(APlayerState* PlayerState, const FString& Reason);
};

USTRUCT()
struct FActiveExtraction
{
    GENERATED_BODY()
    UPROPERTY() TWeakObjectPtr<APlayerState> Player;
    UPROPERTY() float ElapsedTime = 0.f;
};
```

### 1.2 Vulnerability window

`ExtractionDuration` is a channel, not invulnerability — GameplayPlan.md is explicit that the player "remains vulnerable" during the sequence. No damage immunity is applied. If the extracting player takes damage above a small threshold, or moves outside the extraction point's radius, `CancelExtraction` is called and `ElapsedTime` resets to 0 (not paused) — this preserves the tension the design calls for; a paused/resumable timer would remove the risk entirely. Movement-based cancellation reuses the same velocity-check pattern as File 03's stationary-task detection for consistency.

### 1.3 Concurrent multi-player extraction

Because "solo extract does not end the session for remaining squadmates" (GameplayPlan.md), and multiple players may want to extract at different times or even simultaneously at the same point, extraction state is tracked per-`APlayerState` in an array on the extraction point actor rather than as a single shared actor state. This avoids one player's extraction blocking or interfering with another's.

---

## 2. Phase-Based Reward Calculation

### 2.1 `UExtractionRewardLibrary`

New files: `Source/ShooterGame/Mission/Extraction/ExtractionRewardLibrary.h/.cpp` — a `UBlueprintFunctionLibrary` (stateless, pure calculation) rather than a subsystem, since this is a single deterministic function of mission state with no persistent state of its own.

```cpp
UCLASS()
class SHOOTERGAME_API UExtractionRewardLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Extraction")
    static FExtractionReward CalculateReward(EMissionPhase PhaseAtExtraction, bool bIsEmergencyExfil, bool bIsPrimaryExfil);
};

USTRUCT(BlueprintType)
struct FExtractionReward
{
    GENERATED_BODY()
    UPROPERTY() float XPMultiplier = 0.f;
    UPROPERTY() bool bRetainCarriedLoot = false;
    UPROPERTY() bool bAwardKillsCredit = false;
    UPROPERTY() bool bAwardTaskReward = false;
    UPROPERTY() float CurrencyPayoutMultiplier = 1.f;
};
```

The tier table is implemented as a straightforward internal `switch` on `EMissionPhase` (plus the two bool flags for the Phase 4 branch) mapping directly to GameplayPlan.md's "Early Extraction Reward Tiers" table:

| `PhaseAtExtraction` (phase reached, not phase active) | Tier |
|---|---|
| Before `Phase1Infiltration` completes (still `Phase1Infiltration`) | Minimal XP only, no loot |
| `Resupply1` or later, before Phase 2 completes | Partial XP + kills credit |
| `Phase3Exfil` or later, before Phase 3 completes (i.e. Phase 2 done) | Significant XP + task reward + carried loot |
| `Resupply2` or later (Phase 3 done) | Full XP for Phases 1–3 + carried loot |
| `Phase4Exfil`, `bIsPrimaryExfil == true` | Full mission reward tier |
| `Phase4Exfil`, `bIsEmergencyExfil == true` | Full XP, reduced currency payout |

Since this is pure data mapped to logic, keeping it as literal `switch` cases (not a `UCurveTable`) is the right call here — unlike wave escalation curves (File 02), this table has a small fixed number of discrete tiers that won't need smooth interpolation or frequent designer retuning; a `UDataTable` remains an easy future upgrade if that changes, but isn't justified for the demo.

### 2.2 Applying the reward

On `CompleteExtraction` (Section 1.1), `AFieldExtractionPoint` calls `UExtractionRewardLibrary::CalculateReward` and passes the result to `UScoringSubsystem` (File 05) to actually credit `PersonalPoints`/XP, and to `ULoadoutComponent`/`UInventoryComponent` to determine whether carried items are retained or stripped. The extraction point actor itself never mutates player economy or inventory state directly — it only computes *what should happen* and hands off to the systems that own that state, keeping with the single-responsibility pattern used throughout the project.

---

## 3. Doomed Zone Despawn After Pursuit Radius Cleared

This is primarily File 01/02 territory (`AZoneDefinition::Server_SetZoneState(Cleared)` on pursuit-radius exit, `AWaveSpawnManager` reacting to the state change) — restated here only for the extraction-relevant edge case: **a field extraction point located inside a doomed zone remains fully usable during the doomed state.** A player who decides mid-pursuit that they'd rather extract than keep running is allowed to — `CanInteract_Implementation` has no zone-state gate (Section 1). This is an intentional design consequence of GameplayPlan.md's "any player can individually extract at any time," and should be explicitly tested (Section 9) since it's an easy case to accidentally break if a future zone-lock convenience check gets added to `AFieldExtractionPoint` by analogy with `AObjectiveTerminal`.

---

## 4. Phase 4 Exfil Vehicle

### 4.1 `AExfilVehicle`

New files: `Source/ShooterGame/Mission/Extraction/ExfilVehicle.h/.cpp`

```cpp
UCLASS()
class SHOOTERGAME_API AExfilVehicle : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Exfil")
    float ArrivalDelayAfterUpload = 60.f; // seconds after Phase4Exfil begins

    UPROPERTY(EditAnywhere, Category = "Exfil")
    float BoardingWindowDuration = 45.f; // how long the vehicle waits at the LZ before departing

    UPROPERTY(ReplicatedUsing = OnRep_VehicleState, BlueprintReadOnly, Category = "Exfil")
    EExfilVehicleState VehicleState = EExfilVehicleState::EnRoute;

    UPROPERTY(BlueprintAssignable, Category = "Exfil")
    FOnVehicleDeparted OnVehicleDeparted;

protected:
    void HandlePhase4ExfilBegan(); // bound to AShooterGameState::OnMissionPhaseChanged
    void ArriveAtLZ();
    void CheckBoardedPlayers();
    void Depart();

    UFUNCTION() void OnRep_VehicleState();

private:
    FTimerHandle ArrivalTimerHandle;
    FTimerHandle DepartureTimerHandle;
    UPROPERTY() TArray<TWeakObjectPtr<APlayerState>> BoardedPlayers; // overlap-tracked via a trigger volume on the vehicle
};

UENUM(BlueprintType)
enum class EExfilVehicleState : uint8
{
    EnRoute,    // countdown running, not yet visible/landed
    AtLZ,       // landed, boarding window open
    Departing,  // boarding window closed, playing depart sequence
    Departed
};
```

`HandlePhase4ExfilBegan` starts `ArrivalTimerHandle` for `ArrivalDelayAfterUpload` seconds, then transitions to `AtLZ` and starts `DepartureTimerHandle` for `BoardingWindowDuration` seconds. On expiry, `Depart()` snapshots `BoardedPlayers` (tracked via overlap events on a trigger volume attached to the vehicle mesh, not a distance check, since "aboard" should mean physically inside/on the vehicle) and calls `Server_RequestPhaseAdvance(PostMission)` — but only after resolving left-behind players (Section 5) first, since `PostMission` should represent the mission's actual end state, not just "the vehicle left."

### 4.2 Countdown HUD

New widget: `UW_ExfilCountdown` (HUD/Widgets/) — binds to `AExfilVehicle::VehicleState` and a replicated countdown value (`ArrivalTimerHandle`/`DepartureTimerHandle` remaining time, exposed via `GetServerWorldTimeSeconds()`-based end-time fields on the vehicle rather than replicating a raw countdown float — see File 08 Section on replicated timers for why this avoids clock-drift jitter). Shown to all squad members once `Phase4Exfil` begins, regardless of distance from the LZ, so a player deep in the zone knows the clock is running.

---

## 5. Secondary Emergency Extraction Fallback

- A second `AFieldExtractionPoint` instance is placed near (but meaningfully distant from) the fixed LZ, with `bIsEmergencyExtraction = true`.
- It only becomes `CanInteract`-true once `AExfilVehicle::VehicleState == EExfilVehicleState::Departed` (or, generously, once it enters `Departing`, so a player sprinting to it doesn't get stuck in a dead window) — before that point, the primary vehicle is always the correct choice, and offering the emergency option earlier would undercut the primary exfil's stakes.
- Reward tier: `bIsEmergencyExfil = true` branch in `UExtractionRewardLibrary` (Section 2.1) — "full XP, reduced currency payout" per GameplayPlan.md.

---

## 6. Left-Behind Player State

### 6.1 Detecting left-behind players

At `AExfilVehicle::Depart()` (Section 4.1), any living player **not** in `BoardedPlayers` is left behind. This check happens once, at the moment of departure — not continuously — since "left behind" is a discrete event, not an ongoing state to poll.

### 6.2 Resolution paths

Two paths, matching GameplayPlan.md exactly:

1. **Secondary emergency extraction** (Section 5) — if the player reaches and completes an emergency extraction, they get the emergency reward tier (Section 2.1) and are removed from the mission cleanly.
2. **Death → island base respawn, full gear loss** — if a left-behind player dies before reaching emergency extraction, `AShooterGameGameMode::PlayerDied` (existing function, per its current signature taking `AShooterGameCharacter*` and `AShooterGamePlayerController*`) is extended to check a new `bWasLeftBehind` flag on `AShooterPlayerState`; if true, instead of the normal in-mission respawn flow, the player is routed to a "return to island base" transition (a level-travel or a dedicated respawn-at-base actor, scoped simply for the demo as a `ServerTravel`-free teleport to a base persistent level actor if the project is single-level, or an actual level transition if the island base already exists as a separate level — confirm which during Week 1 of this file's work) and `UInventoryComponent`/`ULoadoutComponent` are cleared to match "full gear loss," using the same clearing path already presumably exercised by normal permadeath-style loss if one exists, or a new `Server_ClearAllGear()` on `ULoadoutComponent` if not.

### 6.3 Session continuity

Neither path ends the session for other squad members — this is enforced structurally, not by a special case: `AShooterGameGameMode`'s existing `AlivePlayers` tracking and `HandleMatchOver` logic (confirmed present in `ShooterGameGameMode.h`) already governs match-end based on the *whole squad's* alive count, not per-mission-phase state, so a left-behind player dying or extracting does not by itself trigger `HandleMatchOver` as long as at least one other player is still active. Verify this interaction explicitly in Week 4/8 testing (Section 9) rather than assuming it "just works," since `AlivePlayers` was written before this plan's systems existed and its exact interaction with "extracted" (not dead, but also not an active mission participant) players needs a decision: an extracted player should be removed from `AlivePlayers`-driven match-over accounting without being counted as a death.

---

## 7. UE5 References

- **Actor lifecycle** — `AFieldExtractionPoint` per-player extraction tracking, `AExfilVehicle` state transitions: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Programming → Actor Lifecycle)
- **GameMode** — extending `AShooterGameGameMode::PlayerDied` for left-behind handling: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Gameplay Framework → Game Mode and Game State)
- **PlayerState persistence** — `bWasLeftBehind`, `BoardedPlayers` tracking across the vehicle's lifetime: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Gameplay Framework → Player State)
- **Replicated timers** — countdown HUD, extraction vulnerability window: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Networking and Multiplayer → Actor Replication, Gameplay Timers)
- **Trigger volumes / overlap events** — `BoardedPlayers` detection on the exfil vehicle: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Physics → Collision → Overlap Events)

---

## 8. Week-by-Week Tasks

**Week 4 (Aug 3 – Aug 9)**
- Build `AFieldExtractionPoint` (Sections 1.1–1.3): per-player state array, vulnerability/cancellation logic.
- Build `UExtractionRewardLibrary` and the phase-tier `switch` (Section 2).
- Wire `CompleteExtraction` → `UScoringSubsystem` handoff (stub `UScoringSubsystem` interface now if File 05 hasn't started it yet; full implementation lands in Week 5).
- Place first field extraction point (Zone 1 / transit corridor per File 01's map layout) and playtest the full vulnerability-window/cancel-on-damage behavior.
- **Checkpoint B dependency:** early extraction working is required for this week's "Phases 1–3 playable, exfil works" checkpoint.

**Week 6 (Aug 17 – Aug 23)**
- Build `AExfilVehicle` (Section 4): arrival/boarding/departure state machine, `UW_ExfilCountdown`.
- Build secondary emergency extraction point + its conditional `CanInteract` gate (Section 5).
- Implement left-behind detection and both resolution paths (Section 6).
- Verify `AlivePlayers`/`HandleMatchOver` interaction with extracted and left-behind players explicitly (Section 6.3) — do not assume, test with a 2-client session where one player extracts early and the other completes the full mission.
- **Checkpoint C dependency:** full 4-phase arc playable requires this week's exfil vehicle work to be complete.

---

## 9. Acceptance Criteria

- [ ] Extraction is available at a field extraction point at any mission phase, including inside a doomed zone.
- [ ] Taking damage or moving out of range during the 30-second extraction sequence cancels it and resets progress to zero (not paused).
- [ ] Two players can extract concurrently and independently at the same extraction point without interfering with each other's timers.
- [ ] Reward tier awarded on extraction exactly matches the table in GameplayPlan.md's "Early Extraction Reward Tiers" for all 6 tiers, verified by triggering extraction at each phase boundary in a debug session.
- [ ] The exfil vehicle arrives at the fixed LZ at `ArrivalDelayAfterUpload` seconds after Phase 4 task completion, consistently (not drifting between runs — verify with server-time-based countdown, not a naive `DeltaTime` accumulator).
- [ ] A player aboard the vehicle at departure receives the full primary-exfil reward tier; a player not aboard is correctly flagged left-behind.
- [ ] A left-behind player can successfully use the secondary emergency extraction point and receives the emergency reward tier.
- [ ] A left-behind player who dies before reaching emergency extraction respawns at the island base with confirmed full gear loss (`UInventoryComponent`/`ULoadoutComponent` empty on respawn).
- [ ] Early extraction or left-behind death for one player does not trigger `HandleMatchOver` while at least one other squad member is still active in the mission.
- [ ] `UW_ExfilCountdown` is visible to and synchronized across all squad members regardless of their distance from the LZ.

---

## 10. Estimated Effort

| Task Cluster | Estimate |
|---|---|
| `AFieldExtractionPoint` (per-player state, vulnerability window, cancellation) | 3 dev-days |
| `UExtractionRewardLibrary` + reward application handoff | 1.5 dev-days |
| `AExfilVehicle` state machine + server-time-based countdown | 3.5 dev-days |
| `UW_ExfilCountdown` widget | 1.5 dev-days |
| Secondary emergency extraction + conditional gating | 1 dev-day |
| Left-behind detection + island-base respawn/gear-loss path | 2.5 dev-days |
| `AlivePlayers`/`HandleMatchOver` interaction verification and fix-up | 1.5 dev-days |
| **Total** | **14.5 dev-days** (spans Weeks 4 and 6) |
