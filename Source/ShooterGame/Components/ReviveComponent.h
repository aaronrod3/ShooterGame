// ReviveComponent.h
// Owns the reviver-side logic: proximity detection, hold-to-revive timer,
// interrupt handling, and self-revive delegation to UDownedComponent.
// Lives on AShooterGameCharacter.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ReviveComponent.generated.h"

class AShooterGameCharacter;
class UDownedComponent;
class UDamageType;
class AController;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UReviveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReviveComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// -----------------------------------------------------------------------
	// Public API — called by input bindings on AShooterGameCharacter
	// -----------------------------------------------------------------------

	// Player holds the revive key
	void RevivePressed();

	// Player releases the revive key
	void ReviveReleased();

	// -----------------------------------------------------------------------
	// Configuration — set in BP defaults
	// -----------------------------------------------------------------------

	// How close (cm) the reviver must be to a downed player to begin reviving
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revive")
	float ReviveRadius = 120.f;

	// Time (seconds) to hold revive key to complete a teammate revive
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revive")
	float ReviveDuration = 5.f;

	// Time (seconds) to hold revive key for self-revive (longer)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revive")
	float SelfReviveDuration = 10.f;

	// HP restored to the downed player on successful revive
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Revive")
	float ReviveHealth = 30.f;

	// -----------------------------------------------------------------------
	// State accessors
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintPure)
	bool IsReviving() const { return bReviveInProgress; }

	UFUNCTION(BlueprintPure)
	float GetReviveProgress() const;

	UFUNCTION(BlueprintPure)
	AShooterGameCharacter* GetReviveTarget() const { return ReviveTarget; }

protected:
	virtual void BeginPlay() override;

private:
	// -----------------------------------------------------------------------
	// Internal state
	// -----------------------------------------------------------------------

	// The downed character currently being revived — null if not reviving
	UPROPERTY()
	AShooterGameCharacter* ReviveTarget = nullptr;

	// True while revive key is held and a target is found
	bool bReviveInProgress = false;

	// Elapsed time since revive started (used for progress HUD)
	float ReviveElapsed = 0.f;

	// True if this is a self-revive attempt
	bool bIsSelfRevive = false;

	FTimerHandle ReviveTimerHandle;

	// Owning character — cached in BeginPlay
	UPROPERTY()
	AShooterGameCharacter* Character = nullptr;

	// -----------------------------------------------------------------------
	// Internal helpers
	// -----------------------------------------------------------------------

	// Sphere overlap to find the nearest downed teammate within ReviveRadius
	// Returns nullptr if none found
	AShooterGameCharacter* FindNearestDownedTeammate() const;

	// Begin reviving a specific target (or self if target == owner)
	void StartRevive(AShooterGameCharacter* Target);

	// Cancel any in-progress revive — resumes bleedout on target
	void CancelRevive();

	// Called when ReviveTimerHandle fires — revive complete
	void OnReviveComplete();

	// Bound to owning character's OnTakeAnyDamage — interrupts revive
	UFUNCTION()
	void OnReviverTakeDamage(
		AActor* DamagedActor,
		float Damage,
		const UDamageType* DamageType,
		AController* InstigatedBy,
		AActor* DamageCauser);

	// Medical kit stub — always returns true until kit system is implemented
	static bool CanRevive() { return true; }
};