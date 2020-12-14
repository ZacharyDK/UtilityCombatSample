// Copyright Zachary Kolansky, 2020


#include "UtilityAIManagerComponent.h"
#include "AIController.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "UtilityCombatTaskComponent.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/Pawn.h"
#include "Runtime/Engine/Public/DrawDebugHelpers.h"
#include "GameFramework/PawnMovementComponent.h"	
#include "GameFramework/CharacterMovementComponent.h"
#include "UtilityAIManagerToPawnInterface.h"

// Sets default values for this component's properties
UUtilityAIManagerComponent::UUtilityAIManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;




	// ...
}


// Called when the game starts
void UUtilityAIManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	PrimaryComponentTick.bCanEverTick = false;
	
	
}


// Called every frame
void UUtilityAIManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UUtilityAIManagerComponent::Initialize()
{
	OwnerController = Cast<AAIController>(GetOwner());

	if(OwnerController)
	{
		ControlledPawn = OwnerController->GetPawn();
		OwnerController->GetComponents<UUtilityCombatTaskComponent>(TaskArray,false);


	}

	for( UUtilityCombatTaskComponent* UCTC : TaskArray)
	{
		if(UCTC)
		{
			PossibleLayers.AddUnique(UCTC->TaskLayer);
			UCTC->OwnerController = OwnerController;
		}
	
	}
	
	

}

void UUtilityAIManagerComponent::AnteScoreCalculations()
{
	//Reset cover point variables
	
	
	FVector OwnerLocation;
	
	if(OwnerController)
	{
		ControllerFocus = OwnerController->GetFocusActor(); //Cache it
		ControlledPawn = OwnerController->GetPawn();
		if(!ControlledPawn)
		{
			DistanceToFocus = -1.0f;
			DistanceToCurrentPointOfInterest = -1.0f;
			return;
		}
		OwnerLocation = ControlledPawn->GetActorLocation();
		
		
	}
	
	if(ControllerFocus && OwnerController)
	{
		FVector FocusLocation = ControllerFocus->GetActorLocation();
		DistanceToFocus = (OwnerLocation-FocusLocation).Size();


	}
	else
	{
		DistanceToFocus = -1.0f;
	}
	
	if(bCurrentPointOfInterestSet)
	{
		DistanceToCurrentPointOfInterest = (OwnerLocation-CurrentPointOfInterest).Size();
	}
	else
	{
		DistanceToCurrentPointOfInterest = -1.0f;
	}

	if(bFocusLastDetectedSet)
	{
		DistanceToFocusLastDetectedPoint = (OwnerLocation-FocusLastDetectedPoint).Size();
	}
	else
	{
		DistanceToFocusLastDetectedPoint = -1.0f;
	}
	
	
	if(CharMC)
	{
		bIsCrouched = CharMC->IsCrouching();
	}

	if(bCanFindCover)
	{
		FindClosestCoverPoint();

		bIsCoverPointSafe = IsCoverHitResultSafe();

		if(!bIsCoverHitResultValid)
		{
			ClosestCoverHitResult = FHitResult();
			DistanceToCover = -1.0f;
			bIsCoverPointSafe = false;
		}
	
		
	}

	


	
	bool bImplementsInterface = false;
	if(ControlledPawn)
	{
		bImplementsInterface = ControlledPawn->GetClass()->ImplementsInterface(UUtilityAIManagerToPawnInterface::StaticClass());
		
		if(bShowDebugWarnings && !bImplementsInterface)
		{
			UE_LOG(LogTemp,Warning,TEXT("%s doesn't implement IUtilityAIManagerToPawnInterface interface "),*(ControlledPawn->GetFName().ToString() ) )
		}
		
	}

		
	
	if(ControlledPawn && bImplementsInterface)
	{	
		bMeleeWeaponArmed = IUtilityAIManagerToPawnInterface::Execute_IsInMelee(ControlledPawn);
	}

	bool bFocusImplementsInterface = false;
	if(ControllerFocus)
	{
		bFocusImplementsInterface = ControllerFocus->GetClass()->ImplementsInterface(UUtilityAIManagerToPawnInterface::StaticClass());
		
		if(bShowDebugWarnings && !bFocusImplementsInterface)
		{
			UE_LOG(LogTemp,Warning,TEXT("%s doesn't implement IUtilityAIManagerToPawnInterface interface "),*(ControllerFocus->GetFName().ToString() ) )
		}
		
	}



	
	if(ControllerFocus && bFocusImplementsInterface)
	{
		bIsFocusMeleeAttacking = IUtilityAIManagerToPawnInterface::Execute_IsFocusMeleeAttack(ControllerFocus);
		bIsFocusRangeAttacking = IUtilityAIManagerToPawnInterface::Execute_IsFocusRangeAttack(ControllerFocus);
	}
	else
	{
		bIsFocusMeleeAttacking = false;
		bIsFocusRangeAttacking = false;
	}

	if(bIsFocusMeleeAttacking || bIsFocusRangeAttacking)
	{
		bIsFocusAnyAttacking = true;
	}
	else
	{
		bIsFocusAnyAttacking = false;
	}
	
	
}


void UUtilityAIManagerComponent::ChangeToBestTasks(TMap<int32,UUtilityCombatTaskComponent*>& BestTasks )
{
	
	TMap<int32,UUtilityCombatTaskComponent*> NewCurrentTasks = {};
	NewCurrentTasks = CurrentTasks;


	TArray<int32> EndOrInterruptTaskLayers = {};

	/*
	1. Determine if the Current task is can be interrupted or end 
	2. Remember what layers we were able to end.
	3. Remember tasks that we couldn't end
	4. Begin all the tasks we can
	5. Change CurrentTasks to NewCurrentTasks
	*/
	
	
	

	for (int32 TaskLayer : PossibleLayers)
	{	
		if(!CurrentTasks.Contains(TaskLayer))
		{
			EndOrInterruptTaskLayers.Add(TaskLayer);
			continue;
		}
		
		if(BestTasks.Contains(TaskLayer))
		{
			UUtilityCombatTaskComponent* CurrentTask = CurrentTasks[TaskLayer];
			UUtilityCombatTaskComponent* BestTask = BestTasks[TaskLayer];

			if(!CurrentTask)
			{
				EndOrInterruptTaskLayers.Add(TaskLayer);
				continue;
			}

			//Clean up tasks that are done, or Interrupt and End tasks that we can end.

			if(!CurrentTask->bIsTaskActive || (bAutoEndTasksIfTaskFallsBelowThreshold && CurrentTask->CalculateTaskScore(this) < TaskThreshold) && (!bOnlyAutoEndInterruptableTaskFallsBelowThreshold || CurrentTask->CanTaskBeInterrupted(BestTask) ) )
			{
				EndOrInterruptTaskLayers.Add(TaskLayer);
				CurrentTask->ExitTask(); //Ensure clean up
				OnAnyTaskExit.Broadcast(CurrentTask->TaskName);
			}
			else if(CurrentTask->CanTaskBeInterrupted(BestTask) && BestTask && CurrentTask->TaskName != BestTask->TaskName) 
			{
				//If the best task is already in progress, we don't stop it.
				//CurrentTask->bIsTaskActive = false;
				EndOrInterruptTaskLayers.Add(TaskLayer);
				CurrentTask->ExitTask(); 
				OnAnyTaskExit.Broadcast(CurrentTask->TaskName);
			}
			else // Task cannot be interrupted. Don't end it, and remember it for next time.
			{
				NewCurrentTasks.Add(TaskLayer,CurrentTask);
			}
			
			


		}
	
	}

	
	//Replace the current tasks with the best tasks
	for (int32 TaskLayer : EndOrInterruptTaskLayers)
	{	
		

		if(!BestTasks.Contains(TaskLayer))
		{
			continue;
		}
		UUtilityCombatTaskComponent* BestTask = BestTasks[TaskLayer];
		if(!BestTask)
		{
			continue;
		}


		BestTask->EnterTask();
		NewCurrentTasks.Add(TaskLayer,BestTask);
		OnAnyTaskEnter.Broadcast(BestTask->TaskName);
	}

	CurrentTasks = NewCurrentTasks;
	
}


void UUtilityAIManagerComponent::DetermineBestTask()
{
	AnteScoreCalculations();
	TMap<int32,UUtilityCombatTaskComponent*> BT = ScoreTasks();
	ChangeToBestTasks(BT);
}

void UUtilityAIManagerComponent::FindClosestCoverPoint()
{

	
	bIsCoverHitResultValid = false;
	
	//1. Draw a series of line traces FROM some point away from the pawn TO the pawn
	//This stores all the hit results.
	//2. Determine the closest FHitResult.
	//3. Set variables accordingly.
	


	if(!OwnerController)
	{
		return;
	}
	
	if(!ControlledPawn)
	{
		return;
	}
	if(LineTraceDegrees <= 0.0f || LineTraceDegrees >= 360.0f )
	{
		LineTraceDegrees = 10.0f;
	}
	if(CoverPointSearchDistance <= 0.0f)
	{
		return;
	}

	

	if(PawnMovementComp)
	{
		if(PawnMovementComp->IsFalling() || !PawnMovementComp->IsMovingOnGround())
		{
			return; //Can't find cover if falling
		}
	}

	UWorld* World = GetWorld();

	FIntVector WorldOrig = World->OriginLocation;
	FVector WorldOrigin = FVector(WorldOrig.X,WorldOrig.Y,WorldOrig.Z);


	FVector ZAxis = ControlledPawn->GetActorUpVector();//FVector(0.0f,0.0f,1.0f); //We gonna rotate the forward vector N degrees around this
	FVector PawnForwardVector = ControlledPawn->GetActorForwardVector();
	FVector PawnLocation = ControlledPawn->GetActorLocation();
	float CurrentAngle = 0.0f;

	float ShortestDistanceFromSelf = CoverPointSearchDistance; //Starting value. This is the longest the trace can be
	float LongestCoverDistanceFromFocus = 0.0f;

	float BestDistanceToCover = 0.0f;

	FHitResult BestHit = FHitResult();


	//Finds the closest cover point
	while (CurrentAngle < 360.0f)
	{
		FVector StartLocation = PawnForwardVector.RotateAngleAxis(CurrentAngle,ZAxis)*CoverPointSearchDistance + PawnLocation - WorldOrigin;
		FHitResult CurrentHit; 

		World->LineTraceSingleByChannel(CurrentHit,StartLocation,PawnLocation,ECollisionChannel::ECC_Visibility);
		//Uses out parameter

		if(bDrawLineTraces)
		{
			DrawDebugLine(
            World,
            StartLocation,
            PawnLocation,
            FColor(255,0,0, 1),
            true,
            1.0f,
            1,
            3.0f
			);
		}

		if(!CurrentHit.GetActor())
		{
			CurrentAngle = CurrentAngle + LineTraceDegrees;
			continue; //Force next iteration of the loop, there isn't an actor to consider
		}
		else if(CurrentHit.GetActor() == ControllerFocus || CurrentHit.GetActor() == ControlledPawn ) //ClosestCoverHitResult.GetActor() && ClosestCoverHitResult.GetActor() == CurrentHit.GetActor() 
		{
			CurrentAngle = CurrentAngle + LineTraceDegrees;
			continue; //Force next iteration of the loop, we don't want to consider hits that collide with the focus
		}
		else if(!CurrentHit.GetActor()->ActorHasTag(ValidCoverPointTag))
		{
			CurrentAngle = CurrentAngle + LineTraceDegrees;
			continue; //Force next iteration of the loop, we don't wnat to consider actors that aren't cover points
		}

		float CurrentCoverDistanceFromSelf = FGenericPlatformMath::Abs(CoverPointSearchDistance-CurrentHit.Distance);
		
		//CurrentHit.Distance is the distance from the start of the
		//line trace, to the hit location. Objects that are near where the line trace started
		//have a small distance, but are far from the Pawn.
		

		if(!ControllerFocus && !bFocusLastDetectedSet)
		{
			//No focus, or the focus is in our confortzone, find the closest cover point.
			if(CurrentCoverDistanceFromSelf < ShortestDistanceFromSelf )
			{
				
				ShortestDistanceFromSelf = CurrentCoverDistanceFromSelf;
				BestHit = CurrentHit;
				BestDistanceToCover = CurrentCoverDistanceFromSelf;
			}
		}
		else if(ControllerFocus && DistanceToFocus > CoverInvalidationDistance) //Focus in our confort zone.
		{
			float DistanceFromFocusToCover = 0.0f;
			if(CurrentHit.GetActor())
			{
				FVector FocusLocation = ControllerFocus->GetActorLocation();
				FVector HitActorLocation = CurrentHit.Location;
				DistanceFromFocusToCover = (HitActorLocation-FocusLocation).Size();
			}


			if(CurrentCoverDistanceFromSelf < ShortestDistanceFromSelf && DistanceFromFocusToCover > CurrentCoverDistanceFromSelf) //Want a point that is the closest but closer to us than to the focus
			{
				
				
				ShortestDistanceFromSelf = CurrentCoverDistanceFromSelf;
				BestHit = CurrentHit;
				BestDistanceToCover = CurrentCoverDistanceFromSelf;
			}

		}
		else if(ControllerFocus && DistanceToFocus <= CoverInvalidationDistance) //We have a focus, and they are close 
		{
			if(ClosestCoverHitResult.GetActor() && ClosestCoverHitResult.GetActor() == CurrentHit.GetActor())
			{
				CurrentAngle = CurrentAngle + LineTraceDegrees;
				continue; //Force next iteration of the loop, we want a cover actor that is far away.
			}
			
			float DistanceFromFocusToCover = 0.0f;
			if(ControllerFocus && CurrentHit.GetActor() )
			{
				FVector FocusLocation = ControllerFocus->GetActorLocation();
				FVector HitActorLocation = CurrentHit.Location;
				DistanceFromFocusToCover = (HitActorLocation-FocusLocation).Size();


			}

			//We want to find cover AWAY from the focus. They are too close.
			if(DistanceFromFocusToCover > LongestCoverDistanceFromFocus && DistanceFromFocusToCover >  CurrentCoverDistanceFromSelf + 20.0f) //Want points away from the focus. Give 20.0f leeway
			{
				LongestCoverDistanceFromFocus = DistanceFromFocusToCover;
				BestHit = CurrentHit;
				BestDistanceToCover = LongestCoverDistanceFromFocus;
			}
		
			
		}
		
		CurrentAngle = CurrentAngle + LineTraceDegrees;
	}

	DistanceToCover = BestDistanceToCover;
	ClosestCoverHitResult = BestHit; 
	bIsCoverHitResultValid = IsCoverHitResultValid(); //Only set this to true if we found a valid hit


	

	
}


float UUtilityAIManagerComponent::GetNormalizedStat(const ECurveInputQuery CurveInputQuery) const
{
	float Output = 0.0f;
	
	switch(CurveInputQuery)
	{
		case(ECurveInputQuery::IsCoverSafe):
		{
			Output = bIsCoverPointSafe?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::IsCoverHitResultValid):
		{
			Output = bIsCoverHitResultValid?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::IsCrouch):
		{
			Output = bIsCrouched?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::IsFocusAnyAttack):
		{
			Output = bIsFocusAnyAttacking?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::IsFocusInMeleeRange):
		{
			Output = (DistanceToFocus < MeleeRange)?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::IsFocusInMeleeRangeFallout):
		{
			Output = (DistanceToFocus < MeleeRangeFallout)?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::IsFocusOutOfMeleeRange):
		{
			Output = (DistanceToFocus >= MeleeRange)?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::IsFocusOutOfMeleeRangeFallout):
		{
			Output = (DistanceToFocus >= MeleeRangeFallout)?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::IsFocusMeleeAttack):
		{
			Output = bIsFocusMeleeAttacking?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::IsFocusRangeAttack):
		{
			Output = bIsFocusRangeAttacking?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::IsInCover):
		{
			Output = bIsInCover?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::IsMeleeEquip):
		{
			Output = bMeleeWeaponArmed?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::HasFocus):
		{
			Output = (ControllerFocus != nullptr)?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::HasFocusLastSetPoint):
		{
			Output = (bFocusLastDetectedSet)?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::HasPointOfInterest):
		{
			Output = (bCurrentPointOfInterestSet)?1.0f:0.0f;
			break;
		}
		case(ECurveInputQuery::NormalizedDistanceToCover):
		{
			Output = DistanceToCover/CoverPointSearchDistance;
			break;
		}
		case(ECurveInputQuery::NormalizedDistanceToCurrentPointOfInterest):
		{
			Output = DistanceToCurrentPointOfInterest/MaxDistanceToCurrentPointOfInterest;
			break;
		}
		case(ECurveInputQuery::NormalizedDistanceToFocus):
		{
			Output = DistanceToFocus/ComfortableDistance;
			break;
		}
		case(ECurveInputQuery::NormalizedDistanceToFocusLastDetected):
		{
			Output = DistanceToFocusLastDetectedPoint/MaxFocusSearchDistance;
			break;
		}

	}
	
	return Output; //TODO
}

float UUtilityAIManagerComponent::GetNormalizedStat(const FName& InputStat) const
{
	if(!ControlledPawn)
	{
		return 0.0f;
	}

	if(!(ControlledPawn->GetClass()->ImplementsInterface(UUtilityAIManagerToPawnInterface::StaticClass()) ))
	{
		if(bShowDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("%s doesn't implement IUtilityAIManagerToPawnInterface interface "),*(ControlledPawn->GetFName().ToString() ) )
		}
		
		return 0.0f;
	}

	return 	IUtilityAIManagerToPawnInterface::Execute_GetNormalizedStat(ControlledPawn,InputStat);
}


bool UUtilityAIManagerComponent::IsCoverHitResultValid() const
{

	if (!ClosestCoverHitResult.GetActor())
	{
		return false;
	}
	return true;

}

bool UUtilityAIManagerComponent::IsCoverHitResultSafe() const
{
	if (!ClosestCoverHitResult.GetActor())
	{
		return false;
	}
	else if( (DistanceToFocus < CoverInvalidationDistance && ControllerFocus) || (bFocusLastDetectedSet && !ControllerFocus && DistanceToFocusLastDetectedPoint <  CoverInvalidationDistance))
	{
		return false;
	}


	return true;
}

TMap<int32,UUtilityCombatTaskComponent*> UUtilityAIManagerComponent::ScoreTasks()
{
	TMap<int32,UUtilityCombatTaskComponent*> BestTasks = {};

	TMap<int32,float> RunningScores = {};

	for (UUtilityCombatTaskComponent* Task : TaskArray)
	{
		if(!Task)
		{
			continue;
		}
		int32 CurrentLayer = Task->TaskLayer;
		float TaskScore = Task->CalculateTaskScore(this); //This isn't const for debug purposes, so this method and loop can't be const.
		if(!BestTasks.Contains(CurrentLayer) && TaskScore >= TaskThreshold)
		{
			BestTasks.Add(CurrentLayer,Task);
			RunningScores.Add(CurrentLayer,TaskScore);
		}
		else if(RunningScores.Contains(CurrentLayer) && RunningScores[CurrentLayer] < TaskScore && TaskScore >= TaskThreshold) //NOTE: If two Tasks have the same score, I just don't care and do the first one of score X.
		{
			RunningScores.Add(CurrentLayer,TaskScore);
			BestTasks.Add(CurrentLayer,Task);
		}

	}

	return BestTasks;
}


void UUtilityAIManagerComponent::SetMovementComponentPointers()
{
	if(OwnerController)
	{
		ControlledPawn = OwnerController->GetPawn();
		if(!ControlledPawn)
		{
			return;
		}
	
	PawnMovementComp = ControlledPawn->GetMovementComponent();
	
	CharMC = Cast<UCharacterMovementComponent>(PawnMovementComp);
	
	}
}
