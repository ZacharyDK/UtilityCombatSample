// Copyright Zachary Kolansky, 2020

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "UtilityPlayerController.generated.h"

/**
 * Basic Player controller class. Used to implement IGenericTeamAgentInterface, and expose the team functionality to Blueprint.
 * Additionally, I provide a method called GetAimLocationInWorld() to determine where the crosshair will Project into world.
 */
UCLASS()
class UTILITYCOMBATPLUGIN_API AUtilityPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AUtilityPlayerController();


	/*
	Fraction on the Screen the CrossHair is located
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = CrossHair)
	float CrossHairXLocation = 0.5f;

	/*
	Fraction on the Screen the CrossHair is located
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = CrossHair)
	float CrossHairYLocation = 0.5f;


	/*
	How far in Unreal Units (cm) that we want to ray cast into the world
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = CrossHair)
	float SightLength = 1000.0f;


	/*
	If true, the Camera direction will be printed
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Debug)
	bool bShowDebugWarnings = false;

	/*
	If true, the Line trace will be shown in the world
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Debug)
	bool bShowDebugLineTrace = false;




	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Team)
	int32 TeamIDNumber = 2;



	UFUNCTION(BlueprintCallable, Category = CrossHair)
	FHitResult GetAimLocationInWorld();

	/*
	Exposes Team functionality to Blueprint

	Changes the value of TeamIDNumber to NewTeamID
	Calls  SetGenericTeamId(FGenericTeamId(TeamIDNumber));
	*/
	UFUNCTION(BlueprintCallable, Category = Team)
	void SetTeamID(int32 NewTeamID);


private: 
  // Implement The Generic Team Interface 
  FGenericTeamId PlayerTeamId;

  FGenericTeamId GetGenericTeamId() const;

  void SetGenericTeamId(const FGenericTeamId& TeamID);
	
};
