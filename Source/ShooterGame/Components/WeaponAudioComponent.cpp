// Source/ShooterGame/Components/WeaponAudioComponent.cpp

#include "WeaponAudioComponent.h"
#include "ShooterGame/Items/Weapon/Weapon.h"
#include "ShooterGame/Components/ShooterSoundFunctionLibrary.h"

UWeaponAudioComponent::UWeaponAudioComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UWeaponAudioComponent::BeginPlay()
{
    Super::BeginPlay();
    InitialiseAllSounds();
}

void UWeaponAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UWeaponAudioComponent::InitialiseAllSounds()
{
    ACs_Start                    = CreateAudioComps(Cues_OneShot);
    ACs_Loop                     = CreateAudioComps(Cues_Loop);
    ACs_Tail                     = CreateAudioComps(Cues_Tail);

    ACs_Start_Suppressed         = CreateAudioComps(Cues_OneShot_Suppressed);
    ACs_Loop_Suppressed          = CreateAudioComps(Cues_Loop_Suppressed);
    ACs_Tail_Suppressed          = CreateAudioComps(Cues_Tail_Suppressed);

    ACs_SwitchFireMode           = CreateAudioComps(Cues_SwitchFireMode);
    ACs_EquipUnequipSuppressor   = CreateAudioComps(Cues_EquipUnequipSuppressor);
    ACs_DryFire                  = CreateAudioComps(Cues_DryFire);
    ACs_ReloadRifle              = CreateAudioComps(Cues_ReloadRifle);
    ACs_ReloadPistol             = CreateAudioComps(Cues_ReloadPistol);
    ACs_ReloadBulletDrop         = CreateAudioComps(Cues_ReloadBulletDrop);
}

UAudioComponent* UWeaponAudioComponent::CreateAudioComp(USoundCue* SoundCue)
{
    if (!SoundCue || !GetOwner() || !GetOwner()->GetRootComponent()) return nullptr;

    UAudioComponent* AC = NewObject<UAudioComponent>(GetOwner());
    if (!AC) return nullptr;

    AC->SetSound(SoundCue);
    AC->bAutoActivate = false;
    AC->bAutoDestroy  = false;
    AC->RegisterComponent();
    AC->AttachToComponent(
        GetOwner()->GetRootComponent(),
        FAttachmentTransformRules::SnapToTargetNotIncludingScale
    );

    return AC;
}

TArray<UAudioComponent*> UWeaponAudioComponent::CreateAudioComps(const TArray<USoundCue*>& SoundCues)
{
    TArray<UAudioComponent*> Result;
    for (USoundCue* Cue : SoundCues)
    {
        if (UAudioComponent* AC = CreateAudioComp(Cue))
        {
            Result.Add(AC);
        }
    }
    return Result;
}

bool UWeaponAudioComponent::IsSuppressed() const
{
    if (const AWeapon* Weapon = Cast<AWeapon>(GetOwner()))
    {
        return Weapon->HasSuppressor();
    }
    return false;
}

void UWeaponAudioComponent::PlayAudioComponentArray(TArray<UAudioComponent*>& AudioComps, float StartTime)
{
    for (UAudioComponent* AC : AudioComps)
    {
        if (!AC) continue;
        AC->Stop();
        AC->Play(StartTime);
    }
}

void UWeaponAudioComponent::FadeOutAudioComponentArray(TArray<UAudioComponent*>& AudioComps, float InFadeOutDuration)
{
    for (UAudioComponent* AC : AudioComps)
    {
        if (!AC) continue;
        UShooterSoundFunctionLibrary::FadeOut(AC, InFadeOutDuration, 0.f);
    }
}

void UWeaponAudioComponent::PlayCueArrayAtOwnerLocation(const TArray<USoundCue*>& Cues)
{
    if (!GetOwner()) return;

    for (USoundCue* Cue : Cues)
    {
        if (!Cue) continue;
        UShooterSoundFunctionLibrary::PlaySoundAtLocation(
            this,
            Cue,
            GetOwner()->GetActorLocation()
        );
    }
}

void UWeaponAudioComponent::Internal_PlayOneShot(bool bSuppressed)
{
    TArray<UAudioComponent*>& StartACs = bSuppressed ? ACs_Start_Suppressed : ACs_Start;
    PlayAudioComponentArray(StartACs, OffsetOneShotTime);

    // Stop cue — mechanical action/tail of the shot, fire-and-forget at world location.
    const TArray<USoundCue*>& StopCues = bSuppressed ? Cues_Stop_Suppressed : Cues_Stop;
    PlayCueArrayAtOwnerLocation(StopCues);

    // Tail cue — distant echo/reverb layer, fire-and-forget at world location.
    const TArray<USoundCue*>& TailCues = bSuppressed ? Cues_Tail_Suppressed : Cues_Tail;
    PlayCueArrayAtOwnerLocation(TailCues);
}

void UWeaponAudioComponent::Internal_PlayOneShotLoop(bool bSuppressed, float InCrossfadeTime)
{
    // Guard — if loop is already running, play only the one-shot transient layer
    // and leave the loop untouched. This prevents restart stutter on every fire tick.
    if (bIsLoopPlaying)
    {
        Internal_PlayOneShot(bSuppressed);
        return;
    }

    // First call — play the one-shot transient then start the loop
    Internal_PlayOneShot(bSuppressed);

    TArray<UAudioComponent*>& LoopACs = bSuppressed ? ACs_Loop_Suppressed : ACs_Loop;

    for (int32 i = 0; i < LoopACs.Num(); ++i)
    {
        UAudioComponent* AC = LoopACs[i];
        if (!AC) continue;

        float LoopOffset = InCrossfadeTime;
        if (OffsetLoopTimes.IsValidIndex(i))
        {
            LoopOffset += OffsetLoopTimes[i];
        }

        AC->Stop();
        AC->Play(LoopOffset);
    }

    bIsLoopPlaying = true;
}

void UWeaponAudioComponent::Internal_StopLoop(bool bSuppressed, float InFadeOutDuration)
{
    if (!bIsLoopPlaying) return;

    TArray<UAudioComponent*>& LoopACs = bSuppressed ? ACs_Loop_Suppressed : ACs_Loop;
    FadeOutAudioComponentArray(LoopACs, InFadeOutDuration);

    // Stop and tail cues are fire-and-forget transient sounds —
    // spawn at world location so they outlive the loop AudioComponent lifecycle.
    if (bSuppressed)
    {
        PlayCueArrayAtOwnerLocation(Cues_Stop_Suppressed);
        PlayCueArrayAtOwnerLocation(Cues_Tail_Suppressed);
    }
    else
    {
        PlayCueArrayAtOwnerLocation(Cues_Stop);
        PlayCueArrayAtOwnerLocation(Cues_Tail);
    }

    bIsLoopPlaying = false;
}

void UWeaponAudioComponent::Internal_PlayDryFire()
{
    PlayAudioComponentArray(ACs_DryFire);
}

void UWeaponAudioComponent::Internal_PlayReload(bool bIsPistolClass)
{
    if (bIsPistolClass)
    {
        PlayAudioComponentArray(ACs_ReloadPistol);
    }
    else
    {
        PlayAudioComponentArray(ACs_ReloadRifle);
    }
}

void UWeaponAudioComponent::Internal_PlayReloadBulletDrop()
{
    PlayAudioComponentArray(ACs_ReloadBulletDrop);
}

void UWeaponAudioComponent::Internal_PlaySwitchFireMode()
{
    PlayAudioComponentArray(ACs_SwitchFireMode);
}

void UWeaponAudioComponent::Internal_PlayEquipUnequipSuppressor()
{
    PlayAudioComponentArray(ACs_EquipUnequipSuppressor);
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

void UWeaponAudioComponent::PlayGunshot_ForMultiplayer()
{
    if (!GetOwner()) return;
    const bool bSuppressed = IsSuppressed();

    if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
    {
        // Play locally immediately — zero latency for the owning client.
        Internal_PlayOneShot(bSuppressed);
        // Send to server so it can multicast to all other clients.
        Server_PlayGunshot(bSuppressed);
    }
    else if (GetOwner()->HasAuthority())
    {
        // Listen server host — multicast directly.
        Multi_PlayGunshot(bSuppressed);
    }
}

void UWeaponAudioComponent::PlayGunshotLoop_ForMultiplayer(float InCrossfadeTime)
{
    if (!GetOwner()) return;
    const bool bSuppressed = IsSuppressed();
    const float FinalCrossfadeTime = InCrossfadeTime >= 0.f ? InCrossfadeTime : CrossfadeOneShotLoopTime;

    if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
    {
        // Play locally immediately — zero latency for the owning client.
        Internal_PlayOneShotLoop(bSuppressed, FinalCrossfadeTime);
        // Send to server to multicast to all other clients.
        Server_PlayGunshotLoop(bSuppressed, FinalCrossfadeTime);
    }
    else if (GetOwner()->HasAuthority())
    {
        Multi_PlayGunshotLoop(bSuppressed, FinalCrossfadeTime);
    }
}

void UWeaponAudioComponent::StopLoop_ForMultiplayer(float InFadeOutDuration)
{
    if (!GetOwner()) return;
    const bool bSuppressed = IsSuppressed();
    const float FinalFadeOutDuration = InFadeOutDuration >= 0.f ? InFadeOutDuration : FadeOutDuration;

    if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
    {
        // Stop locally immediately — owner hears burst end without RTT delay.
        Internal_StopLoop(bSuppressed, FinalFadeOutDuration);
        // Tell server to stop on all other clients.
        Server_StopLoop(bSuppressed, FinalFadeOutDuration);
    }
    else if (GetOwner()->HasAuthority())
    {
        Multi_StopLoop(bSuppressed, FinalFadeOutDuration);
    }
}

void UWeaponAudioComponent::PlayDryFire_ForMultiplayer()
{
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
    {
        // Play locally immediately — zero latency for the owning client.
        Internal_PlayDryFire();
        // Send to server so it can multicast to all other clients.
        Server_PlayDryFire();
    }
    else if (GetOwner() && GetOwner()->HasAuthority())
    {
        Multi_PlayDryFire();
    }
}

void UWeaponAudioComponent::PlayReload_ForMultiplayer(bool bIsPistolClass)
{
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
    {
        // Play locally immediately — zero latency for the owning client.
        Internal_PlayReload(bIsPistolClass);
        // Send to server so it can multicast to all other clients.
        Server_PlayReload(bIsPistolClass);
    }
    else if (GetOwner() && GetOwner()->HasAuthority())
    {
        Multi_PlayReload(bIsPistolClass);
    }
}

void UWeaponAudioComponent::PlayReloadBulletDrop_ForMultiplayer()
{
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
    {
        // Play locally immediately — zero latency for the owning client.
        Internal_PlayReloadBulletDrop();
        // Send to server so it can multicast to all other clients.
        Server_PlayReloadBulletDrop();
    }
    else if (GetOwner() && GetOwner()->HasAuthority())
    {
        Multi_PlayReloadBulletDrop();
    }
}

void UWeaponAudioComponent::PlaySwitchFireMode_ForMultiplayer()
{
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
    {
        // Play locally immediately — zero latency for the owning client.
        Internal_PlaySwitchFireMode();
        // Send to server so it can multicast to all other clients.
        Server_PlaySwitchFireMode();
    }
    else if (GetOwner() && GetOwner()->HasAuthority())
    {
        Multi_PlaySwitchFireMode();
    }
}

void UWeaponAudioComponent::PlayEquipUnequipSuppressor_ForMultiplayer()
{
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
    {
        // Play locally immediately — zero latency for the owning client.
        Internal_PlayEquipUnequipSuppressor();
        // Send to server so it can multicast to all other clients.
        Server_PlayEquipUnequipSuppressor();
    }
    else if (GetOwner() && GetOwner()->HasAuthority())
    {
        Multi_PlayEquipUnequipSuppressor();
    }
}

// -----------------------------------------------------------------------
// Server RPCs
// -----------------------------------------------------------------------

void UWeaponAudioComponent::Server_PlayGunshot_Implementation(bool bSuppressed)
{
    Multi_PlayGunshot(bSuppressed);
}
bool UWeaponAudioComponent::Server_PlayGunshot_Validate(bool bSuppressed)
{
    return true;
}

void UWeaponAudioComponent::Server_PlayGunshotLoop_Implementation(bool bSuppressed, float InCrossfadeTime)
{
    Multi_PlayGunshotLoop(bSuppressed, InCrossfadeTime);
}
bool UWeaponAudioComponent::Server_PlayGunshotLoop_Validate(bool bSuppressed, float InCrossfadeTime)
{
    return true;
}

void UWeaponAudioComponent::Server_StopLoop_Implementation(bool bSuppressed, float InFadeOutDuration)
{
    Multi_StopLoop(bSuppressed, InFadeOutDuration);
}
bool UWeaponAudioComponent::Server_StopLoop_Validate(bool bSuppressed, float InFadeOutDuration)
{
    return true;
}

void UWeaponAudioComponent::Server_PlayDryFire_Implementation()
{
    Multi_PlayDryFire();
}
bool UWeaponAudioComponent::Server_PlayDryFire_Validate()
{
    return true;
}

void UWeaponAudioComponent::Server_PlayReload_Implementation(bool bIsPistolClass)
{
    Multi_PlayReload(bIsPistolClass);
}
bool UWeaponAudioComponent::Server_PlayReload_Validate(bool bIsPistolClass)
{
    return true;
}

void UWeaponAudioComponent::Server_PlayReloadBulletDrop_Implementation()
{
    Multi_PlayReloadBulletDrop();
}
bool UWeaponAudioComponent::Server_PlayReloadBulletDrop_Validate()
{
    return true;
}

void UWeaponAudioComponent::Server_PlaySwitchFireMode_Implementation()
{
    Multi_PlaySwitchFireMode();
}
bool UWeaponAudioComponent::Server_PlaySwitchFireMode_Validate()
{
    return true;
}

void UWeaponAudioComponent::Server_PlayEquipUnequipSuppressor_Implementation()
{
    Multi_PlayEquipUnequipSuppressor();
}
bool UWeaponAudioComponent::Server_PlayEquipUnequipSuppressor_Validate()
{
    return true;
}

// -----------------------------------------------------------------------
// Multicast RPCs
// -----------------------------------------------------------------------

void UWeaponAudioComponent::Multi_PlayGunshot_Implementation(bool bSuppressed)
{
    // Skip owning client — they already played locally in ForMultiplayer.
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy) return;
    Internal_PlayOneShot(bSuppressed);
}
bool UWeaponAudioComponent::Multi_PlayGunshot_Validate(bool bSuppressed)
{
    return true;
}

void UWeaponAudioComponent::Multi_PlayGunshotLoop_Implementation(bool bSuppressed, float InCrossfadeTime)
{
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy) return;
    Internal_PlayOneShotLoop(bSuppressed, InCrossfadeTime);
}
bool UWeaponAudioComponent::Multi_PlayGunshotLoop_Validate(bool bSuppressed, float InCrossfadeTime)
{
    return true;
}

void UWeaponAudioComponent::Multi_StopLoop_Implementation(bool bSuppressed, float InFadeOutDuration)
{
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy) return;
    Internal_StopLoop(bSuppressed, InFadeOutDuration);
}
bool UWeaponAudioComponent::Multi_StopLoop_Validate(bool bSuppressed, float InFadeOutDuration)
{
    return true;
}

void UWeaponAudioComponent::Multi_PlayDryFire_Implementation()
{
    // Skip owning client — they already played locally in ForMultiplayer.
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy) return;
    Internal_PlayDryFire();
}
bool UWeaponAudioComponent::Multi_PlayDryFire_Validate()
{
    return true;
}

void UWeaponAudioComponent::Multi_PlayReload_Implementation(bool bIsPistolClass)
{
    UE_LOG(LogTemp, Warning, TEXT("Multi_PlayReload_Implementation — Role: %d"), (int32)GetOwnerRole());
    // Skip owning client — they already played locally in ForMultiplayer.
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy) return;
    Internal_PlayReload(bIsPistolClass);
}
bool UWeaponAudioComponent::Multi_PlayReload_Validate(bool bIsPistolClass)
{
    return true;
}

void UWeaponAudioComponent::Multi_PlayReloadBulletDrop_Implementation()
{
    // Skip owning client — they already played locally in ForMultiplayer.
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy) return;
    Internal_PlayReloadBulletDrop();
}
bool UWeaponAudioComponent::Multi_PlayReloadBulletDrop_Validate()
{
    return true;
}

void UWeaponAudioComponent::Multi_PlaySwitchFireMode_Implementation()
{
    // Skip owning client — they already played locally in ForMultiplayer.
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy) return;
    Internal_PlaySwitchFireMode();
}
bool UWeaponAudioComponent::Multi_PlaySwitchFireMode_Validate()
{
    return true;
}

void UWeaponAudioComponent::Multi_PlayEquipUnequipSuppressor_Implementation()
{
    // Skip owning client — they already played locally in ForMultiplayer.
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy) return;
    Internal_PlayEquipUnequipSuppressor();
}
bool UWeaponAudioComponent::Multi_PlayEquipUnequipSuppressor_Validate()
{
    return true;
}