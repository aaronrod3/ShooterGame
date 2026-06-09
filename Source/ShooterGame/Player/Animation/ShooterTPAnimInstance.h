#pragma once

#include "CoreMinimal.h"
#include "ShooterAnimInstanceBase.h"
#include "ShooterTPAnimInstance.generated.h"

/**
 * Third-person anim instance.
 * Inherits all shared state from UShooterAnimInstanceBase.
 * This is the class your existing ABP (ABP_ShooterCharacter or equivalent)
 * should be reparented to in the editor.
 *
 * TP-specific additions will be added in Milestone 6 (Infima montage
 * selection, overlay state, action gating).
 * For now it is intentionally thin — just the right parent class.
 */
UCLASS()
class SHOOTERGAME_API UShooterTPAnimInstance : public UShooterAnimInstanceBase
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// -----------------------------------------------------------------------
	// TP-only state (expanded in Milestone 6)
	// -----------------------------------------------------------------------

	/**
	 * True while the character is using the rifle two-handed grip.
	 * Currently always true when a rifle is equipped — Milestone 7 will
	 * drive this from grip notify states.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|TP")
	bool bUseTwoHandedGrip = false;

private:
	void UpdateTPData();
};