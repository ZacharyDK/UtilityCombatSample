// Copyright Zachary Kolansky, 2020

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "UtilityAIController.generated.h"

/**
 * A custom AIController base to expose basic functionality of the TeamSystem to Blueprint, and 
 * provide a way to modify the result from GetFocalPointOnActor() with Blueprints.
 *  
 * This solves:
 * 1) AI always focuses on players feet when Aim offsets are enabled. 
 * 2) A way for the AI to ignore sensing each other if they are on the same team. 
 * AI sensing each other can cause uncessary slowdowns when a bunch of AIs on the same team are together
 * 
 */
UCLASS()
class UTILITYCOMBATPLUGIN_API AUtilityAIController : public AAIController
{
	GENERATED_BODY()

public:
	AUtilityAIController();
	
	/*
	Expose what integer will passed into
	SetGenericTeamId(FGenericTeamId(TeamID)) in the constructor.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Team)
	int32 TeamIDNumber = 1;

	/*
	Actor location is typically the root of the objects. 
	Pawns will have their root at the feet, but we want to focus at a point above the
	feet, like a chest or face.  

	To do this, I override GetFocalPointOnActor(const AActor* Actor) const
	If the given actor is a APawn, then the focal point will be
	
	ActorLocation + FVector(0.0f,0.0f,FocusEyeHeight)*ActorScale.Z

	Otherwise, I call the super function

	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = FocusSettings)
	float FocusEyeHeight = 80.0f;

	/*
	Overwridden from the parent
	*/
	FVector GetFocalPointOnActor(const AActor* Actor) const override; 

	/*
	Barebones implementation
	*/
	ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	/*
	Exposes Team functionality to Blueprint

	Changes the value of TeamIDNumber to NewTeamID
	Calls  SetGenericTeamId(FGenericTeamId(TeamIDNumber));
	*/
	UFUNCTION(BlueprintCallable, Category = Team)
	void SetTeamID(int32 NewTeamID);
	
};
