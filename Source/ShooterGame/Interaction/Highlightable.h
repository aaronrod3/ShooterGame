// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Highlightable.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UHighlightable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SHOOTERGAME_API IHighlightable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	UFUNCTION(BlueprintNativeEvent, Category = "Item | Highlight")
	void Highlight();
	
	UFUNCTION(BlueprintNativeEvent, Category = "Item | Highlight")
	void UnHighlight();
	
	
	
	
	
	
	
	
	
	
	
	
	
};
