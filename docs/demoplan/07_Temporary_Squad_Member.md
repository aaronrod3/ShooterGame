# 07 — Temporary Squad Member

## Dependencies

- [02_Enemy_AI_and_Waves.md](02_Enemy_AI_and_Waves.md) — reuses `BTTask_TakeCover`, `BTTask_SuppressTarget`, and `EQS_FindCoverNearTarget` built for human AI reinforcement rather than re-authoring cover/suppression logic from scratch.
- [05_Economy_and_Resupply.md](05_Economy_and_Resupply.md) — deployment is a High-tier call-in purchased through `ACallInManager`, including the squad-vote requirement.
- [06_Kit_Presets_and_Loadout.md](06_Kit_Presets_and_Loadout.md) — the squad member is outfitted from one of the requesting player's stored `FKitPreset`s, reusing `ApplyKitPresetToCharacter`.
- [08_Multiplayer_and_Networking.md](08_Multiplayer_and_Networking.md) — command dispatch is a server-authoritative RPC path; read the authority model first.

**This system is built last in the schedule (Week 7, after every other file is functionally complete) — see the risk note in Section 7 before starting.**

---

## 1. AI Soldier Actor

### 1.1 `ATemporarySquadMember`

New files: `Source/ShooterGame/Mission/SquadMember/TemporarySquadMember.h/.cpp`

An `ACharacter` subclass, spawned server-side by `ACallInManager::ExecuteDelivery` (File 05) when the `TemporarySquadMember` call-in type resolves. Reuses `UHitZoneComponent` (already shared between player and zombie characters per CLAUDE.md) for consistent damage handling — a temporary squad member should take headshot/bodyshot damage multipliers exactly like a player or zombie, not a bespoke calculation.

```cpp
UCLASS()
class SHOOTERGAME_API ATemporarySquadMember : public ACharacter
{
    GENERATED_BODY()
public:
    UPROPERTY(ReplicatedUsing = OnRep_CommandOwner, BlueprintReadOnly, Category = "Squad Member")
    TWeakObjectPtr<APlayerState> CommandingPlayer; // who can issue orders — the purchaser

    UPROPERTY(EditAnywhere, Category = "Squad Member")
    float DeploymentDuration = 400.f; // seconds — GameplayPlan.md "~5-8 min", 400s = 6.67min midpoint

    UFUNCTION(BlueprintCallable, Category = "Squad Member")
    void Server_OutfitFromPreset(const FKitPreset& Preset); // File 06's ApplyKitPresetToCharacter, targeted at self

    UFUNCTION(BlueprintCallable, Category = "Squad Member")
    void Server_IssueCommand(ESquadMemberCommand Command, AActor* OptionalTargetActor, FVector OptionalTargetLocation);

protected:
    virtual void Die() override; // gear loss handling, Section 5

private:
    FTimerHandle DeploymentExpiryHandle;
    UFUNCTION() void OnRep_CommandOwner();
    void HandleDeploymentExpired(); // despawns cleanly, distinct from death (Section 5)
};
```

`Server_OutfitFromPreset` is called once, immediately after spawn, before the character is visible — it calls the exact same `ApplyKitPresetToCharacter` function built in File 06 Section 4.1, targeted at `this` instead of the purchasing player's own character. No parallel outfitting logic is written for this file; the entire point of building Kit Presets before Temporary Squad Member (per the recommended implementation order) is that this step becomes a one-line reuse.

### 1.2 `ASquadMemberAIController`

New files: `Source/ShooterGame/Mission/SquadMember/SquadMemberAIController.h/.cpp`

```cpp
UCLASS()
class SHOOTERGAME_API ASquadMemberAIController : public AAIController
{
    GENERATED_BODY()
public:
    virtual void OnPossess(APawn* InPawn) override; // runs BT_TemporarySquadMember, initializes Blackboard

protected:
    UPROPERTY() TObjectPtr<UBlackboardComponent> BlackboardComp;
    UPROPERTY() TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComp;
};
```

Blackboard keys: `CurrentCommand` (Enum, `ESquadMemberCommand`), `CommandTargetActor` (Object), `CommandTargetLocation` (Vector), `CommandingPlayerActor` (Object, for `Follow`/`CoverRetreat` — see Section 2).

---

## 2. Behavior Tree Design

### 2.1 `ESquadMemberCommand`

New file: `Source/ShooterGame/Types/SquadMemberTypes.h`

```cpp
UENUM(BlueprintType)
enum class ESquadMemberCommand : uint8
{
    HoldPosition,
    Follow,
    SuppressTarget,
    MoveTo,
    BreachRoom,
    CoverRetreat
};
```

### 2.2 `BT_TemporarySquadMember` structure

Root: a `Selector` gated by a `Decorator_CommandEquals` per branch, each branch reading the `CurrentCommand` Blackboard key. "Follows last issued command until new one given" (GameplayPlan.md) is satisfied structurally — the BT re-evaluates the same `CurrentCommand` value every tick until `Server_IssueCommand` overwrites it, no separate "command queue" needed.

| Command | BT Branch Behavior | New/Reused Nodes |
|---|---|---|
| `HoldPosition` | Stay at current location, face nearest perceived threat, fire if one is in range/LOS | `BTTask_HoldPosition` [CREATE] — mostly a no-op movement task plus a reused fire-at-perceived-target task |
| `Follow` | Maintain a trailing offset behind `CommandingPlayerActor`, break off to engage if directly threatened within a short reaction radius | `BTTask_FollowOwner` [CREATE] |
| `SuppressTarget` | Move to a covered firing position if not already in one, then suppress `CommandTargetActor` | Reuses `BTTask_TakeCover` + `BTTask_SuppressTarget` from File 02 directly |
| `MoveTo` | Path to `CommandTargetLocation`, engaging any threats encountered en route | `BTTask_MoveToPoint` [CREATE] — thin wrapper around standard `UAITask_MoveTo`, exists mainly to also handle "engage while moving" via a parallel branch |
| `BreachRoom` | Move to `CommandTargetLocation` (a designer/player-marked doorway/entry point), then execute a short scripted entry (move through + immediate 180° threat scan) rather than cautious approach | `BTTask_BreachRoom` [CREATE] — the single most bespoke node in this system; keep its scope to "move fast to point, snap-scan on arrival," do not attempt full room-clearing pathing logic |
| `CoverRetreat` | Move to nearest cover point *behind* `CommandingPlayerActor`'s position (biased away from the last perceived threat direction) and hold there | `BTTask_CoverRetreat` [CREATE] — reuses `EQS_FindCoverNearTarget` (File 02) with a modified query context: scores points behind the commanding player rather than near a target |

### 2.3 Command dispatch

`Server_IssueCommand` validates `CommandingPlayer` matches the RPC sender (only the purchasing player may command their squad member — GameplayPlan.md: "Player-directed... Player outfits it... commands via radial menu," implicitly the purchaser's own squad member, not a shared/communal one), then writes directly to the Blackboard via `ASquadMemberAIController::BlackboardComp->SetValueAsEnum/SetValueAsObject/SetValueAsVector`. This is a direct, immediate write — no additional replication needed beyond the RPC itself, since Blackboard state lives server-side only (matches every other AI controller in the project, which are server-authoritative with no client-side BT execution).

---

## 3. Radial Command Menu

New widget: `UW_RadialCommandMenu` (HUD/Widgets/) — opened by a held input action (reuse Enhanced Input pattern already established for `IA_ToggleView` per the Infima integration plan) while the player has a line of sight to (or is within a short interact range of) their own `ATemporarySquadMember`. Six wedges, one per `ESquadMemberCommand`.

- `HoldPosition` / `Follow` / `CoverRetreat` — fire immediately on selection, no further input needed, dispatch `Server_IssueCommand` with no target.
- `SuppressTarget` — requires a target actor; after wedge selection, enter a brief targeting mode (crosshair-based trace against enemy characters) before dispatching.
- `MoveTo` / `BreachRoom` — requires a target location; after wedge selection, enter a brief targeting mode (ground trace under crosshair, similar to a waypoint-marking flow) before dispatching.

This UI is scoped deliberately simple for the demo — a full 3D world-space marker/preview system for `MoveTo`/`BreachRoom` targeting is a nice-to-have, not required; a simple crosshair trace with a small on-screen confirm prompt is sufficient to prove the command loop works end-to-end.

---

## 4. Deployment Timer and One-Per-Team Enforcement

### 4.1 Deployment timer

`Server_OutfitFromPreset` (called once at spawn) starts `DeploymentExpiryHandle` for `DeploymentDuration` seconds. On expiry, `HandleDeploymentExpired()` — the squad member is despawned cleanly (a short "extracted"/"recalled" VFX cue, not a death) and **does not** trigger gear loss (Section 5's gear-loss path is specifically for death, matching GameplayPlan.md's "On death, gear is lost" — natural timer expiry is a different, non-punishing outcome, since the design intent is a time-boxed asset, not a gamble).

### 4.2 One-per-team enforcement

`AShooterGameState` gains:

```cpp
UPROPERTY(ReplicatedUsing = OnRep_ActiveSquadMember, BlueprintReadOnly, Category = "Squad Member")
TWeakObjectPtr<ATemporarySquadMember> ActiveSquadMember;
```

`ACallInManager::ValidateRequest` (File 05) adds a check specific to the `TemporarySquadMember` call-in type: reject the request if `AShooterGameState::ActiveSquadMember` is valid and not yet despawned/dead. Set on spawn, cleared on death (Section 5) or deployment expiry (Section 4.1) — a single shared reference on `AShooterGameState`, not a per-player flag, since GameplayPlan.md is explicit this is "one active **per team**," not one per player.

---

## 5. Death Handling — Gear Loss

`ATemporarySquadMember::Die()` (override, or bound to the same damage/death event path `AZombieCharacter`/player characters use through `UHitZoneComponent` and `TakeDamage()`) performs: (1) clear `AShooterGameState::ActiveSquadMember`, (2) do **not** attempt to return equipped items to the commanding player's inventory or presets — "gear is lost" per GameplayPlan.md means the outfit used to deploy this unit is simply gone, there is no drop-on-death loot pass for the demo (a stretch goal, not required — see File 10), (3) broadcast a clear notification to the commanding player (and ideally the squad) that their squad member has died, since losing the ability to issue further commands should be an obvious, legible moment, not a silent despawn.

---

## 6. One-Per-Team Enforcement — Interaction with Left-Behind/Session Rules

If the commanding player extracts early (File 04) or is the one left behind and dies, the still-deployed `ATemporarySquadMember` does **not** automatically despawn — it remains active under its last-issued command (most likely `HoldPosition` or `Follow`, which will simply stop following a now-absent player and behave like `HoldPosition`) until its own deployment timer expires or it dies, and it becomes uncommandable by anyone else (no command-transfer-to-another-player system for the demo — this is an explicit scope cut, noted in File 10). This avoids a special-case despawn rule that would need to interact with File 04's already-nontrivial left-behind logic, and matches GameplayPlan.md's framing that a squad member is a time-boxed or death-boxed asset, not tied to session-presence bookkeeping beyond that.

---

## 7. Risk Notes — Highest-Complexity System, Scope Reduction Options

This is explicitly the highest-implementation-complexity system in the entire feature list (Master Brief Risk #3) and is scheduled last for exactly that reason — if any prior week overruns, this is the file that absorbs the schedule pressure, not File 01–06.

**If Week 7 runs short on time, cut in this order:**

1. **Cut `BreachRoom` and `CoverRetreat` first** — these are the two most bespoke BT branches (Section 2.2) with the least code reuse from File 02. Shipping with 4 commands (`HoldPosition`, `Follow`, `SuppressTarget`, `MoveTo`) still satisfies GameplayPlan.md's stated first-pass floor ("keep to 4–6 BT commands first pass").
2. **Cut the crosshair-trace targeting flow for `MoveTo`** next, replacing it with "move to the player's current look-at point at the moment of wedge selection" (a single trace at command-issue time, no separate targeting sub-mode) — reduces UI complexity at the cost of precision.
3. **If the whole system is at risk of not landing at all**, the fallback is to ship the call-in purchase and outfit flow (Sections 1, 4.1, 4.2) with only `HoldPosition` and `Follow` wired — a squad member that spawns, is visibly equipped from your preset, and follows you around fighting whatever's nearby is still a recognizable, demonstrable version of the feature, even without a command menu. This should only be invoked if Week 8 integration testing (File 09) is otherwise at risk — check with the overall schedule buffer (Sep 7–9) before cutting this far.

Do **not** cut Section 5 (death/gear-loss) or Section 4.2 (one-per-team) under time pressure — both are small, cheap to implement relative to the BT work, and both are load-bearing for the acceptance criteria in File 10 (gear loss on death is explicitly listed as a Definition of Done item in the Master Brief).

---

## 8. UE5 References

- **Behavior Trees / Blackboard** — `BT_TemporarySquadMember`, all new BT tasks: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see AI → Behavior Trees, Blackboard)
- **BT Tasks/Services/Decorators** — `Decorator_CommandEquals`, `BTTask_HoldPosition`/`FollowOwner`/`MoveToPoint`/`BreachRoom`/`CoverRetreat`: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see AI → Behavior Trees → Task/Service/Decorator Nodes)
- **AIController** — `ASquadMemberAIController`: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see AI → AI Controllers)
- **EQS** — reused `EQS_FindCoverNearTarget` with a modified query context for `CoverRetreat`: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see AI → Environment Query System)
- **UMG** — `UW_RadialCommandMenu`: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see User Interface → UMG UI Designer)
- **Enhanced Input** — the held-input-to-open-radial-menu binding: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Input → Enhanced Input)

---

## 9. Week-by-Week Tasks

**Week 7 (Aug 24 – Aug 30)** — the entire system is scoped to this one week by design
- Day 1–2: `ATemporarySquadMember`, `ASquadMemberAIController`, `ESquadMemberCommand`, Blackboard setup, `Server_OutfitFromPreset` integration with File 06.
- Day 2–3: `ACallInManager` integration — `TemporarySquadMember` call-in type, one-per-team validation (Section 4.2), squad-vote requirement (already built in File 05, this is just wiring the new call-in type through it).
- Day 3–5: `BT_TemporarySquadMember` — build `HoldPosition`, `Follow`, `SuppressTarget` (reusing File 02 nodes) first, since these are the cheapest and highest-value; then `MoveTo`; then, time permitting, `BreachRoom` and `CoverRetreat` per the Section 7 priority order.
- Day 5: `UW_RadialCommandMenu` first pass (immediate-dispatch commands only: `HoldPosition`, `Follow`, `CoverRetreat`).
- Day 6–7 (spills into Week 8 buffer if needed): targeting sub-mode for `SuppressTarget`/`MoveTo`, deployment timer (Section 4.1), death/gear-loss handling (Section 5).

**Week 8 (Aug 31 – Sep 6)**
- Integration-only — no new features, only bug-fixing and the scope-reduction decisions from Section 7 if needed, based on how much of the above landed in Week 7.

---

## 10. Acceptance Criteria

- [ ] Squad member spawns outfitted with the exact weapons/inventory of the preset the purchasing player selected at call-in time.
- [ ] At minimum 4 of the 6 designed commands (`HoldPosition`, `Follow`, `SuppressTarget`, `MoveTo` — the floor per Section 7) are functional and produce visibly distinct behavior from each other.
- [ ] The squad member continues executing its last-issued command indefinitely until a new command is issued — verify by issuing `HoldPosition` and waiting several minutes without further input.
- [ ] Only the commanding (purchasing) player's `Server_IssueCommand` RPCs are accepted — verify a different player's attempt to command someone else's squad member is rejected server-side.
- [ ] Attempting to purchase a second temporary squad member while one is already active and alive is rejected by `ACallInManager::ValidateRequest`.
- [ ] Squad member death clears `AShooterGameState::ActiveSquadMember` (allowing a new purchase) and does not attempt any gear-return logic.
- [ ] Deployment timer expiry despawns the squad member cleanly without triggering the death/gear-loss path.
- [ ] The commanding player receives a clear, legible notification on squad member death.

---

## 11. Estimated Effort

| Task Cluster | Estimate |
|---|---|
| `ATemporarySquadMember` + `ASquadMemberAIController` + Blackboard/command types | 2.5 dev-days |
| Call-in integration (spawn, one-per-team, vote requirement wiring) | 1.5 dev-days |
| BT: `HoldPosition`, `Follow`, `SuppressTarget` (reusing File 02 nodes) | 2 dev-days |
| BT: `MoveTo` | 1 dev-day |
| BT: `BreachRoom`, `CoverRetreat` (cut-first candidates per Section 7) | 2.5 dev-days |
| `UW_RadialCommandMenu` + targeting sub-modes | 2.5 dev-days |
| Deployment timer + death/gear-loss handling | 1.5 dev-days |
| **Total** | **13.5 dev-days** (scoped entirely within Week 7, with Week 8 buffer as overflow — if the full 13.5 days doesn't fit in Week 7's ~5 working days, Section 7's cut order applies immediately rather than bleeding into Week 8's integration testing time) |
