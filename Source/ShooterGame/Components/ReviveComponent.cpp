// ReviveComponent.cpp

#include "ReviveComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Components/DownedComponent.h"

UReviveComponent::UReviveComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UReviveComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AShooterGameCharacter>(GetOwner());

	// Bind damage interrupt — if this reviver takes damage, cancel revive
	if (Character)
	{
		Character->OnTakeAnyDamage.AddDynamic(this, &UReviveComponent::OnReviverTakeDamage);
	}
}

// -----------------------------------------------------------------------
// Tick — tracks revive progress elapsed time for HUD progress bar
// -----------------------------------------------------------------------

void UReviveComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bReviveInProgress)
	{
		ReviveElapsed += DeltaTime;
	}
}

// -----------------------------------------------------------------------
// Input — RevivePressed / ReviveReleased
// -----------------------------------------------------------------------

void UReviveComponent::RevivePressed()
{
	if (bReviveInProgress) return;
	if (!CanRevive()) return;

	// Look for a nearby downed teammate first
	AShooterGameCharacter* NearestDowned = FindNearestDownedTeammate();

	if (NearestDowned)
	{
		StartRevive(NearestDowned);
	}
	else
	{
		// No teammate nearby — attempt self-revive if this player is downed
		if (Character && Character->GetDownedComponent() &&
			Character->GetDownedComponent()->IsDowned())
		{
			bIsSelfRevive = true;
			StartRevive(Character);
		}
	}
}

void UReviveComponent::ReviveReleased()
{
	if (!bReviveInProgress) return;
	CancelRevive();
}

// -----------------------------------------------------------------------
// Find nearest downed teammate
// -----------------------------------------------------------------------

AShooterGameCharacter* UReviveComponent::FindNearestDownedTeammate() const
{
	if (!Character || !GetWorld()) return nullptr;

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(ReviveRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character); // ignore self

	GetWorld()->OverlapMultiByChannel(
		Overlaps,
		Character->GetActorLocation(),
		FQuat::Identity,
		ECollisionChannel::ECC_Pawn,
		Sphere,
		Params
	);

	AShooterGameCharacter* NearestTarget = nullptr;
	float NearestDist = FLT_MAX;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AShooterGameCharacter* OtherCharacter =
			Cast<AShooterGameCharacter>(Overlap.GetActor());

		if (!OtherCharacter) continue;

		UDownedComponent* OtherDowned = OtherCharacter->GetDownedComponent();
		if (!OtherDowned || !OtherDowned->IsDowned()) continue;

		// TODO: Team check goes here when team system is implemented
		// For now all AShooterGameCharacter instances are friendly (PvE)

		float Dist = FVector::Dist(Character->GetActorLocation(),
			OtherCharacter->GetActorLocation());

		if (Dist < NearestDist)
		{
			NearestDist = Dist;
			NearestTarget = OtherCharacter;
		}
	}

	return NearestTarget;
}

// -----------------------------------------------------------------------
// Start revive
// -----------------------------------------------------------------------

void UReviveComponent::StartRevive(AShooterGameCharacter* Target)
{
	if (!Target) return;

	ReviveTarget  = Target;
	bReviveInProgress = true;
	ReviveElapsed = 0.f;

	// Pause the target's bleedout while being revived
	UDownedComponent* TargetDowned = Target->GetDownedComponent();
	if (TargetDowned)
	{
		TargetDowned->PauseBleedout();
	}

	const float Duration = bIsSelfRevive ? SelfReviveDuration : ReviveDuration;

	UE_LOG(LogTemp, Log,
		TEXT("UReviveComponent::StartRevive — reviving %s (%.1fs, self: %s)"),
		*Target->GetName(),
		Duration,
		bIsSelfRevive ? TEXT("true") : TEXT("false"));

	GetWorld()->GetTimerManager().SetTimer(
		ReviveTimerHandle,
		this,
		&UReviveComponent::OnReviveComplete,
		Duration,
		false
	);
}

// -----------------------------------------------------------------------
// Cancel revive — key released or reviver took damage
// -----------------------------------------------------------------------

void UReviveComponent::CancelRevive()
{
	if (!bReviveInProgress) return;

	GetWorld()->GetTimerManager().ClearTimer(ReviveTimerHandle);

	// Resume bleedout on target
	if (ReviveTarget)
	{
		UDownedComponent* TargetDowned = ReviveTarget->GetDownedComponent();
		if (TargetDowned)
		{
			// Only resume if not being revived by someone else
			TargetDowned->ResumeBleedout();
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("UReviveComponent::CancelRevive — revive cancelled on %s"),
		ReviveTarget ? *ReviveTarget->GetName() : TEXT("None"));

	ReviveTarget      = nullptr;
	bReviveInProgress = false;
	ReviveElapsed     = 0.f;
	bIsSelfRevive     = false;
}

// -----------------------------------------------------------------------
// Complete revive
// -----------------------------------------------------------------------

void UReviveComponent::OnReviveComplete()
{
	if (!ReviveTarget) return;

	UDownedComponent* TargetDowned = ReviveTarget->GetDownedComponent();
	if (TargetDowned)
	{
		TargetDowned->ServerCompleteRevive(ReviveHealth);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("UReviveComponent::OnReviveComplete — %s revived with %.1f HP"),
		*ReviveTarget->GetName(),
		ReviveHealth);

	ReviveTarget      = nullptr;
	bReviveInProgress = false;
	ReviveElapsed     = 0.f;
	bIsSelfRevive     = false;
}

// -----------------------------------------------------------------------
// Damage interrupt
// -----------------------------------------------------------------------

void UReviveComponent::OnReviverTakeDamage(
	AActor* DamagedActor,
	float Damage,
	const UDamageType* DamageType,
	AController* InstigatedBy,
	AActor* DamageCauser)
{
	if (!bReviveInProgress) return;

	UE_LOG(LogTemp, Log,
		TEXT("UReviveComponent::OnReviverTakeDamage — revive interrupted (%.1f damage)"),
		Damage);

	CancelRevive();
}

// -----------------------------------------------------------------------
// Progress accessor — used by HUD progress bar
// -----------------------------------------------------------------------

float UReviveComponent::GetReviveProgress() const
{
	if (!bReviveInProgress) return 0.f;
	const float Duration = bIsSelfRevive ? SelfReviveDuration : ReviveDuration;
	return FMath::Clamp(ReviveElapsed / Duration, 0.f, 1.f);
}