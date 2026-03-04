// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Highlightable.h"
#include "Components/StaticMeshComponent.h"
#include "HighlightableStaticMesh.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API UHighlightableStaticMesh : public UStaticMeshComponent, public IHighlightable
{
	GENERATED_BODY()
	
public:
	virtual void Highlight_Implementation() override;
	virtual void UnHighlight_Implementation() override;
	
	
private:
	
	UPROPERTY(EditAnywhere, Category = "Item | Highlight")
	TObjectPtr<UMaterialInterface> HighlightMaterial;
	
};
