// Copyright Zachary Kolansky, 2020


#include "UtilityCombatTaskComponent.h"
#include "UtilityAIManagerComponent.h"
#include "Math/UnrealMathUtility.h"
#include "AIController.h"


// Sets default values for this component's properties
UUtilityCombatTaskComponent::UUtilityCombatTaskComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UUtilityCombatTaskComponent::BeginPlay()
{
	Super::BeginPlay();

	//Handle bad parameter values.
	if(MinCooldown < 0.0f)
	{
		MinCooldown = 0.0f;
	}

	if(MaxCooldown < 0.0f)
	{
		MaxCooldown = 0.0f;
	}
	
}


// Called every frame
void UUtilityCombatTaskComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


float UUtilityCombatTaskComponent::CalculateTaskScore_Implementation(UUtilityAIManagerComponent* ManagerComponent)
{
	CurrentManagerComponent = ManagerComponent;
	if(!ManagerComponent || !IsTaskReady())
	{
		return 0.0f; //Score of 0.0f if we aren't ready OR we were never given a manager component.
	}

	TArray<FName> CollectionKeys = {};

	float RunningNormalizedUtilityValue = 1.0f; //Multiple the output of each graph

	for (FUtilityCurveCollection& CurveCollection : CurveCollectionArray)
	{
		float NormalizedCurveOutput = 1.0f;
		float Dampen = 1.0f;

		if(CurveCollection.CurveInputQuery == ECurveInputQuery::STAT_BY_FNAME)
		{
			float CurveTime = ManagerComponent->GetNormalizedStat(CurveCollection.StatName);
			if(CurveCollection.CurveFloat)
			{
				NormalizedCurveOutput = CurveCollection.CurveFloat->GetFloatValue(CurveTime);
				CurveCollection.CurveOutput = NormalizedCurveOutput*CurveCollection.CurveDampen;
				Dampen = CurveCollection.CurveDampen;

			}
			else
			{
				NormalizedCurveOutput = 0.0f;
			}
			
		}
		else
		{
			float CurveTime = ManagerComponent->GetNormalizedStat(CurveCollection.CurveInputQuery);
			if(CurveCollection.CurveFloat)
			{
				NormalizedCurveOutput = CurveCollection.CurveFloat->GetFloatValue(CurveTime);
				CurveCollection.CurveOutput = NormalizedCurveOutput*CurveCollection.CurveDampen;
				Dampen = CurveCollection.CurveDampen;	
			}
			else
			{
				NormalizedCurveOutput = 0.0f;
			}
		}

		if(CurveCollection.bMultiplyThisCurveOutputToRunningTotal)
		{
			RunningNormalizedUtilityValue = RunningNormalizedUtilityValue*NormalizedCurveOutput*Dampen;
		}
		else
		{
			RunningNormalizedUtilityValue = RunningNormalizedUtilityValue+NormalizedCurveOutput*Dampen;
		}
		

	
	}

	FinalNormalizedUtilityValue = RunningNormalizedUtilityValue;
	return RunningNormalizedUtilityValue;
}

bool UUtilityCombatTaskComponent::CanTaskBeInterrupted(UUtilityCombatTaskComponent* InterruptingTask)
{
	bool bCanInterrupt = false;
	
	switch(InterruptType)
	{
		case EUtilityInterruptionType::NEVER:
			break;
		case EUtilityInterruptionType::ALWAYS:
			bCanInterrupt = true;
			break;
		case EUtilityInterruptionType::PRIORITY:
			if(InterruptingTask && InterruptingTask->InterruptionPriorityNumber > InterruptionPriorityNumber)
			{
				bCanInterrupt = true;
			}
			break;
			


	}


	return bCanInterrupt;
}

void UUtilityCombatTaskComponent::EnterTask_Implementation()
{
	if(bSetCurrentCooldownBetweenMinMax)
	{
		CurrentCooldown = FMath::FRandRange(MinCooldown,MaxCooldown);
	}

	WorldTimeBegun = GetWorld()->GetTimeSeconds();
	bIsTaskActive = true;
}

void UUtilityCombatTaskComponent::ExitTask_Implementation()
{
	WorldTimeEnd = GetWorld()->GetTimeSeconds();
	bPeformedTask = true;
	bIsTaskActive = false;
}

bool UUtilityCombatTaskComponent::IsTaskReady()
{	
	if( (bPeformedTask && bOnlyDoTaskOnce) || bTaskLocked)
	{
		return false; 
	}

	if(CurrentCooldown <= 0.0f || WorldTimeBegun <= 0.0f)
	{
		return true; //Either Cooldown is none, or we haven't done the task yet.
	}
	
	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	
	if(CurrentTime - WorldTimeBegun >= CurrentCooldown)
	{
		return true;
	}
		


	return false;
}