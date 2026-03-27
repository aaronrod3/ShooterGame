// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShooterGame/Types/FireMode.h"
#include "CombatComponent.generated.h"


// Local-only reticle runtime state, computed each tick on owning client 
USTRUCT()
struct FReticleState
{
	GENERATED_BODY()

	bool bIsEquipped			= false;
	bool bIsAiming				= false;
	bool bIsCrouched			= false;
	float SpreadAlpha			= 0.f;								// 0 = fully accurate, 1 = max spread
	FVector2D CursorScreenPos	= FVector2D(0.f, 0.f);		// mouse position in screen space
	bool bCursorValid			= false;							// is mouse over valid viewport
};



UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	friend class AShooterGameCharacter;
	friend class AShooterGamePlayerController;
	
	void EquipWeapon(class AWeapon* WeaponToEquip);
	
	FORCEINLINE const FReticleState& GetReticleState() const { return ReticleState; }
	FORCEINLINE FVector GetReticleWorldPosition() const { return ReticleWorldPosition; }
	

protected:
	virtual void BeginPlay() override;
	
	void SetAiming(bool bIsAiming);
	
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);
	
	UFUNCTION()
	void OnRep_EquippedWeapon();
	void FireButtonPressed(bool bPressed);
	void FireButtonReleased();
	void CycleFireMode();
	void HandleFire();
	void HandleBurstFire();
	
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& InHitTarget);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire();

private:
	
	class AShooterGameCharacter* Character;
	
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;
	
	UPROPERTY(Replicated)
	bool bAiming;
	
	bool bFireButtonPressed;
	FVector HitTarget;
	FVector ReticleWorldPosition = FVector::ZeroVector;
	FTimerHandle FireTimerHandle;
	FTimerHandle BurstFireTimerHandle;
	bool bCanFire = true;              // semi / burst cooldown gate
	int32 BurstShotsRemaining = 0;    // tracks shots left in a burst sequence
	bool bFullAutoFiring = false;     // true while LMB is held in full auto
	

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	
	// Reticle state, local client only, never replicated
	FReticleState ReticleState;
	void UpdateReticleState();
	void UpdateReticleWorldPosition();
	
	
	UPROPERTY(EditAnywhere, Category = "Reticle")
	float ReticleMaxDistance = 2000.f;
	
	UPROPERTY(EditAnywhere, Category = "Reticle")
	float CrouchSpreadMultiplier = 0.7f;   // < 1.0 tightens spread while crouched

	UPROPERTY(EditAnywhere, Category = "Reticle")
	float MovementSpreadMultiplier = 0.4f; // how much max movement adds to spread (0..1 range addition)
	
};
