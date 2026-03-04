// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"


UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Initial			UMETA(DisplayName = "Initial"),
	Equipped		UMETA(DisplayName = "Equipped"),
	Dropped			UMETA(DisplayName = "Dropped"),
	
	Max				UMETA(DisplayName = "DefaultMax")
};



UCLASS()
class SHOOTERGAME_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UStaticMeshComponent* WeaponMesh;
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* CollisionSphere;
	
	UPROPERTY(VisibleAnywhere)
	EWeaponState WeaponState;
	
	
	
};
