# 08 — Multiplayer and Networking

## Dependencies

- This file is **cross-cutting** — it does not sit downstream of Files 01–07, it sits *alongside* all of them. Every other file explicitly references this file for its authority model and replication pattern; read this file's Sections 1–3 before implementing any class from Files 01–07, not after.
- Extends the replication pattern already mandated project-wide in `CLAUDE.md` ("Every stateful system uses: `UPROPERTY(ReplicatedUsing=OnRep_X)` + server-only `Server_X()` mutator + `OnRep_X()` re-broadcasting a `BlueprintAssignable` delegate. Never poll replicated arrays/structs directly.") — nothing in this file introduces a new replication idiom, it only says *where* that existing idiom applies across the new systems.

---

## 1. Replication Strategy Per System

| System | Owning Actor/Struct | Replicated Fields | Notes |
|---|---|---|---|
| Mission phase | `AShooterGameState` | `CurrentMissionPhase`, `ActiveWindowEndServerTime` | Always-replicated, `COND_None` — every client needs phase state at all times for UI/HUD |
| Zone state | `AZoneDefinition` (per instance) | `CurrentZoneState` | Always-replicated; low cardinality (2 zones in the demo), negligible bandwidth cost |
| Economy | `AShooterPlayerState` (`PersonalPoints`), `AShooterGameState` (`SquadSharedFund`) | Both always-replicated | Small ints, changes infrequently relative to tick rate — no bandwidth concern |
| Resupply/exfil timers | `AShooterGameState::ActiveWindowEndServerTime`, `AExfilVehicle`'s arrival/departure end-times | Always-replicated, but expressed as **server-time end-timestamps**, never as a raw countdown float | See Section 4 |
| Objective task progress | `UObjectiveTaskComponent::TaskProgress`, `CurrentTaskState` | Always-replicated while the component is relevant | One component per terminal (2 in the demo) — negligible cost; do not throttle |
| Call-in drops | `ACallInManager::PendingDeliveries` | **Not** a replicated `UPROPERTY` — see Section 1.1 | Server-only bookkeeping; clients learn about deliveries via the spawned crate/strike actor's own replication, not by replicating the manager's internal queue |
| Kit presets | `AShooterPlayerState::SavedPresets` | **Not replicated** — on-demand RPC only | Full rationale in File 06 Section 1.3; restated here as the canonical example of "replicate on demand, not always-on" for any future system modeled on this one |
| Temporary squad member | `ATemporarySquadMember` (standard `ACharacter` replication), `AShooterGameState::ActiveSquadMember` | Standard pawn replication (transform, animation) + the single weak-pointer reference | No new replication idiom — reuses the exact movement/animation replication every other `ACharacter` in the project already has |

### 1.1 Why `ACallInManager`'s queue is not replicated

`PendingDeliveries` is server-only planning state — clients don't need to know "there is an internal timer object representing this delivery," they need to know "a crate is now visible in the world" (handled by the crate actor's own spawn + replication, standard for any `AActor`) and, for UI purposes, "a delivery is incoming in approximately N seconds" (handled by a small, purpose-built replicated struct — see below — not by exposing the manager's internal timer bookkeeping). This is the same "replicate the outcome, not the implementation" principle applied elsewhere in this plan (e.g., `UObjectiveTaskComponent` replicates `TaskProgress`, not the internal `FTimerHandle` driving it).

For the "incoming in Xs" indicator referenced in File 05 Section 3.2, add a small dedicated replicated array on `AShooterGameState`:

```cpp
UPROPERTY(Replicated)
TArray<FIncomingDeliveryInfo> IncomingDeliveries; // { FGuid RequestID; double ETAServerTime; FVector TargetLocation; }
```

populated/cleared by `ACallInManager` as requests are scheduled/executed — this is the one small, purpose-built piece of client-visible state the call-in system needs, kept minimal and separate from the manager's full internal queue.

---

## 2. Authority Model

**Rule for this entire plan: every system introduced in Files 01–07 is server-authoritative. No client ever computes gameplay-affecting state locally and trusts it — clients only ever request (via `Server_` RPCs) and display (via replicated properties and `OnRep_` bound delegates).** This is a direct extension of the project-wide rule already stated in CLAUDE.md ("Server functions: Prefixed `Server_` or `Server...` — any function mutating replicated state without this prefix is a bug unless explicitly noted as client-local").

Concrete application per system:

- **Phase transitions** — only `AMissionStateManager::Server_RequestPhaseAdvance` (server-only, called from server-side trigger logic, never from a client RPC directly) can change `CurrentMissionPhase`. No client-side prediction of phase state — a phase transition is inherently a rare, high-stakes event where a one-frame client/server mismatch is imperceptible, so there is no UX cost to waiting for the authoritative value.
- **Zone state** — same pattern, `AZoneDefinition::Server_SetZoneState`, called only from server-side systems (`AWaveSpawnManager`, pursuit-radius checks), never a client RPC.
- **Task progress** — `UObjectiveTaskComponent`'s progress tick runs server-only (Section 1.4 in File 03); clients never locally increment progress even optimistically, since the "stationary players" check itself must be server-verified (a client claiming to be stationary cannot be trusted).
- **Economy** — all point awards and all spending happen server-side. `Server_RequestCallIn` re-validates cost and window state server-side even though the UI (File 05's `UW_ResupplyOverlay`) also disables buttons client-side when funds are insufficient — the client-side disabling is a UX convenience, not a security boundary, per the standard "never trust the client" rule.
- **Extraction** — `AFieldExtractionPoint`'s timer and cancellation-on-damage logic run server-side; the client only sees the replicated per-player extraction state (Section 1) to render a progress UI.
- **Kit presets** — `Server_SavePreset`/`Server_SwapToPreset` validate and persist entirely server-side; a client's local UMG state during preset editing is scratch/draft data discarded on save-request until the server confirms.
- **Temporary squad member commands** — `Server_IssueCommand` validates the sender is `CommandingPlayer` before writing to the Blackboard; there is no client-side BT execution or prediction at all (matches existing `AZombieAIController`/human AI, which are already server-only per standard UE dedicated-server-authoritative AI practice).

**No system in this plan uses client-side prediction.** All of the new gameplay is either turn/event-driven (phase transitions, purchases, commands — rare enough that request→confirm latency is not a felt problem) or continuous-but-tolerant-of-a-frame-of-latency (task progress bars, countdown timers) — none of it is the kind of frame-critical movement/aiming state where prediction is normally justified. This is a deliberate scope decision: prediction adds real complexity (reconciliation, rollback) that would consume schedule better spent elsewhere, and the systems in this plan don't need it to feel responsive.

---

## 3. Session Management

Per GameplayPlan.md: "No save-and-return — sessions are played start-to-finish or abandoned." This significantly simplifies session lifecycle relative to a persistent-world game:

- `AShooterGameGameMode`/`AShooterGameState` are instantiated fresh per session (standard UE level-load behavior); there is no mid-session save/restore of mission state to design for — `EMissionPhase`, zone states, economy values, and in-flight timers all simply reset to their defaults on a new session, matching the existing `UShooterSaveGameSubsystem`'s scope (which persists player loadout/appearance/presets — all *pre-session* configuration — never mission-run state).
- **Abandoned session handling:** if all players disconnect before `PostMission`, the server should tear down/reset the level rather than leaving a mission stuck mid-phase for a hypothetical later rejoin — confirm during Week 1 whether the project already has a "return to lobby on empty server" path in `AShooterGameGameMode`; if not, this plan requires adding one, since several of this plan's systems (timers, spawn managers) should not be left running indefinitely against zero players.

### 3.1 Join-in-progress

The demo's target squad size is 1–4 players (per the Master Brief's Definition of Done referencing "a host and at least one joined client"). Two supported join patterns:

- **Pre-session join** (all players in the lobby before `PreMission` begins) is the primary supported path for the demo and should receive the majority of testing time.
- **Mid-session join-in-progress** (a player connects after `Phase1Infiltration` has already begun) is explicitly a **stretch goal, not required for the demo** (see File 10) — supporting it correctly would require every system in this plan to handle "a fresh `PlayerState` appears after my `OnRep_X` has already fired for everyone else," which is a meaningful correctness burden (e.g., a joining player's client needs `AShooterGameState`'s current values immediately, which standard UE replication already provides on actor relevancy/possession — but the *UI* for every one of Files 01–07's widgets needs to initialize correctly from current state rather than only reacting to change deltas). Flag any UI widget built in this plan that only updates `On*Changed` and never reads the current value on construction as a join-in-progress bug even if it's not a blocking one for the demo.

---

## 4. Replicated Timers

**Rule: no timer in this plan is ever replicated as a raw countdown float.** Every timed window (resupply, extraction vulnerability, exfil vehicle arrival/departure, call-in delivery ETA, doomed-zone escalation) is expressed as an absolute **server-time end-timestamp**, computed once via `GetWorld()->GetGameState()->GetServerWorldTimeSeconds() + Duration` and replicated as a `double`. Clients compute remaining time locally each frame as `EndServerTime - GetServerWorldTimeSeconds()` (client's locally-synced server-time estimate, which UE's engine-level clock sync already maintains) purely for display. This avoids:

- **Clock drift** — a replicated countdown that decrements independently on server and client will visibly desync over a 90–120 second window (exactly File 05's resupply window length) unless constantly re-synced, which is wasteful bandwidth for something the end-timestamp approach avoids entirely.
- **Late-join correctness** — a player joining mid-window immediately gets the correct remaining time for free, since the end-timestamp is a single static value valid from any point in its lifetime — no special "catch up the new client's countdown" logic needed, directly supporting (though not fully solving — see Section 3.1) join-in-progress correctness.

This pattern is used by every timed system in Files 01–07 and should be treated as the one non-negotiable networking convention introduced by this plan — inconsistent adoption (one system using end-timestamps, another using a raw countdown) would be a correctness bug, not a style preference.

---

## 5. Squad Membership

The project's existing multiplayer model (per `AShooterGameGameMode`'s `AlivePlayers` tracking, existing across all connected players with no visible sub-team/squad-partitioning logic) treats all connected players as a single squad — there is no PvP, no multiple competing squads in one session. This plan does not introduce squad-partitioning; "squad" throughout Files 01–07 means "all currently connected players in this session." If a future milestone introduces multiple independent squads sharing a server (unlikely given the co-op, mission-first design), every reference to `AShooterGameState`-level shared state (`SquadSharedFund`, `ActiveSquadMember`, `IncomingDeliveries`) in this plan would need to move to a new per-squad subobject — explicitly flagged here as a scaling concern, not solved by this plan.

### 5.1 Early extraction without ending the session

Covered in detail in File 04 Section 6.3 — restated here as the networking-specific requirement: an extracted or left-behind-then-dead player must be removable from `AShooterGameGameMode::AlivePlayers`-driven match-over accounting without their `APlayerController`/`APlayerState` connection being forcibly closed (they may want to spectate, or the session may support them re-engaging via emergency extraction before fully resolving — see File 04 Section 5). This requires a third player-status category beyond "alive" and "dead" — `EPlayerMissionStatus { Active, Extracted, LeftBehind }` — tracked on `AShooterPlayerState`, checked by `HandleMatchOver` alongside the existing alive-count check (a session ends when all *Active* players are resolved to Extracted/LeftBehind-final, not merely when `AlivePlayers` hits zero, since an extracted player was never "dead" in the sense that flag currently represents).

---

## 6. Networking Risk Areas

| Risk | Why | Mitigation |
|---|---|---|
| **Latency on wave spawns** — a spawn decision made server-side needs to visually appear for all clients close to simultaneously, or players in a squad see enemies "pop in" at different times relative to their own actions | Standard `AActor` spawn replication has inherent latency (one round-trip-ish for relevancy + initial replication); for a fast-paced combat spawn this can read as unfair ("that zombie wasn't there a second ago on my screen") | Spawn enemies with a short server-side "pre-roll" — spawn slightly outside immediate player view/audio range where possible (`AWaveSpawnManager`'s spawn-point selection already biases toward zone-edge points per File 02) so the replication-latency window happens before the enemy is player-relevant, not while a player is already looking at the spawn point |
| **Call-in synchronization** — a fire-support strike's VFX/damage timing must feel synchronized across clients, not staggered | `NetMulticast` RPCs for the impact cue have inherent per-client latency variance | Use the same server-time-end-timestamp pattern as Section 4 for the strike's authored delay countdown (clients all compute "impact happens at ServerTime X," not "impact happens N seconds from when I received this message") — the actual `ApplyRadialDamage` call remains server-only and authoritative regardless of any client-side VFX timing variance, so a few frames of *visual* desync never causes a *gameplay* desync |
| **Task progress replication cadence** — File 03's 4Hz server tick is deliberately low-rate; confirm this doesn't read as choppy on a UMG progress bar under real network latency (as opposed to the zero-latency PIE-on-one-machine testing likely to happen first) | Solo-dev testing environment (single machine, multiple PIE clients) has near-zero latency and can mask a choppiness problem that only appears over a real connection | Schedule at least one Week 8 test session using two physically separate machines/network connections (not just multi-client PIE) specifically to catch this class of issue — PIE-only testing is explicitly insufficient for validating this |
| **Squad-vote RPC race conditions** — two players both requesting a High-tier call-in near-simultaneously, or a vote resolving right as the resupply window expires | Could produce a double-spend against `SquadSharedFund` or a vote resolving into a phase that's no longer the resupply window | `ACallInManager::ValidateRequest`'s checks (File 05 Section 3.3) must be the *final* authority evaluated atomically at the moment of spend-deduction, not just at vote-initiation — re-check phase and funds again right before deducting, not only when the vote starts |

---

## 7. UE5 References

- **Actor Replication** — the foundational pattern for every `UPROPERTY(ReplicatedUsing=...)` in this plan: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Networking and Multiplayer → Actor Replication)
- **GameMode / GameState / PlayerState architecture** — `AShooterGameState` (new), extending `AShooterGameGameMode` and `AShooterPlayerState`: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Gameplay Framework → Game Mode and Game State, Player State)
- **Online Subsystem** — session creation/join, relevant to Section 3's join-in-progress discussion; the project already has `UShooterGameOnlineSubsystem` (confirmed present in `Framework/Subsystems/`) — this plan does not replace it, only documents how new systems interact with session lifecycle: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Networking and Multiplayer → Online Subsystem)
- **RPCs (Server / Client / NetMulticast, Reliable / Unreliable)** — every `Server_X()` function across Files 01–07: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Networking and Multiplayer → RPCs)
- **`GetServerWorldTimeSeconds()`** — the replicated-timer pattern in Section 4: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation (see Networking and Multiplayer → Actor Replication → Time and Clock Synchronization)

---

## 8. Week-by-Week Tasks

This file has no dedicated implementation week of its own — its content is a set of standing rules applied *during* every other file's implementation weeks. The tasks below are the points at which this file's rules require active verification, not new code of their own.

**Week 1 (Jul 13 – Jul 19)**
- Confirm current join-in-progress and abandoned-session behavior in `AShooterGameGameMode` (Section 3) before any new system is built on top of it.
- Establish the server-time-end-timestamp helper pattern (Section 4) as a small shared utility (e.g. a `static` helper on `AShooterGameState` or a free function) so every subsequent file uses the identical pattern rather than each reinventing it slightly differently.

**Week 2–7 (ongoing)**
- Every file's implementation must be reviewed against Section 2's authority model before being marked complete in that file's own week-by-week plan — this is a standing cross-check, not a separate task block.

**Week 8 (Aug 31 – Sep 6)**
- Dedicated two-client (minimum), physically-separate-network test pass (not PIE-only) covering: phase transition sync, resupply window countdown accuracy, call-in delivery timing, task progress smoothness under real latency (Section 6), and the extracted/left-behind player-status handling in Section 5.1.
- Audit every `Server_` function introduced across Files 01–07 for missing server-side re-validation (the squad-vote race condition class of bug in Section 6).

---

## 9. Acceptance Criteria

- [ ] Every piece of gameplay-affecting state introduced in Files 01–07 is mutated only via a `Server_`-prefixed function, verified by a code-level audit (grep for direct mutation of any `ReplicatedUsing` property outside its `Server_X`/`OnRep_X` pair).
- [ ] No timer in the entire plan is replicated as a raw countdown float — verified by the same audit, checking every new `FTimerHandle`-adjacent replicated field is paired with an end-server-time value, not a duration.
- [ ] A player joining before `PreMission` sees fully correct, synchronized state for all systems from their first frame.
- [ ] An extracted player does not trigger `HandleMatchOver` while other players remain active, and does not appear as "dead" in any UI.
- [ ] Two clients on physically separate networks (not just multi-client PIE) complete a full test session with no observed phase/economy desync.
- [ ] A deliberately-induced race (two players requesting a High-tier call-in within the same network frame) does not double-spend `SquadSharedFund`.

---

## 10. Estimated Effort

| Task Cluster | Estimate |
|---|---|
| Session lifecycle audit + abandoned-session handling (if missing) | 1.5 dev-days |
| Server-time-end-timestamp shared utility | 0.5 dev-days |
| `EPlayerMissionStatus` + `HandleMatchOver` extension (File 04/08 shared work) | 1.5 dev-days |
| `IncomingDeliveries` replicated struct + wiring | 1 dev-day |
| Week 8 dedicated cross-network test pass + audit | 2.5 dev-days |
| Race-condition fixes surfaced by the audit (buffer estimate) | 2 dev-days |
| **Total** | **9 dev-days** (distributed across Week 1 and Week 8, plus ongoing review overhead absorbed into each other file's own estimates) |
