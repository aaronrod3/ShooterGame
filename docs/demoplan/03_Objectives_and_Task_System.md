# 03 — Objectives and Task System

## Dependencies

- [01_Phase_Triggers_and_Map_Structure.md](01_Phase_Triggers_and_Map_Structure.md) — task completion calls `AMissionStateManager::Server_RequestPhaseAdvance`; task start/overrun interacts with `AZoneDefinition` state.
- [02_Enemy_AI_and_Waves.md](02_Enemy_AI_and_Waves.md) — task start triggers the "pressure response" spawn-rate change; overrun timer expiry hands off to that file's doomed-zone logic.
- Reuses the existing `IInteractable` interface (`Source/ShooterGame/Interaction/Interactable.h`) — do not create a parallel interaction system.

---

## 1. Stationary Interaction System

### 1.1 `AObjectiveTerminal`

New files: `Source/ShooterGame/Mission/Objectives/ObjectiveTerminal.h/.cpp`

An `AActor` implementing `IInteractable` (existing interface — `Interact`, `CanInteract`, `GetInteractPromptText`), placed once per zone-phase (Zone 1 gets one for Phase 2's data download, Zone 2 gets one for Phase 4's comms upload). It owns a `UObjectiveTaskComponent` rather than implementing task logic itself, keeping the actor a thin world-presence shell — consistent with the project's stated preference for single-responsibility components (see `AShooterGameCharacter`'s component composition in CLAUDE.md).

```cpp
UCLASS()
class SHOOTERGAME_API AObjectiveTerminal : public AActor, public IInteractable
{
    GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UObjectiveTaskComponent> TaskComponent;

    // IInteractable
    virtual void Interact_Implementation(ACharacter* InstigatingCharacter) override;
    virtual bool CanInteract_Implementation(ACharacter* InstigatingCharacter) const override;
    virtual FText GetInteractPromptText_Implementation(ACharacter* InstigatingCharacter) const override;

    UPROPERTY(EditAnywhere, Category = "Objective")
    TObjectPtr<AZoneDefinition> OwningZone; // locked until this zone clears — see Section 2

    UPROPERTY(EditAnywhere, Category = "Objective")
    EMissionPhase RequiredPhase; // only interactable during this phase
};
```

`Interact_Implementation` calls `TaskComponent->Server_BeginTask(InstigatingCharacter)`. `CanInteract_Implementation` returns false if `OwningZone->CurrentZoneState != EZoneState::Cleared`, if `AShooterGameState::CurrentMissionPhase != RequiredPhase`, or if the task is already complete/in-progress by another player.

### 1.2 `UObjectiveTaskComponent`

New files: `Source/ShooterGame/Components/ObjectiveTaskComponent.h/.cpp`

Follows the project's mandatory replication pattern exactly (per CLAUDE.md: `ReplicatedUsing=OnRep_X` + `Server_X()` + `OnRep_X()` broadcasting a delegate — "Never poll replicated arrays/structs directly").

```cpp
UCLASS(ClassGroup=(Mission), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UObjectiveTaskComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Task")
    float TaskDuration = 60.f; // seconds of continuous progress required

    UPROPERTY(EditAnywhere, Category = "Task")
    int32 RequiredStationaryPlayers = 1; // GameplayPlan.md: "one or more players stationary"

    UPROPERTY(EditAnywhere, Category = "Task")
    float OverrunTimeLimit = 240.f; // File 02 §4 — zone goes Doomed if this elapses

    UPROPERTY(ReplicatedUsing = OnRep_TaskProgress, BlueprintReadOnly, Category = "Task")
    float TaskProgress = 0.f; // 0..TaskDuration

    UPROPERTY(ReplicatedUsing = OnRep_TaskState, BlueprintReadOnly, Category = "Task")
    ETaskState CurrentTaskState = ETaskState::Locked;

    UPROPERTY(BlueprintAssignable, Category = "Task")
    FOnTaskProgressChanged OnTaskProgressChanged;

    UPROPERTY(BlueprintAssignable, Category = "Task")
    FOnTaskStateChanged OnTaskStateChanged;

    UFUNCTION(BlueprintCallable, Category = "Task")
    void Server_BeginTask(ACharacter* Instigator);

protected:
    void TickTaskProgress(); // server-only FTimerHandle, ~4Hz — see 1.4
    void CheckStationaryPlayerCount();
    void HandleOverrunExpired();

    UFUNCTION() void OnRep_TaskProgress();
    UFUNCTION() void OnRep_TaskState();

private:
    FTimerHandle ProgressTickHandle;
    FTimerHandle OverrunTimerHandle;
    TArray<TWeakObjectPtr<ACharacter>> PlayersOnTask;
};
```

```cpp
UENUM(BlueprintType)
enum class ETaskState : uint8
{
    Locked,       // zone not yet cleared, or wrong phase
    Available,    // interactable, not yet started
    InProgress,   // at least one player stationary on it, progress ticking
    Stalled,      // was in progress, required player count dropped — progress holds, does not decay
    Complete
};
```

### 1.3 Stationary detection

"Stationary" is defined as: the instigating character's `ACharacter::GetVelocity().SizeSquared()` below a small threshold (`EditAnywhere` constant, ~2500 uu²/s² to allow minor idle sway) **and** within the terminal's interaction radius, checked every tick of `TickTaskProgress` (not every frame — the ~4Hz timer in 1.4 is sufficient resolution for this check and avoids per-frame velocity polling for a slow-moving progress bar). If the count of qualifying players drops below `RequiredStationaryPlayers`, state moves to `Stalled` (progress holds, does not tick up or down) rather than `Locked` or resetting to zero — this avoids punishing a squad for a player briefly stepping off to fight, which would otherwise fight the "rest of squad defends while task runs" pillar in GameplayPlan.md.

### 1.4 Progress ticking

`Server_BeginTask` starts a repeating `FTimerHandle` at ~4Hz (0.25s interval) calling `TickTaskProgress`, which increments `TaskProgress` by `0.25 * RequiredStationaryPlayers-scaled rate` only while `CurrentTaskState == InProgress`. A 4Hz tick is enough resolution for a UMG progress bar to read as smooth (File 03 Section 1.5) without the cost of a per-frame `Tick()` on every terminal in the level. `OnRep_TaskProgress` broadcasts `OnTaskProgressChanged` on clients purely for UI binding — clients never compute progress themselves, only display the replicated value (server-authoritative, per File 08's authority model).

On `TaskProgress >= TaskDuration`: set `CurrentTaskState = ETaskState::Complete`, clear `OverrunTimerHandle`, and call `AMissionStateManager::Server_RequestPhaseAdvance` with the appropriate next phase (Section 4).

### 1.5 UMG Progress Bar

New widget: `Source/ShooterGame/HUD/Widgets/` → `UW_ObjectiveProgressBar` (UMG Widget Blueprint, C++ base class `UObjectiveProgressBarWidget` if native binding is preferred over pure-BP binding). Binds to the local player's currently-tracked `UObjectiveTaskComponent` (found via the nearest `AObjectiveTerminal` in interaction range, or a squad-wide "active task" reference broadcast so squadmates defending nearby also see progress, not just the player standing on it — this matters because GameplayPlan.md explicitly describes the rest of the squad defending while watching the task run).

---

## 2. Objective Zone Interactable State Management

Locking is handled entirely through `CanInteract_Implementation` reading `AZoneDefinition::CurrentZoneState` and `AShooterGameState::CurrentMissionPhase` (Section 1.1) — there is no separate "lock" state to manage on the terminal itself beyond what `ETaskState::Locked` already represents. This keeps the objective terminal a pure consumer of state owned elsewhere (File 01's zone state, and its own task component), matching the "state machine never calls directly into other systems" decoupling principle established in File 01.

`AObjectiveTerminal::TaskComponent->CurrentTaskState` transitions `Locked → Available` automatically when both `OwningZone->CurrentZoneState == Cleared` (or, for Phase 1/Phase 2 overlap — see Section 3 — `Active` is sufficient) and `AShooterGameState::CurrentMissionPhase == RequiredPhase` become true simultaneously; this is driven by binding `UObjectiveTaskComponent` to both `AZoneDefinition::OnZoneStateChanged` and `AShooterGameState::OnMissionPhaseChanged` in `BeginPlay`, re-evaluating the lock condition on either event rather than polling.

---

## 3. Phase 2 Task Pressure Response

Per GameplayPlan.md: "Enemy waves arrive in increasing intensity" specifically once the task begins, not merely once the zone is active. Concretely:

- `UObjectiveTaskComponent::Server_BeginTask` (the moment `CurrentTaskState` first becomes `InProgress`) fires a one-time notification — `AZoneDefinition` exposes `Server_NotifyTaskEngaged()`, called from the task component — which tells `AWaveSpawnManager` (File 02) to advance that zone's `CurrentWaveIndex` forward to its "task pressure" wave subset (a `UWaveSetDataAsset` can mark specific `FWaveEntry` rows with a `bRequiresTaskEngaged` flag that `AWaveSpawnManager` skips until this notification fires).
- This means Phase 1's clearance waves and Phase 2's task-pressure waves can live in the *same* `UWaveSetDataAsset` for a zone, gated by this single bool, rather than requiring the manager to swap data assets mid-zone — simpler state to reason about, one fewer transition edge to test.
- The overrun timer (`OverrunTimeLimit`, Section 1.2) starts at the same moment task engagement begins, not at zone clearance — this matches GameplayPlan.md's framing that the overrun clock is about task completion pressure specifically.

---

## 4. Phase 4 Comms Tower Upload Task

Structurally identical to the Phase 2 terminal — same `AObjectiveTerminal` class, same `UObjectiveTaskComponent`, a second instance placed in Zone 2 with `RequiredPhase = EMissionPhase::Phase4SecondObjective`. No new class is needed; this is a content/placement task, not an engineering task, which is why File 01 recommended `AreaControl` clearance mode for Zone 2 — it's the one place in the demo where a second clearance mode gets validated, and reusing the identical task component here keeps that the only real variance between the two zones.

On completion, `Server_RequestPhaseAdvance(EMissionPhase::Phase4Exfil)` — this is also the trigger File 04 listens for to begin the exfil vehicle's fixed-time countdown (per GameplayPlan.md: "Extraction vehicle... arrives at a fixed LZ at a fixed time **after upload completes**").

---

## 5. Objective Completion Events → Phase Transitions

Summary table (cross-referenced from File 01 Section 1.4, restated here from the task system's perspective):

| Task Component Event | Downstream Effect |
|---|---|
| `Server_BeginTask` (Phase 2 terminal) | `AZoneDefinition::Server_NotifyTaskEngaged()` → wave pressure response (Section 3); overrun timer starts |
| `TaskProgress >= TaskDuration` (Phase 2 terminal) | `AMissionStateManager::Server_RequestPhaseAdvance(Phase3Exfil)` |
| `OverrunTimeLimit` elapsed, task not complete | `AZoneDefinition::Server_SetZoneState(Doomed)` directly (File 02 §4) — mission does not hard-fail, zone becomes escapable-only |
| `Server_BeginTask` (Phase 4 terminal) | Same pressure-response pattern on Zone 2 |
| `TaskProgress >= TaskDuration` (Phase 4 terminal) | `AMissionStateManager::Server_RequestPhaseAdvance(Phase4Exfil)` → triggers `AExfilVehicle` countdown (File 04) |

---

## 6. UE5 References

- **UMG (Unreal Motion Graphics)** — `UW_ObjectiveProgressBar`: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see User Interface → UMG UI Designer)
- **GameState replication** — `TaskProgress`/`CurrentTaskState` replicated fields, consumed by client-side UMG binding: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Networking and Multiplayer → Actor Replication)
- **Custom interaction (existing `IInteractable`)** — this plan deliberately does **not** introduce the Gameplay Ability System for interaction; the project's existing lightweight interface is sufficient and GAS is explicitly gated behind a "confirm first" module-dependency rule in CLAUDE.md. Reference only if a future milestone justifies GAS adoption: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Gameplay Systems → Gameplay Ability System, for future reference only)
- **`FTimerManager`** — 4Hz progress tick, overrun timer: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Programming → Gameplay Timers)

---

## 7. Week-by-Week Tasks

**Week 3 (Jul 27 – Aug 2)**
- Build `AObjectiveTerminal` + `UObjectiveTaskComponent` (Sections 1.1–1.4).
- Implement stationary-player detection and `Locked`/`Available`/`InProgress`/`Stalled`/`Complete` state machine.
- Wire zone-clearance-gated unlock (Section 2).
- Place and configure the Phase 2 terminal in Zone 1.
- Build `UW_ObjectiveProgressBar` first pass (local-player only; squad-visibility can follow in Week 4).

**Week 4 (Aug 3 – Aug 9)**
- Implement task-engaged wave-pressure notification (Section 3), coordinate with File 02's `AWaveSpawnManager` for the `bRequiresTaskEngaged` wave-row gating.
- Implement overrun timer and its doomed-zone handoff.
- Wire `TaskProgress >= TaskDuration` → phase advance.
- Extend `UW_ObjectiveProgressBar` to be squad-visible (defending teammates see progress, not just the player on the terminal).

**Week 6 (Aug 17 – Aug 23)**
- Place and configure the Phase 4 terminal in Zone 2 (content task, reuses all Week 3–4 engineering).
- Verify Phase 4 completion correctly hands off to `AExfilVehicle` countdown start (File 04 dependency, both files' owners should test this integration point together this week).

---

## 8. Acceptance Criteria

- [ ] Terminal is not interactable (`CanInteract` returns false, no prompt shown) until its owning zone reaches the required clearance state and the mission is in the required phase.
- [ ] Progress bar only advances while the required number of players are stationary within range; stepping away holds progress (does not reset to zero).
- [ ] Progress bar value is identical on host and all clients at all times.
- [ ] Enemy wave intensity visibly increases within the zone specifically once the task begins (not merely once the zone was cleared) — verify via File 02's spawn-count logging before/after `Server_BeginTask`.
- [ ] If the overrun timer expires before task completion, the zone enters `Doomed` state and the mission does not crash, hang, or silently continue as if nothing happened — the squad must be able to see/feel the zone has become unwinnable.
- [ ] Task completion at Zone 1 correctly triggers `Phase3Exfil`; task completion at Zone 2 correctly triggers `Phase4Exfil`.
- [ ] Two terminals (Phase 2 and Phase 4) run off the identical component/actor classes with no code duplication — only data/placement differs.

---

## 9. Estimated Effort

| Task Cluster | Estimate |
|---|---|
| `AObjectiveTerminal` + `IInteractable` implementation | 1.5 dev-days |
| `UObjectiveTaskComponent` (state machine, replication, stationary detection, progress tick) | 4 dev-days |
| Zone-clearance-gated unlock wiring | 1 dev-day |
| Task-engaged wave-pressure notification + overrun timer/doomed handoff | 2 dev-days |
| `UW_ObjectiveProgressBar` (local + squad-visible) | 2.5 dev-days |
| Phase 2 + Phase 4 terminal placement/content | 1 dev-day |
| **Total** | **12 dev-days** (spans Weeks 3, 4, and 6) |
