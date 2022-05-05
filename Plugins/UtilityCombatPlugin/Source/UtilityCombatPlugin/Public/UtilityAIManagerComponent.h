// Copyright Zachary Kolansky, 2020

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UtilityCombatDataStructures.h"
#include "UtilityAIManagerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUtilityTaskEvent, FName, TaskName);

class UUtilityCombatTaskComponent;
class APawn;
class AAIController;
class UCharacterMovementComponent;
class UPawnMovementComponent;

/*
Determines which tasks are the best to do. 
Interfaces with AI Controller, to interface with the Controlled pawn.
Task Components query this to get normalized values for their curves.
*/
UCLASS( Blueprintable,BlueprintType,ClassGroup=(UtilityCombat), meta=(BlueprintSpawnableComponent) )
class UTILITYCOMBATPLUGIN_API UUtilityAIManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUtilityAIManagerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/*
	DELEGATES
	*/

	/*
	Called whenever a task has begun
	*/
	UPROPERTY(BlueprintAssignable)
	FUtilityTaskEvent OnAnyTaskEnter;

	/*
	Called whenever a task has ended
	*/
	UPROPERTY(BlueprintAssignable)
	FUtilityTaskEvent OnAnyTaskExit;


	/*
	VARIABLES
	*/



	UPROPERTY(VisibleDefaultsOnly, Transient, BlueprintReadWrite, Category = ControlledPawn)
	bool bIsCrouched = false;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = ControlledPawn)
	APawn* ControlledPawn = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Transient, Category = ControlledPawn)
	UPawnMovementComponent* PawnMovementComp = nullptr; 
	
	UPROPERTY(VisibleDefaultsOnly, Transient, Category = ControlledPawn)
	UCharacterMovementComponent* CharMC = nullptr; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	bool bDrawLineTraces = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	bool bShowDebugWarnings = false;

	/*
	Refers to whether DistanceToFocusLastDetectedPoint has been set
	*/
	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = Focus)
	bool bFocusLastDetectedSet = false;
	
	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = Focus)
	bool bIsFocusAnyAttacking = false;

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = Focus)
	bool bIsFocusMeleeAttacking = false;

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = Focus)
	bool bIsFocusRangeAttacking = false;


	UPROPERTY(VisibleAnywhere, Transient,Category = Focus)
	AActor* ControllerFocus = nullptr;

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = Focus)
	float DistanceToFocus = -1.0f;

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = Focus)
	float DistanceToFocusLastDetectedPoint = -1.0f;

	UPROPERTY(EditAnywhere, Transient, BlueprintReadWrite, Category = Focus)
	FVector FocusLastDetectedPoint = FVector(0.0f,0.0f,0.0f);


	/*
	About where do we stop when walking to the focus.
	Also used as the denominator to normalize DistanceToFocus for  GetNormalizedStat()
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Manager)
	float ComfortableDistance = 1000.0f;

	/*
	Not related to AI senses. 
	used as the denominator to normalize DistanceToFocusLastDetectedPoint for  GetNormalizedStat()
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Manager)
	float MaxFocusSearchDistance = 1000.0f;

	/*
	CurrentTasks in progress. 
	*/
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Transient, Category = Manager)
	TMap<int32,UUtilityCombatTaskComponent*> CurrentTasks = {};


	/*
	This ActorComponent is meant to be attached to an AAIController
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = Manager)
	AAIController* OwnerController = nullptr;

	/*
	All possible layers that tasks can run
	*/
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Transient, Category = Manager)
	TArray<int32> PossibleLayers = {};

	/*
	All the possible tasks.
	*/
	UPROPERTY(VisibleAnywhere, Transient, Category = Manager)
	TArray<UUtilityCombatTaskComponent*> TaskArray = {};

	/*
	If true,  any task will be ended if its score is below the  TaskThreshold. 
	If false, task will not be auto ended based on threshold.
	*/
	UPROPERTY(EditAnywhere,Category = Manager)
	bool bAutoEndTasksIfTaskFallsBelowThreshold = true;

	/*
	Only matters if bAutoEndTasksIfTaskFallsBelowThreshold = true

	If false, any task can be ended if it falls below threshold
	If true, only interruptable tasks can be aut ended.

	*/
	UPROPERTY(EditAnywhere,Category = Manager)
	bool bOnlyAutoEndInterruptableTaskFallsBelowThreshold = false;

	/*
	Minimum value a task must score to be considered
	*/
	UPROPERTY(EditAnywhere,Category = Manager)
	float TaskThreshold = 0.1f;


	/*
	What Range do we begin to swap to melee
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Melee)
	float MeleeRange = 400.0f;

	/*
	What Range do we begin to swap from melee
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Melee)
	float MeleeRangeFallout = 600.0f;

	/*
	if we have a melee weapon
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Melee)
	bool bMeleeWeaponArmed = false;



	
	UPROPERTY(EditAnywhere, Transient, BlueprintReadWrite, Category = PointOfInterest)
	bool bCurrentPointOfInterestSet = false;

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = PointOfInterest)
	FVector CurrentPointOfInterest = FVector(0.0f,0.0f,0.0f);

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = PointOfInterest)
	float DistanceToCurrentPointOfInterest = -1.0f;

	/*
	Used as the denominator to normalize DistanceToCurrentPointOfInterest for  GetNormalizedStat()
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Manager)
	float MaxDistanceToCurrentPointOfInterest = 1000.0f;

	//Uncheck to save processoing power for AI that you don't want to be in cover.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cover)
	bool bCanFindCover = true;

	/*
	If we ever calculated a valid cover hit result
	*/
	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = Cover)
	bool bIsCoverHitResultValid = false;

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = Cover)
	bool bIsCoverPointSafe = false;

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = Cover)
	bool bIsInCover = false;



	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = Cover)
	FHitResult ClosestCoverHitResult = FHitResult();

	/*
	When the Distance to the focus is less than this value, the cover point is no longer considered safe.
	*/
	UPROPERTY(EditAnywhere, Transient, BlueprintReadWrite, Category = Cover)
	float CoverInvalidationDistance = 300.0f;

	/*
	How far we want to search for cover
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cover)
	float CoverPointSearchDistance = 3000.0f;

	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite, Category = Cover)
	float DistanceToCover = -1.0f;

	/*
	Degrees between cover point line trace search
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cover)
	float LineTraceDegrees = 20.0f;


	/*
	Only consider actors that are marked as cover
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Cover)
	FName ValidCoverPointTag = FName("Cover");


	



	/*
	FUNCTIONS
	*/

	/*
	Call when the pawn is possessed
	*/
	UFUNCTION(BlueprintCallable, Category = Initialize)
	void Initialize();

	/*
	Calculations that need to be done before calling ScoreTasks()
	*/
	void AnteScoreCalculations();

	/*
	This does the work of evaluationing the tasks

	Calls AnteScoreCalculations();
	ScoreTasks();
	ChangeToBestTasks();

	*/
	UFUNCTION(BlueprintCallable, Category = Basic)
	void DetermineBestTask();

	/*
	Interrupt all tasks that we can. Clean up any ended tasks. Only do the BestTasks on layers that are freed for a new task.
	*/
	void ChangeToBestTasks(TMap<int32,UUtilityCombatTaskComponent*>& BestTasks );

	void FindClosestCoverPoint();

	float GetNormalizedStat(const ECurveInputQuery CurveInputQuery) const;

	float GetNormalizedStat(const FName& InputStat) const;

	bool IsCoverHitResultValid() const;

	bool IsCoverHitResultSafe() const;




	TMap<int32,UUtilityCombatTaskComponent*> ScoreTasks();
	
	/*
	Call when the owner is possessed.
	*/
	UFUNCTION(BlueprintCallable, Category = Basic)
	void SetMovementComponentPointers();
};
