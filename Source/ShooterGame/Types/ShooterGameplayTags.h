// Source/ShooterGame/Types/ShooterGameplayTags.h
#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// ============================================================================
// Shooter Gameplay Tags — Native Slot Tag Declarations
//
// UE_DECLARE_GAMEPLAY_TAG_EXTERN declares each tag as an extern so any file
// that includes this header can reference it by name. The actual definition
// lives in ShooterGameplayTags.cpp using UE_DEFINE_GAMEPLAY_TAG at file scope.
//
// HOW TO USE IN C++:
//   #include "ShooterGame/Types/ShooterGameplayTags.h"
//
//   // Check slot compatibility:
//   if (Item.Definition->AcceptedSlotTags.HasTag(TAG_Slot_Backpack))
//
//   // Pass as RequiredSlotTag:
//   InventoryComp->Server_AddItem(ItemInstance, TAG_Slot_Rig);
//
// HOW TO ADD A NEW TAG:
//   1. UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_NewName) below
//   2. UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_NewName, "Slot.NewName") in the .cpp
//   3. Add the matching entry to Config/DefaultGameplayTags.ini
// ============================================================================

// Slot Tags — used as RequiredSlotTag on containers and AcceptedSlotTags on items

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Backpack);      // General storage
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Rig);           // Tactical chest rig
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Belt);          // Belt pouches
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_WeaponMod);     // Weapon attachment socket
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Holster);       // Pistol holster
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_MedPouch);      // Dedicated medical pouch
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_MissionCache);  // In-mission squad cache / loot containers
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Slot_Stash);         // Player personal stash