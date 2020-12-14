// Copyright Zachary Kolansky, 2020

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UtilityAIManagerToPawnInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UUtilityAIManagerToPawnInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 * The purpose of this Interface is allow the UUtilityAIManagerComponent to query the Controlled Pawn of its associated AAIController. 
 * The main method GetNormalizedStat(), expects to be given an FName, and should return a value from 0.0f to 1.0f.
 * 
 * It is also used to query information from the focus.
 */
class UTILITYCOMBATPLUGIN_API IUtilityAIManagerToPawnInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	/*
	Get a given stat from this controlled pawn
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ControlledPawn_UMPI")
    float GetNormalizedStat(const FName InputStatName);

	/*
	Does the controlled pawn have a melee weapon equipped?
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ControlledPawn_UMPI")
    bool IsInMelee();

	/*
	Is the focus using a melee attack
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Focus_UMPI")
    bool IsFocusMeleeAttack();

	/*
	Is the focus using a range attack
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Focus_UMPI")
    bool IsFocusRangeAttack();



};
