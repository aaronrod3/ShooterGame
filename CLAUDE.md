ShooterGame
UE5.7 multiplayer zombie shooter — four-phase wave-based missions, server-authoritative, C++/Blueprint hybrid. Solo dev. IDE: JetBrains Rider.

Commands
Build (Editor): Rider toolbar → ShooterGameEditor Win64 Development, or: "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" ShooterGameEditor Win64 Development -project="...\ShooterGame.uproject" -waitmutex

Generate project files: Build.bat -projectfiles -project="...\ShooterGame.uproject" -game -engine

Test: PIE (Play In Editor) — no automated test suite

Source control: Git, branch master — commit after each completed sub-task

After any header change: Close Rider → delete Binaries/ + Intermediate/ → regenerate project files → rebuild

Live Coding (Ctrl+Alt+F11): .cpp only — never after header changes

Architecture
Character (AShooterGameCharacter)
Thin shell composing single-responsibility components. Read Player/Character/ShooterGameCharacter.h for the fastest full-project overview.

UCombatComponent — fire/reload/ADS/recoil state machine, RPCs, equips weapons from loadout

ULoadoutComponent — replicated FLoadoutData; validated via CanEquipInSlot(); populated on login by UShooterSaveGameSubsystem

UInventoryComponent — tag-gated item container (FItemInstance array); contains a legacy "magazine shim" (FMagazine) being phased out — do not extend it

UHitZoneComponent — per-bone damage-multiplier lookup; shared with AZombieCharacter; server-only, no replicated state

UDownedComponent, UReviveComponent, UEquippedStateComponent — bleedout, revive, and carried-gear snapshotting

Replication Pattern
Every stateful system uses: UPROPERTY(ReplicatedUsing=OnRep_X) + server-only Server_X() mutator + OnRep_X() re-broadcasting a BlueprintAssignable delegate. Never poll replicated arrays/structs directly — always bind to the On*Changed delegate.

Item / Loadout / Inventory
UItemDefinition (Inventory/ItemDefinition.h) — UPrimaryDataAsset, one per item/weapon. New weapons = new data asset, no C++ changes required

FItemInstance (Types/ItemTypes.h) — runtime item instance. Always reference by InstanceID GUID, never array index

UWeaponConfig (Items/Weapon/WeaponConfig.h) — mirrors Infima's BP_TFA_BaseConfig field-for-field (meshes, sockets, montages). All asset refs are TSoftObjectPtr/TSoftClassPtr. Gameplay stats (damage, fire rate) live on AWeapon/UCombatComponent, not here

FWeaponPartCondition (per-part durability) is a declared stub — do not wire up without a deliberate design pass

Slot compatibility via GameplayTagContainer: RequiredSlotTag on slot, AcceptedSlotTags on item. Adding a new slot tag requires updating ShooterGameplayTags.h, the matching .cpp, and Config/DefaultGameplayTags.ini — all three, every time

Save / Persistence
UShooterSaveGameSubsystem (Framework/Subsystems/) is the sole disk I/O point. Never call SaveGameToSlot/LoadGameFromSlot anywhere else — always go through this subsystem.

Enemy AI
AZombieCharacter owns health, hit-zone routing, melee — no decision logic

All decisions in AZombieAIController → Blackboard → Behavior Tree (Enemy/BehaviorTree/)

StateTree/GameplayStateTree plugins are enabled project-wide but zombie AI uses classic Behavior Trees — verify before assuming StateTree

Animation
ShooterFPAnimInstance / ShooterTPAnimInstance (Player/Animation/) derive from UShooterAnimInstanceBase

"Phase 1 Infima bridge" comments mark the C++↔AnimGraph contract (locomotion bools, procedural FTransforms for crouch/ADS/recoil, grip-blend alpha). Changing these breaks Blueprint AnimGraphs silently — reconnect in editor before testing

Infima / TFA Integration
This project ports the Infima Games "Toolkit for Advanced FPS/TPS" (TFA) Blueprint template into native C++. Comments referencing "Infima", "BP_TFA_BaseConfig", "M6", "Milestone N", "Phase N" are intent documentation — read before refactoring anything they touch. Demo Blueprints in the Infima folder are reference-only, not gameplay logic.

Mission State Machine
PreMission → Phase1Infiltration → Resupply1 → Phase2Objective → Phase3Exfil → Resupply2 → Phase4SecondObjective → Phase4Exfil → PostMission

Each transition fires a multicast delegate

State machine never calls directly into other systems

Economy
PersonalPoints (per player) + SquadSharedFund (pooled)

All scoring through ScoringSubsystem (server-authoritative)

Resupply window locks IMCGameplay, opens shop, starts countdown — unspent points are lost

Zone Spawning
ZoneDefinition actor: spawn tables per phase, clearance threshold, zone state

States: Neutral → Active → Doomed → Cleared

Doomed zone: no spawn ceiling; rate multiplier escalates every 15s after Phase 3 trigger

Conventions
Naming: Epic standard — AMyActor, UMyComponent, FMyStruct, EMyEnum, IMyInterface

Booleans: Prefix with b (e.g., bIsReloading, bIsDead)

Pointers: TObjectPtr<T> for UPROPERTY declarations — raw T* only in local function logic

Reflection: Every reflected class must have GENERATED_BODY()

UPROPERTY: EditAnywhere, BlueprintReadWrite, Category="X" for variables; VisibleAnywhere, BlueprintReadOnly, Category="Components" for component pointers

UFUNCTION: BlueprintImplementableEvent for BP-implemented; BlueprintNativeEvent for C++ with BP override

Server functions: Prefixed Server_ or Server... — any function mutating replicated state without this prefix is a bug unless explicitly noted as client-local

No magic numbers: All tunable values in DataAssets or UPROPERTY(EditAnywhere)

Damage: Always route through TakeDamage() — never apply directly

No commented-out code in commits — use branches

Module deps: Check .Build.cs before adding heavy modules (GameplayAbilities, AIModule) — confirm first

Off-Limits / DO NOT
Content/**/*.uasset — binary, do not read or parse

Infima demo/common/core logic — being replaced, not extended

Modify SaveGame schema mid-milestone without a migration plan

Edit Data Assets without noting the change in the linked GitHub Issue

Change any IMC binding without updating all three IMC files (IMCGameplay, IMCVehicle, IMCUI)

Change the phase transition delegate signature without updating all subscribers

Ignored Paths — Do Not Index
/.vs/ /Binaries/ /Intermediate/ /Saved/ /DerivedDataCache/

Development Order
Build bottom-up — do not start Layer 2 until Layer 1 is stable.

Layer	Systems
Layer	Systems
1 — Foundation	Player input, health/damage, weapon base
2 — Game Systems	Kit presets, UI/HUD, fire support, squad AI
3 — Mission	State machine, zone spawning, economy, extraction
4 — Balance	Tuning, playtesting, difficulty curves
Current target → Milestone 2: First Playable Wave Loop
Done when: Player inserts → clears Phase 1 → completes Phase 2 download → escapes pursuit → extracts. Win/fail states work.

GitHub Workflow
Issues: github.com/users/aaronrod3/projects/1 | Sprint: 1 week

Labels: p0-critical p1-high p2-medium p3-low blocked needs-design needs-playtest ready-for-test

Log bugs and discoveries as Issues immediately — never hold in chat

Confirm milestone exit condition before closing any feature issue

Reference Docs
@docs/GameplayPlan.md — full design doc

@docs/SystemNotes/ — per-system notes

@docs/infima_integration_plan.md — Infima Pack integration steps

UE5 Docs: https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-documentation