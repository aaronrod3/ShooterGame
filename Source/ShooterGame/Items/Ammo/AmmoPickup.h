// Source/ShooterGame/Interaction/AmmoPickup.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterGame/Items/Ammo/WeaponFeedTypes.h"
#include "AmmoPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UCombatComponent;

UCLASS()
class SHOOTERGAME_API AAmmoPickup : public AActor
{
	GENERATED_BODY()

public:
	AAmmoPickup();

	/**
	 * The magazine this pickup grants on collection.
	 * Set AmmoType, Capacity, and CurrentRounds per placed instance in the editor.
	 * e.g. AmmoType=5.56, Capacity=30, CurrentRounds=30 for a full mag.
	 *      AmmoType=5.56, Capacity=30, CurrentRounds=17 for a partial mag.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Ammo", meta = (ExposeOnSpawn = "true"))
	FMagazine GrantedMagazine;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

private:

	UPROPERTY(VisibleAnywhere, Category = "Pickup|Components")
	USphereComponent* PickupSphere;

	UPROPERTY(VisibleAnywhere, Category = "Pickup|Components")
	UStaticMeshComponent* PickupMesh;
};