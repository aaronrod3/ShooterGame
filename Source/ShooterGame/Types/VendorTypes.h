// Source/ShooterGame/Types/VendorTypes.h
#pragma once

#include "CoreMinimal.h"
#include "QuestTypes.h"
#include "VendorTypes.generated.h"


// ============================================================================
// FVendorInventoryEntry
//
// One line item in a vendor's stock list, authored on BP_VendorNPC_* in editor.
//
// StockCount      — -1 means infinite supply; >= 0 is depleting stock.
// BasePriceCredits — base cost before any reputation discount is applied.
//                   Phase 5 economy will apply rep-scaled discounts on top.
// MinReputationRequired — vendor will not show or sell this entry until the
//                         player's ReputationLevel with this vendor is >= this.
// ============================================================================
USTRUCT(BlueprintType)
struct SHOOTERGAME_API FVendorInventoryEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor|Stock")
    TSoftObjectPtr<class UItemDefinition> ItemDefinition;

    // -1 = infinite; 0 = sold out; > 0 = remaining units
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor|Stock",
        meta = (ClampMin = "-1"))
    int32 StockCount = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor|Stock",
        meta = (ClampMin = "0.0"))
    float BasePriceCredits = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor|Stock",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float MinReputationRequired = 0.0f;
};


// ============================================================================
// FVendorTransactionResult
//
// Returned by AVendorNPCActor's server-side purchase and sell logic.
// The client receives this via ClientReceiveTransactionResult() RPC
// to display feedback in UVendorWidget without trusting client state.
// ============================================================================
USTRUCT(BlueprintType)
struct SHOOTERGAME_API FVendorTransactionResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vendor")
    bool bSuccess = false;

    // Populated only when bSuccess == false — shown as UI error toast
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vendor")
    FText FailureReason;
};