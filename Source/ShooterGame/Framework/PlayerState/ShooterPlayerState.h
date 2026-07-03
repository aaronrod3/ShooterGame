#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "ShooterPlayerState.generated.h"


class ULoadoutComponent;
class ACharacter;

UCLASS()
class SHOOTERGAME_API AShooterPlayerState : public APlayerState
{
    GENERATED_BODY()

public:

    AShooterPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // -----------------------------------------------------------------------
    // Loadout Interface — called by lobby UI and SaveSubsystem
    // -----------------------------------------------------------------------

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Loadout")
    void Server_UpdateLoadoutSlot(EEquipmentSlot Slot, const FLoadoutSlot& NewSlotData);

    UFUNCTION(BlueprintCallable, Category = "Loadout")
    void SetFullLoadout(const FLoadoutData& NewLoadout, bool bPushToCharacter = false);

    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Loadout")
    void Server_UpdateAppearance(const FCharacterAppearance& NewAppearance);

    // -----------------------------------------------------------------------
    // Character Possession Bridge
    // -----------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Loadout")
    void PushLoadoutToCharacter(ACharacter* NewCharacter);

    // -----------------------------------------------------------------------
    // Read Accessors
    // -----------------------------------------------------------------------

    FORCEINLINE const FLoadoutData&         GetSavedLoadout()    const { return SavedLoadout; }
    FORCEINLINE const FCharacterAppearance& GetSavedAppearance() const { return SavedAppearance; }

    // Returns true if the saved loadout has at least one occupied slot.
    bool HasSavedLoadout() const { return SavedLoadout.HasAnyItems(); }

    // Overwrites the saved loadout without pushing to the character.
    // Used to seed the PlayerState from the character's DefaultLoadout
    // before PushLoadoutToCharacter is called.
    void SetSavedLoadout(const FLoadoutData& NewLoadout) { SavedLoadout = NewLoadout; }

protected:

    virtual void CopyProperties(APlayerState* PlayerState) override;

private:

    UPROPERTY(ReplicatedUsing = OnRep_SavedLoadout)
    FLoadoutData SavedLoadout;

    UPROPERTY(ReplicatedUsing = OnRep_SavedAppearance)
    FCharacterAppearance SavedAppearance;

    UFUNCTION()
    void OnRep_SavedLoadout();

    UFUNCTION()
    void OnRep_SavedAppearance();

    ULoadoutComponent* GetOwnedLoadoutComponent() const;
    void NotifySaveSubsystem();
};