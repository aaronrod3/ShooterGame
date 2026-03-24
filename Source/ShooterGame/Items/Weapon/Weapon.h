// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"


class USphereComponent;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial			UMETA(DisplayName = "Initial"),
	EWS_Equipped		UMETA(DisplayName = "Equipped"),
	EWS_Dropped			UMETA(DisplayName = "Dropped"),
	
	EWS_Max				UMETA(DisplayName = "DefaultMax")
};



UCLASS()
class SHOOTERGAME_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);
	
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere() const { return CollisionSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetCurrentSpread() const { return CurrentSpread; }
	FORCEINLINE float GetMaxSpread() const { return MaxSpread; }
	FORCEINLINE float GetWeaponRange() const { return WeaponRange; }
	

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
		);
	
	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* CollisionSphere;
	
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Spread")
	float CurrentSpread; // 0.0 = fully accurate, 1.0 = max spread

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Spread")
	float SpreadIncreasePerShot = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Spread")
	float SpreadDecayRate = 2.5f; // per second, how fast it recovers

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Spread")
	float MaxSpread = 1.0f;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float WeaponRange = 1500.f; // cm, tune per weapon
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACaseEject> CasingClass;
	
	
	UFUNCTION()
	void OnRep_WeaponState();
	
	
};
