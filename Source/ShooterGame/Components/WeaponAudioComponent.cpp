// Source/ShooterGame/Components/WeaponAudioComponent.cpp

#include "WeaponAudioComponent.h"
#include "ShooterGame/Items/Weapon/Weapon.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UWeaponAudioComponent::UWeaponAudioComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Must be replicated so Server RPCs can be called from owning client
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

// -----------------------------------------------------------------------
// Initialisation — pre-create all audio components at BeginPlay
// so there's zero allocation cost when the weapon fires
// -----------------------------------------------------------------------

void UWeaponAudioComponent::InitialiseAllSounds()
{
    ACs_Start            = CreateAudioComps(Cues_OneShot);
    ACs_Loop             = CreateAudioComps(Cues_Loop);
    ACs_Start_Suppressed = CreateAudioComps(Cues_OneShot_Suppressed);
    ACs_Loop_Suppressed  = CreateAudioComps(Cues_Loop_Suppressed);
}

UAudioComponent* UWeaponAudioComponent::CreateAudioComp(USoundCue* SoundCue)
{
    if (!SoundCue || !GetOwner()) return nullptr;

    UAudioComponent* AC = NewObject<UAudioComponent>(GetOwner());
    AC->SetSound(SoundCue);
    AC->bAutoActivate = false;
    AC->bAutoDestroy  = false;

    // Attach to the weapon mesh root so positional audio follows the weapon
    AC->RegisterComponent();
    AC->AttachToComponent(GetOwner()->GetRootComponent(),
        FAttachmentTransformRules::SnapToTargetNotIncludingScale);

    return AC;
}

TArray<UAudioComponent*> UWeaponAudioComponent::CreateAudioComps(
    const TArray<USoundCue*>& SoundCues)
{
    TArray<UAudioComponent*> Result;
    for (USoundCue* Cue : SoundCues)
    {
        UAudioComponent* AC = CreateAudioComp(Cue);
        if (AC) Result.Add(AC);
    }
    return Result;
}

// -----------------------------------------------------------------------
// Suppressor query
// -----------------------------------------------------------------------

bool UWeaponAudioComponent::IsSuppressed() const
{
    if (AWeapon* Weapon = Cast<AWeapon>(GetOwner()))
    {
        return Weapon->HasSuppressor();
    }
    return false;
}

// -----------------------------------------------------------------------
// Internal play functions — run on every machine via NetMulticast
// -----------------------------------------------------------------------

void UWeaponAudioComponent::Internal_PlayOneShot(bool bSuppressed)
{
    TArray<UAudioComponent*>& StartACs = bSuppressed ? ACs_Start_Suppressed : ACs_Start;

    for (UAudioComponent* AC : StartACs)
    {
        if (AC)
        {
            AC->Stop();
            AC->Play(OffsetOneShotTime);
        }
    }
}

void UWeaponAudioComponent::Internal_PlayOneShotLoop(bool bSuppressed, float CrossfadeTime)
{
    // Play the one-shot layer immediately
    Internal_PlayOneShot(bSuppressed);

    // Start the loop layer after the crossfade offset
    TArray<UAudioComponent*>& LoopACs = bSuppressed ? ACs_Loop_Suppressed : ACs_Loop;

    for (int32 i = 0; i < LoopACs.Num(); ++i)
    {
        UAudioComponent* AC = LoopACs[i];
        if (!AC) continue;

        float LoopOffset = CrossfadeTime;
        if (OffsetLoopTimes.IsValidIndex(i))
        {
            LoopOffset += OffsetLoopTimes[i];
        }

        AC->Stop();
        AC->Play(LoopOffset);
    }

    bIsLoopPlaying = true;
}

void UWeaponAudioComponent::Internal_StopLoop(bool bSuppressed, float FadeOutDuration)
{
    if (!bIsLoopPlaying) return;

    // Fade out the loop layer
    TArray<UAudioComponent*>& LoopACs = bSuppressed ? ACs_Loop_Suppressed : ACs_Loop;
    for (UAudioComponent* AC : LoopACs)
    {
        if (AC) AC->FadeOut(FadeOutDuration, 0.f);
    }

    // Play stop cue
    const TArray<USoundCue*>& StopCues = bSuppressed ? Cues_Stop_Suppressed : Cues_Stop;
    for (USoundCue* Cue : StopCues)
    {
        if (Cue)
        {
            UGameplayStatics::PlaySoundAtLocation(this, Cue,
                GetOwner()->GetActorLocation());
        }
    }

    // Play tail cue (unsuppressed only — suppressed weapons have no tail)
    if (!bSuppressed)
    {
        for (USoundCue* Cue : Cues_Tail)
        {
            if (Cue)
            {
                UGameplayStatics::PlaySoundAtLocation(this, Cue,
                    GetOwner()->GetActorLocation());
            }
        }
    }

    bIsLoopPlaying = false;
}

// -----------------------------------------------------------------------
// Public API — entry points called from AWeapon::Fire()
// -----------------------------------------------------------------------

void UWeaponAudioComponent::PlayGunshot_ForMultiplayer()
{
    const bool bSuppressed = IsSuppressed();

    // Owning client calls Server RPC — server then multicasts to all
    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
    {
        Server_PlayGunshot(bSuppressed);
    }
    else if (GetOwner() && GetOwner()->HasAuthority())
    {
        // Called directly on server (e.g., AI-controlled weapon)
        Multi_PlayGunshot(bSuppressed);
    }
}

void UWeaponAudioComponent::PlayGunshotLoop_ForMultiplayer(float CrossfadeTime)
{
    const bool bSuppressed = IsSuppressed();

    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
    {
        Server_PlayGunshotLoop(bSuppressed, CrossfadeTime);
    }
    else if (GetOwner() && GetOwner()->HasAuthority())
    {
        Multi_PlayGunshotLoop(bSuppressed, CrossfadeTime);
    }
}

void UWeaponAudioComponent::StopLoop_ForMultiplayer(float FadeOutDuration)
{
    const bool bSuppressed = IsSuppressed();

    if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
    {
        Server_StopLoop(bSuppressed, FadeOutDuration);
    }
    else if (GetOwner() && GetOwner()->HasAuthority())
    {
        Multi_StopLoop(bSuppressed, FadeOutDuration);
    }
}

// -----------------------------------------------------------------------
// Server RPCs — validate then forward to Multicast
// -----------------------------------------------------------------------

void UWeaponAudioComponent::Server_PlayGunshot_Implementation(bool bSuppressed)
{
    Multi_PlayGunshot(bSuppressed);
}
bool UWeaponAudioComponent::Server_PlayGunshot_Validate(bool bSuppressed)
{
    return true;
}

void UWeaponAudioComponent::Server_PlayGunshotLoop_Implementation(bool bSuppressed,
    float CrossfadeTime)
{
    Multi_PlayGunshotLoop(bSuppressed, CrossfadeTime);
}
bool UWeaponAudioComponent::Server_PlayGunshotLoop_Validate(bool bSuppressed,
    float CrossfadeTime)
{
    return true;
}

void UWeaponAudioComponent::Server_StopLoop_Implementation(bool bSuppressed,
    float FadeOutDuration)
{
    Multi_StopLoop(bSuppressed, FadeOutDuration);
}
bool UWeaponAudioComponent::Server_StopLoop_Validate(bool bSuppressed,
    float FadeOutDuration)
{
    return true;
}

// -----------------------------------------------------------------------
// NetMulticast RPCs — run on every connected machine
// -----------------------------------------------------------------------

void UWeaponAudioComponent::Multi_PlayGunshot_Implementation(bool bSuppressed)
{
    Internal_PlayOneShot(bSuppressed);
}
bool UWeaponAudioComponent::Multi_PlayGunshot_Validate(bool bSuppressed)
{
    return true;
}

void UWeaponAudioComponent::Multi_PlayGunshotLoop_Implementation(bool bSuppressed,
    float CrossfadeTime)
{
    Internal_PlayOneShotLoop(bSuppressed, CrossfadeTime);
}
bool UWeaponAudioComponent::Multi_PlayGunshotLoop_Validate(bool bSuppressed,
    float CrossfadeTime)
{
    return true;
}

void UWeaponAudioComponent::Multi_StopLoop_Implementation(bool bSuppressed,
    float FadeOutDuration)
{
    Internal_StopLoop(bSuppressed, FadeOutDuration);
}
bool UWeaponAudioComponent::Multi_StopLoop_Validate(bool bSuppressed,
    float FadeOutDuration)
{
    return true;
}