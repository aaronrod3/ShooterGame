# GameplayPlan — ShooterGame Wave Mission Mode

> Design document for Wave Mission Mode. This is the authoritative reference for mission structure, economy, extraction, and AI systems. Read this before implementing or changing any mission-layer system.

---

## Overview

Wave Mission Mode is a mission-first, phased infiltration experience — not endless survival. Players deploy with a defined objective, fight through escalating enemy pressure across four sequential phases, and either complete the mission for full rewards or extract early for partial rewards. **The session ends — it is not infinite.**

### Core Pillars

- **Mission clarity over chaos** — Players always have a defined objective and a clear next step
- **Pressure escalation, not just survival** — Enemy waves are a clock, not just an obstacle
- **Planning pays off** — Pre-game loadout and in-mission economy knowledge give real advantage
- **Graceful failure path** — Early extraction always available with partial rewards; no session feels wasted
- **Milsim grounding** — Movement, logistics, and loadout decisions feel consequential

---

## Map Structure

- 3–5 distinct objective zones in a single continuous map (urban, industrial, forest, radioactive blast zone)
- **Transit corridors** between zones: low-density enemy territory for movement and regrouping
- Enemy density is zone-anchored — sparse in transit, dense near objectives
- Hot zones form naturally around objectives as waves progress; the longer the squad stays, the more dangerous it gets
- Map does not play like a sandbox — enemy placement and wave spawns are authored around the phase structure

---

## Pre-Game Loadout

### Kit Preset System

- Players configure up to 6 named presets before the session (island base loadout screen)
- Each preset: weapons + attachments, armor tier, chest rig, magazine loadout, consumables
- At mission start, player selects one active preset
- Role identity (Assaulter, Support, Heavy, Scout) shapes recommended presets and long-term unlocks — not class-locked in early build

### In-Mission Kit Swap

- Available **only during resupply windows**
- Costs currency from `SquadSharedFund`
- Squad can see each other's current and available presets during the window — enables coordination
- Swapping heavier = adapt to pressure; swapping lighter = pivot to speed extraction

---

## Mission Phase Structure

Phases are sequential. Completing a phase advances the mission and opens the next resupply window.

### Phase 1 — Infiltration and Area Clearance

**Objective:** Enter the target area and establish control.

- Players insert by boat or helicopter
- Enemies already present — defended position, not empty
- Squad clears zone to a defined threshold (enemy count, area control, or triggered event)
- Once clearance threshold met → wave sequence begins (escalating reinforcements)
- **Resupply Window 1** opens immediately after zone clearance, before first wave fully arrives (~60–90 sec)

### Phase 2 — Objective Execution Under Pressure

**Objective:** Complete the primary task (e.g., download data from a terminal).

- Task requires one or more players stationary/near-stationary
- Remaining squad defends while task runs
- Enemy waves arrive in increasing intensity; zombies primary, human AI reinforcements in later waves
- Night variants and special infected may arrive based on time-of-day state
- Zone has an **overrun threshold** — if task not completed in time, zone becomes unwinnable

### Phase 3 — Exfiltration Under Pursuit

**Objective:** Leave the first objective zone and reach the second.

- Begins the moment Phase 2 task completes
- First zone enters **doomed state**: endless spawns, no ceiling — there is no winning it, only leaving
- Players must physically travel to transit corridor and clear the pursuit radius
- Once outside pursuit radius, zone despawns and pressure drops to transit density
- **Resupply Window 2** opens in transit corridor — primary restocking opportunity (~90–120 sec)

### Phase 4 — Second Objective and Final Extraction

**Objective:** Upload data via comms tower, then extract before the exfil vehicle departs.

- Mirrors Phase 1–3 structure at the second zone
- Extraction vehicle (helicopter or boat) arrives at a fixed LZ at a fixed time after upload completes
- If squad is not aboard when it departs → **left behind**
- Stranded players: secondary emergency extraction point (lower reward tier) or die → full gear loss
- Completing with primary exfil = full mission rewards

---

## Economy — Points and Call-Ins

### Earning Points

| Source | Value |
|---|---|
| Kills (standard enemies) | Low |
| Assists | Shared partial credit |
| Special enemy kills (night variants, human AI officers) | Elevated |
| Phase 2 task milestone completion | Significant |
| Wave fully cleared (survival bonus) | Per wave |
| Phase completion (1, 2, 4) | Fixed bonus |

Points tracked **per player** (`PersonalPoints`) and **pooled** (`SquadSharedFund`). Vehicles, fire support, and squad reinforcement draw from the shared pool (squad vote or host approval).

### Resupply Windows

- **Window 1:** After Phase 1 clearance — short timer (~60–90 sec)
- **Window 2:** Transit corridor after Phase 3 exfil — longer timer (~90–120 sec)
- No resupply during active phases — players who run dry manage with what they have or find field ammo caches
- Timer expires → window closes → **unspent points are lost**

### Call-In Types

Drops arrive on a schedule — not instant. Items purchased in a resupply window arrive at the **next drop window** (authored intervals during phases).

| Call-In | Cost Tier | Notes |
|---|---|---|
| Ammo resupply drop | Low | Crate lands at marked zone; **draws nearby enemies** |
| Medical supply crate | Low–Medium | Heals + replenishes consumables; despawns after 30 sec |
| Kit swap (per player) | Medium | Switch to stored preset; resupply windows only |
| Mortar strike | Medium | Area denial; 10-sec delay before strike |
| Distraction flare | Low | Diverts zombie pathing ~90 sec |
| Chopper fire support | High | Single strafing run on marked corridor |
| Artillery strike | High | Larger area, longer delay; **not usable near players** |
| Temporary squad member | High | See below |

> **Design principle:** Every call-in has a cost beyond currency. Nothing purchased is a free win.

### Temporary Squad Member

- Single AI-controlled soldier, player-directed
- Player outfits using one of their own stored kit presets
- Commands via radial menu (hold key → select): Hold Position, Follow, Suppress Target, Move To, Breach Room, Cover Retreat
- Follows last issued command until new one given — not fully autonomous
- Deployment duration: ~5–8 min per call-in, or until killed
- On death: gear is lost (same as player death)
- **One active per team at a time**
- **Implement last** — stub early, full pass after Alpha Feature Complete

---

## Session Rules

- No save-and-return — sessions are played start-to-finish or abandoned
- Any player can individually extract at a **field extraction point** at any time
- Extraction sequence: ~30 seconds, player remains vulnerable
- Solo extract does **not** end the session for remaining squadmates

### Early Extraction Reward Tiers

| Extraction Timing | Reward |
|---|---|
| Before Phase 1 completes | Minimal XP only; no loot retained |
| After Phase 1 completes | Partial XP + kills credit |
| After Phase 2 completes | Significant XP + task reward + carried loot |
| After Phase 3 completes | Full XP for Phases 1–3 + carried loot |
| Phase 4 primary exfil | Full mission reward tier |
| Phase 4 emergency exfil | Full XP, reduced currency payout |

Death before reaching an extraction point = **full loadout loss**.

---

## High-Risk Design Areas (Tune Early)

- **Doomed zone timing** — overrun threshold must feel like pressure, not a hard timer; determine enemy density curve through playtesting
- **Resupply window length** — start at 90 sec; too short = players feel cheated, too long = tension dissolves
- **Temporary squad member AI** — keep to 4–6 BT commands first pass; full autonomy comes later
- **Drop window vs. resupply alignment** — drop schedule must be designed relative to phase pacing or call-ins arrive too late
- **Early extraction reward gap** — Phase 2 vs. Phase 4 payout must incentivize full completion without making partial feel worthless

---

## Recommended Implementation Order

1. Authored phase triggers — zone-based spawn sets keyed to phase state
2. Phase 2 task system — stationary interaction with progress bar + enemy pressure response
3. Doomed zone logic — spawn rate multiplier escalating past threshold, no ceiling
4. Field extraction points — timed sequence, phase-based reward calculation
5. Resupply window system — timed UI overlay, locked during active phases
6. Basic call-in economy — points tracking, drop crates (ammo + meds first), scheduled drop windows
7. Phase 4 exfil vehicle — timed departure, secondary emergency extraction fallback
8. Kit preset system — multiple stored presets, swap during resupply with currency cost
9. Fire support call-ins — mortar + chopper strafing, authored delay, enemy attraction
10. Temporary squad member — behavior tree + radial command menu (**last**)
