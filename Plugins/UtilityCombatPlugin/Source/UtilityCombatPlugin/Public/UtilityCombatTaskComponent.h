// Copyright Zachary Kolansky, 2020

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UtilityCombatDataStructures.h"
#include "UtilityCombatTaskComponent.generated.h"

class UUtilityAIManagerComponent;
class AAIController;

/*
The Purpose of this component is to encapsulate a specific task that an AI can do.
*/
UCLASS( Blueprintable,BlueprintType,ClassGroup=(UtilityCombat), meta=(BlueprintSpawnableComponent) )
class UTILITYCOMBATPLUGIN_API UUtilityCombatTaskComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUtilityCombatTaskComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	/*
	True if the Task is occuring
	False if the Task isn't occuring

	Used to know whether we need to remove this task from the CurrentTasks Map
	*/
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite,Transient,Category = Control)
	bool bIsTaskActive = false;

	/*
	Right now, I'm going to all the inputs of the graphs are normalized from 0.0f to 1.0f;
	I will also assume that the output of each graph is normalized from 0.0f to 1.0f;
	Key = Stat to query to the UUtilityAIManagerComponent. This float X is used as the input for its Curve
	Value = FUtilityCurveCollection Struct. Consists of a UCurveFloat, and the output of Curve when evaluated at X.
	The CurveOuput will change at runtime.

	The value is a struct thats a Curve and an Output so one can see the evaluation of each curve.
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Control)
	TArray<FUtilityCurveCollection> CurveCollectionArray;


	/*
	If this is true, then this task can only be performed once.
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Flow)
	bool bOnlyDoTaskOnce = false;

	/*
	Set to true if ExitTask() is called.
	*/
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite,Transient, Category = Flow)
	bool bPeformedTask = false;

	/*
	If this is true, this task can't begin.
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Flow)
	bool bTaskLocked = false;


	/*
	If true, we will Randomly change the CurrentCooldown to be between MinCooldown and MaxCooldown after we EnterTask()
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Cooldown)
	bool bSetCurrentCooldownBetweenMinMax = false;

	/*
	Time in seconds that must pass until the task can fire again.
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Cooldown)
	float CurrentCooldown = 0.0f;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Cooldown)
	float MinCooldown = 0.0f;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Cooldown)
	float MaxCooldown = 0.0f;

	/*
	Time in world seconds when the task begins. (GetWorld()->GetTimeSeconds();)
	*/
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite,Transient,Category = Cooldown)
	float WorldTimeBegun = -1.0f;

	/*
	Time in world seconds when the task ends. (GetWorld()->GetTimeSeconds();)
	*/
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite,Transient,Category = Cooldown)
	float WorldTimeEnd = -1.0f;
	
		
	/*
	Stores last value from CalculateTaskScore()
	*/
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite,Transient, Category = Task)
	float FinalNormalizedUtilityValue = 0.0f;

	/*
	We can do multiple tasks at one time. Tasks on TaskLayer 1 can occur at the same time as tasks on TaskLayer 2.
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Transient, Category = Task)
	int32 TaskLayer = 0;

	/*
	Give each task a Unique name, because the TaskName will be passed into a Delegate whenever tasks Begin or End.
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Task)
	FName TaskName = FName("");

	/*
	Always = This task can always be interrupted by another task. Interruption will occur when a better task to do is found.
	Never = This task can never be interrupted by another task, no matter what.
	Priority = This task can only be interrupted by tasks with a greater priority number.
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Interruption)
	EUtilityInterruptionType InterruptType = EUtilityInterruptionType::ALWAYS;

	/*
	Controls what tasks can interrupt this task. Is relevant only when 
	InterruptType = EUtilityInterruptionType::PRIORITY.
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = Interruption)
	int32 InterruptionPriorityNumber = 1;


	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = Owner)
	UUtilityAIManagerComponent* CurrentManagerComponent = nullptr;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category = Owner)
	AAIController* OwnerController = nullptr;


	UFUNCTION(BlueprintCallable,BlueprintNativeEvent, Category = Basic)
	float CalculateTaskScore(UUtilityAIManagerComponent* ManagerComponent);

	/*
	Returns true if this task can be interrupted by InterruptingTask
	Returns false if this task cannot be interrupted by InterruptingTask
	*/
	UFUNCTION(BlueprintPure, Category = Query)
	bool CanTaskBeInterrupted(UUtilityCombatTaskComponent* InterruptingTask);

	/*
	This is called whenever we begin this task
	*/
	UFUNCTION(BlueprintCallable,BlueprintNativeEvent, Category = Basic)
	void EnterTask();

	/*
	This is called whenever we exit this task
	*/
	UFUNCTION(BlueprintCallable,BlueprintNativeEvent, Category = Basic)
	void ExitTask();

	/*
	Returns true if the task is not on Cooldown
	Returns False if the task is on Cooldown
	*/
	UFUNCTION(BlueprintPure, Category = Query)
	bool IsTaskReady();

	




};
