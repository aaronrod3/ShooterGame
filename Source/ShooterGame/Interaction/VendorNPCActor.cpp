// Source/ShooterGame/Interaction/VendorNPCActor.cpp
#include "ShooterGame/Interaction/VendorNPCActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "ShooterGame/Interaction/HighlightableStaticMesh.h"
#include "ShooterGame/Quest/QuestDefinition.h"

AVendorNPCActor::AVendorNPCActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    VendorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VendorMesh"));
    VendorMesh->SetupAttachment(SceneRoot);
    VendorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    VendorMesh->SetCollisionResponseToAllChannels(ECR_Block);

    InteractionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("InteractionCapsule"));
    InteractionCapsule->SetupAttachment(SceneRoot);
    InteractionCapsule->SetCapsuleHalfHeight(96.0f);
    InteractionCapsule->SetCapsuleRadius(48.0f);
    InteractionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    HighlightMeshComponent = CreateDefaultSubobject<UHighlightableStaticMesh>(TEXT("HighlightMeshComponent"));
    HighlightMeshComponent->SetupAttachment(VendorMesh);

    InteractPromptText = FText::FromString(TEXT("Interact"));

    // Lobby vendors do not need networked movement/replication at this stage.
    // Server-authoritative transaction and quest state will live in backend systems.
    bReplicates = false;
}

void AVendorNPCActor::BeginPlay()
{
    Super::BeginPlay();
}

void AVendorNPCActor::Interact_Implementation(ACharacter* InteractingCharacter)
{
    if (!CanInteract_Implementation(InteractingCharacter))
    {
        return;
    }

    APlayerController* InteractingPlayerController = Cast<APlayerController>(InteractingCharacter->GetController());
    if (!IsValid(InteractingPlayerController))
    {
        return;
    }

    OnVendorInteracted.Broadcast(InteractingPlayerController, this);
}

bool AVendorNPCActor::CanInteract_Implementation(ACharacter* InteractingCharacter) const
{
    // Phase 3 intentionally deferred interaction range validation.
    // For now, only require a valid character. Future additions:
    // - range/overlap checks against InteractionCapsule
    // - lobby-only gating
    // - faction restrictions
    // - cinematic locks / cooldowns
    return IsValid(InteractingCharacter);
}

FText AVendorNPCActor::GetInteractPromptText_Implementation(ACharacter* InteractingCharacter) const
{
    return InteractPromptText;
}

TArray<UQuestDefinition*> AVendorNPCActor::GetAvailableQuests_Implementation() const
{
    TArray<UQuestDefinition*> Result;
    Result.Reserve(OfferedQuests.Num());

    for (const TObjectPtr<UQuestDefinition>& Quest : OfferedQuests)
    {
        if (IsValid(Quest))
        {
            Result.Add(Quest.Get());
        }
    }

    return Result;
}

bool AVendorNPCActor::OffersQuests_Implementation() const
{
    return OfferedQuests.Num() > 0;
}

bool AVendorNPCActor::CanTurnInQuest_Implementation(UQuestDefinition* QuestDefinition) const
{
    return IsValid(QuestDefinition);
}

bool AVendorNPCActor::TurnInQuest_Implementation(APlayerController* InteractingPlayer, UQuestDefinition* QuestDefinition)
{
    // Foundation-only implementation.
    // Real quest completion validation and reward payout should be delegated
    // to UQuestTrackerSubsystem and save/economy systems in a later step.
    return IsValid(InteractingPlayer) && IsValid(QuestDefinition);
}