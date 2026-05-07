// Source/ShooterGame/Types/ShooterGameplayTags.cpp
#include "ShooterGameplayTags.h"

// ============================================================================
// UE_DEFINE_GAMEPLAY_TAG must be at file scope — cannot be inside a namespace.
// Each definition registers the tag with UGameplayTagsManager at module startup.
// The string must exactly match an entry in Config/DefaultGameplayTags.ini.
// A mismatch will assert on startup in Development builds.
// ============================================================================

UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_Backpack,     "Slot.Backpack");
UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_Rig,          "Slot.Rig");
UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_Belt,         "Slot.Belt");
UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_WeaponMod,    "Slot.WeaponMod");
UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_Holster,      "Slot.Holster");
UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_MedPouch,     "Slot.MedPouch");
UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_MissionCache, "Slot.MissionCache");
UE_DEFINE_GAMEPLAY_TAG(TAG_Slot_Stash,        "Slot.Stash");