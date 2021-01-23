// Copyright Zachary Kolansky, 2020


#include "StatManager.h"
#include "StatusEffectComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
UStatManager::UStatManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

}


// Called when the game starts
void UStatManager::BeginPlay()
{
	Super::BeginPlay();
	ReadStatDataTable();	
}

void UStatManager::EndPlay(const EEndPlayReason::Type EndPlayReason) 
{
	GetWorld()->GetTimerManager().ClearTimer(ModifyStatRPCHandle);

	Super::EndPlay(EndPlayReason);
}

void UStatManager::GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UStatManager,bStatDictionaryCanModifyOtherStatDictionary);
	DOREPLIFETIME(UStatManager,OtherStatManagers);
	//DOREPLIFETIME(UStatManager,EffectComponentList);
	
}


// Called every frame
void UStatManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


//IMPORTANT

void UStatManager::ForceUpdateUIMulticast_Implementation()
{
	OnDataTableInitialized.Broadcast();
}


//STAT


void UStatManager::AddStat(FName StatName, const FStat& Stat )
{
	AActor* OwnerActor = GetOwner();

	if(!OwnerActor)
	{
		return;
	}


	if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy )
	{
		AddStatServer(StatName,Stat); 
		
	}
	else if(OwnerActor->HasAuthority())
	{
		AddStatServer(StatName,Stat); 
	}
	
		
}

void UStatManager::AddStatMulticast_Implementation(FName StatName, const FStat& Stat )
{
	StatDictionary.Add(StatName,Stat); //Added to either the server, or the client
}

void UStatManager::AddStatServer_Implementation(FName StatName, const FStat& Stat )
{
	AddStatMulticast(StatName,Stat);
}


void UStatManager::ModifyStat(const FName StatName, const float Value,  const EStatValueType ValueType, const EStatModificationOperation StatOperation)
{

	if(!StatDictionary.Contains(StatName))
	{
		return;
	}
	
	AActor* OwnerActor = GetOwner();

	if(!OwnerActor)
	{
		return;
	}

	if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy )
	{
		ModifyStatServer(StatName,Value,ValueType,StatOperation); 
	}
	else if(OwnerActor->HasAuthority())
	{
		ModifyStatServer(StatName,Value,ValueType,StatOperation); 
	}


	




	



}

void UStatManager::ModifyStatMulticast_Implementation(const FName StatName, const float Value,  const EStatValueType ValueType, const EStatModificationOperation StatOperation)
{

	FStat Stat = *StatDictionary.Find(StatName);

	/*
	Need to get the net effect of all  the status conditions, then zero them, and add those values to the current value.
	This ensures the stat is bounded, then adjusted so addition leads to a CurrentValue that will always be greater than the minimum, and subtraction 
	likewise will always lead to a value that is less than the maximum.  


	However, bindings  will cause artificial min/max.
	*/
	float EffectTotal = GetEffectorTotal(StatName,true); 
	Stat = Stat + EffectTotal;

	switch(StatOperation)
	{
		case EStatModificationOperation::Addition:
		{
			Stat.AddValue(Value,ValueType);
			break;
		}
		case EStatModificationOperation::Subtraction:
		{
			Stat.SubtractValue(Value,ValueType);
			break;
		}
		case EStatModificationOperation::Multiplication:
		{
			Stat.MultiplyValue(Value,ValueType);
			break;
		}
		case EStatModificationOperation::Division:
		{
			Stat.DivideValue(Value,ValueType);
			break;
		}
		case EStatModificationOperation::Replacement:
		{
			Stat.ReplaceValue(Value,ValueType);
			break;
		}

	}


	StatDictionary.Add(StatName,Stat);

	OnStatModified.Broadcast(StatName,Stat);

}

void UStatManager::ModifyStatServer_Implementation(const FName StatName, const float Value,  const EStatValueType ValueType, const EStatModificationOperation StatOperation)
{
	ModifyStatMulticast(StatName,Value,ValueType,StatOperation);
}

void UStatManager::ModifyStatManyModifications(const FName StatName, const float Value,  const EStatValueType ValueType, const EStatModificationOperation StatOperation, float TimeToCollectModifications)
{
	if(TimeToCollectModifications <= 0.0f)
	{
		TimeToCollectModifications = 0.2f;
	}
	
	//Prepare to flush collected operations
	if(!ModifyStatRPCTimer.IsBound())
	{
		ModifyStatRPCTimer.BindUFunction(this,FName("ModifyStatByArray"));
	}

	FTimerManager* WorldTimerManager = &(GetWorld()->GetTimerManager());


	if(WorldTimerManager && !WorldTimerManager->IsTimerActive(ModifyStatRPCHandle))
	{
		WorldTimerManager->SetTimer(ModifyStatRPCHandle,ModifyStatRPCTimer,TimeToCollectModifications,false,TimeToCollectModifications);
	}
	
	FStatOperation NewOperation = FStatOperation(StatName,Value,ValueType,StatOperation);

	
	int32 OutIndex = 0;
	//Remember the most recent stat modification operations.
	if(StatOperations.Find(NewOperation,OutIndex))
	{
		FStatOperation CollectedOperation = StatOperations[OutIndex];
		CollectedOperation = CollectedOperation + NewOperation;
		StatOperations[OutIndex] = CollectedOperation;
	}
	else
	{
		StatOperations.Add(NewOperation);
	}
	

}

void UStatManager::ModifyStatByArray()
{
	AActor* OwnerActor = GetOwner();

	if(!OwnerActor || StatOperations.Num() == 0 )
	{
		return;
	}


	if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy )
	{
		ModifyStatByArrayServer(StatOperations); 
	}
	else if(OwnerActor->HasAuthority())
	{
		ModifyStatByArrayMulticast(StatOperations); 
		
	}

	TimeLastCalledCollectionRPC = GetWorld()->GetTimeSeconds();

}

void UStatManager::ModifyStatByArrayMulticast_Implementation(const TArray<FStatOperation>& StatOperationArray)
{	
	
	for (const FStatOperation& StatOperation : StatOperationArray)
	{
		//ModifyStatMulticast(StatOperation.StatName,StatOperation.Value,StatOperation.ValueType,StatOperation.StatOperation);

		FStat Stat = *StatDictionary.Find(StatOperation.StatName);


		float EffectTotal = GetEffectorTotal(StatOperation.StatName,true); 
		Stat = Stat + EffectTotal;

		switch(StatOperation.StatOperation)
		{
			case EStatModificationOperation::Addition:
			{
				Stat.AddValue(StatOperation.Value,StatOperation.ValueType);
				break;
			}
			case EStatModificationOperation::Subtraction:
			{
				Stat.SubtractValue(StatOperation.Value,StatOperation.ValueType);
				break;
			}
			case EStatModificationOperation::Multiplication:
			{
				Stat.MultiplyValue(StatOperation.Value,StatOperation.ValueType);
				break;
			}
			case EStatModificationOperation::Division:
			{
				Stat.DivideValue(StatOperation.Value,StatOperation.ValueType);
				break;
			}
			case EStatModificationOperation::Replacement:
			{
				Stat.ReplaceValue(StatOperation.Value,StatOperation.ValueType);
				break;
			}

		}


		StatDictionary.Add(StatOperation.StatName,Stat);

		OnStatModified.Broadcast(StatOperation.StatName,Stat);

	}
	ForceUpdateUIMulticast();
	GetWorld()->GetTimerManager().ClearTimer(ModifyStatRPCHandle);
	StatOperations.Empty();

	
	
	
}

void UStatManager::ModifyStatByArrayServer_Implementation(const TArray<FStatOperation>& StatOperationArray)
{
	ModifyStatByArrayMulticast(StatOperationArray);
}


void UStatManager::RemoveStat(FName StatName)
{
	AActor* OwnerActor = GetOwner();

	if(!OwnerActor)
	{
		return;
	}
	

	

	if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy )
	{
		RemoveStatServer(StatName); 
		
	}
	else if(OwnerActor->HasAuthority())
	{
		RemoveStatMulticast(StatName);
	}
	
}

void UStatManager::RemoveStatMulticast_Implementation(FName StatName)
{
	StatDictionary.Remove(StatName);
}


void UStatManager::RemoveStatServer_Implementation(FName StatName)
{
	RemoveStatMulticast(StatName);
}



//STAT BINDING


void UStatManager::AddStatBinding(FName BoundStat, const FStatBind& NewStatBinding)
{
	AActor* OwnerActor = GetOwner();

	if(!OwnerActor)
	{
		return;
	}

	if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy )
	{
		AddStatBindingServer(BoundStat,NewStatBinding); 
		
	}
	else if(OwnerActor->HasAuthority())
	{
		AddStatBindingServer(BoundStat,NewStatBinding); 
	}
}


void UStatManager::AddStatBindingMulticast_Implementation(FName BoundStat, const FStatBind& NewStatBinding)
{
	StatBindings.Add(BoundStat,NewStatBinding);
}


void UStatManager::AddStatBindingServer_Implementation(FName BoundStat, const FStatBind& NewStatBinding)
{
	AddStatBindingMulticast(BoundStat,NewStatBinding);
}


void UStatManager::RemoveStatBinding(FName StatName)
{
	AActor* OwnerActor = GetOwner();

	if(!OwnerActor)
	{
		return;
	}
	

	

	if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy )
	{
		RemoveStatBindingServer(StatName); 
	}
	else if(OwnerActor->HasAuthority())
	{
		RemoveStatBindingServer(StatName);
	}
}


void UStatManager::RemoveStatBindingMulticast_Implementation(FName StatName)
{
	StatBindings.Remove(StatName);
}


void UStatManager::RemoveStatBindingServer_Implementation(FName StatName)
{
	RemoveStatBindingMulticast(StatName);
}


void UStatManager::SetStatBindings( const TMap<FName,FStatBind>& InputStatBindings)
{
	AActor* OwnerActor = GetOwner();

	if(!OwnerActor)
	{
		return;
	}
	

	

	if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy )
	{
		SetStatBindingsServer(InputStatBindings); 
	}
	else if(OwnerActor->HasAuthority())
	{
		SetStatBindingsMulticast(InputStatBindings);
	}
}


void UStatManager::SetStatBindingsMulticast_Implementation(const TMap<FName,FStatBind>& InputStatBindings)
{
	StatBindings = InputStatBindings;
}


void UStatManager::SetStatBindingsServer_Implementation(const TMap<FName,FStatBind>& InputStatBindings)
{
	SetStatBindingsMulticast(InputStatBindings);
}

//STATUS

void UStatManager::ApplyStatusEffectMulticast_Implementation(const  FStatusEffect StatusEffect,float Effector )
{
	
	
	if(EffectorDictionary.Contains(StatusEffect.ActionName))
	{
		float CurrentEffector = EffectorDictionary[StatusEffect.ActionName];
		float NewEffect =  Effector + CurrentEffector;
		EffectorDictionary[StatusEffect.ActionName] = NewEffect;
	}
	else
	{
		EffectorDictionary.Add(StatusEffect.ActionName,Effector);
	}
	
	OnStatApplied.Broadcast(StatusEffect,Effector); 


}


void UStatManager::ClearEffectMulticast_Implementation(const FName EffectName,const FStatusEffect StatusEffect)
{
	OnEffectClear.Broadcast(EffectName,StatusEffect);
}

void UStatManager::ClearEffects(EStatusClearCondition ClearCondition)
{
	
	AActor* OwnerActor = GetOwner();
	if(!OwnerActor)
	{
		return;
	}
	
	if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy )
	{
		ClearEffectsServer(ClearCondition); 
		
	}
	else if(OwnerActor->HasAuthority())
	{
		ClearEffectsMulticast(ClearCondition); 
	}
	
	

}

void UStatManager::ClearEffectsMulticast_Implementation(EStatusClearCondition ClearCondition)
{
	for(UStatusEffectComponent* EC : EffectComponentList)
	{
		if(EC)
		{
			if(EC->StatusEffect.ClearConditions.Contains(ClearCondition))
			{
				EC->ClearEffect();
				OnEffectClear.Broadcast(EC->EffectName,EC->StatusEffect);
			}

		}
	}
}


void UStatManager::ClearEffectsServer_Implementation(EStatusClearCondition ClearCondition)
{
	ClearEffectsMulticast(ClearCondition);//Will ensure ClearEffects() is called on the server.
}



void UStatManager::InitializeEffect(const FName EffectName, const FStatusEffect& EffectToInitalize, float StartEffector, float EffectSaveTime, float EffectStartTime)
{

	if(EffectToInitalize.EffectAction != EEffectAction::ModifyStat)
	{
		return; 
	}

	
	if(EffectToInitalize.Duration == 0 || EffectToInitalize.TickTime <= 0) 
	{
		return; //Invalid status effect
	}


	AActor* OwnerActor = GetOwner();
	if(!OwnerActor)
	{
		if(bShowDebugWarnings)
		{
			UE_LOG(LogTemp,Error,TEXT("InitializeEffect()  for Effect %s failed, GetOwner() returned nullptr"),*(EffectToInitalize.ActionName.ToString()))
		}	
		
		return;
	}
	


	if(OwnerActor)
	{
		if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy)
		{
			InitializeEffectServer(EffectName, EffectToInitalize,StartEffector,EffectSaveTime,EffectStartTime); 
		}
		else if (OwnerActor->HasAuthority() )
		{
			InitializeEffectServer(EffectName, EffectToInitalize,StartEffector,EffectSaveTime,EffectStartTime); 
		}
	}

	



}

void UStatManager::InitializeEffectMulticast_Implementation(const FName EffectName, const  FStatusEffect EffectToInitalize)
{
	OnEffectInitialize.Broadcast(EffectName,EffectToInitalize);
}

void UStatManager::InitializeEffectServer_Implementation(const FName EffectName,const  FStatusEffect EffectToInitalize,float StartEffector, float EffectSaveTime, float EffectStartTime)
{
	AActor* OwnerActor = GetOwner();
	int32 EffectNum = EffectComponentList.Num();
	FString EffectAsString = EffectToInitalize.ActionName.ToString();
	EffectAsString.AppendInt(EffectNum);
	FName FinalName = FName(*EffectAsString);
	
	UStatusEffectComponent* NewStatusEffectComponent = NewObject<UStatusEffectComponent>(this,UStatusEffectComponent::StaticClass(),FinalName);

	
	if(OwnerActor && NewStatusEffectComponent)
	{
		NewStatusEffectComponent->RegisterComponentWithWorld(GetWorld());
		
		NewStatusEffectComponent->InitializeStatusEffectComponent(this,EffectName,EffectToInitalize,StartEffector,EffectSaveTime,EffectStartTime);
		OwnerActor->AddInstanceComponent(NewStatusEffectComponent);

		
		
		EffectComponentList.Add(NewStatusEffectComponent);
		EffectComponentList.Shrink(); //Least likely place to cause threading issues
	}
	else
	{
		if(bShowDebugWarnings)
		{
			UE_LOG(LogTemp,Error,TEXT("InitializeEffect()  for Effect %s failed, couldn't create component"),*(EffectToInitalize.ActionName.ToString()))
		}
	}
	
	InitializeEffectMulticast(EffectName,EffectToInitalize); 
}

void UStatManager::InitializeEffectArray( const TArray<FName>& EffectNames, const TArray<FStatusEffect>& EffectArray)
{
	if(EffectArray.Num() != EffectNames.Num())
	{
		if(bShowDebugWarnings)
		{
			UE_LOG(LogTemp,Error,TEXT("Arrays must be the same length"))
		}
		
		return;
	}
	
	int32 i = 0;
	for (FStatusEffect E : EffectArray)
	{
		InitializeEffect(EffectNames[i],E);
		i++;
	}

}


void UStatManager::PauseEffectByName(const FName EffectName)
{
	AActor* OwnerActor = GetOwner();
	if(!OwnerActor)
	{
		return;
	}
	
	if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy )
	{
		PauseEffectByNameServer(EffectName); 
		
	}
	else if(OwnerActor->HasAuthority())
	{
		PauseEffectByNameServer(EffectName); 
	}
}


void UStatManager::PauseEffectByNameMulticast_Implementation(const FName EffectName,const  FStatusEffect StatusEffect)
{
	OnEffectPause.Broadcast(EffectName,StatusEffect);
}


void UStatManager::PauseEffectByNameServer_Implementation(const FName EffectName)
{
	
	for(UStatusEffectComponent* EC : EffectComponentList)
	{
		if(EC)
		{
			if(EC->EffectName == EffectName)
			{
				EC->PauseEffect();
				PauseEffectByNameMulticast(EC->EffectName,EC->StatusEffect);
			}

		}
	}
}

void UStatManager::ResumeEffectByName(const FName EffectName)
{
	AActor* OwnerActor = GetOwner();
	if(!OwnerActor)
	{
		return;
	}
	
	if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy )
	{
		ResumeEffectByNameServer(EffectName); 
		
	}
	else if(OwnerActor->HasAuthority())
	{
		ResumeEffectByNameServer(EffectName); 
	}
}


void UStatManager::ResumeEffectByNameMulticast_Implementation(const FName EffectName,const  FStatusEffect StatusEffect)
{
	OnEffectResume.Broadcast(EffectName,StatusEffect);
}

void UStatManager::ResumeEffectByNameServer_Implementation(const FName EffectName)
{
	for(UStatusEffectComponent* EC : EffectComponentList)
	{
		if(EC)
		{
			if(EC->EffectName == EffectName)
			{
				EC->ResumeEffect();
				ResumeEffectByNameMulticast(EC->EffectName,EC->StatusEffect);
			}

		}
	}
}



//STAT QUERY

float UStatManager::GetCurrentValueRaw(const FName StatName) const
{
	if(!StatDictionary.Contains(StatName))
	{
		return 0.0f;
	}
	FStat Stat = *StatDictionary.Find(StatName);
	return Stat.CurrentValue;
}


float UStatManager::GetEffectorTotal(const FName StatName,bool ZeroFoundEffectors)
{
	float Output = 0.0f;

	if(EffectorDictionary.Contains(StatName))
	{
		Output = EffectorDictionary[StatName];
		if(ZeroFoundEffectors)
		{
			EffectorDictionary[StatName] = 0.0f;
		}
	}
	return Output;
	

	/*
	ONLY WORKS SERVER SIDE
	float Sum = 0.0f;
	for(UStatusEffectComponent* EC : EffectComponentList)
	{
		if(EC)
		{
			if(EC->StatusEffect.ActionName == StatName)
			{
				float Value = EC->Effector;
				Sum = Sum + Value;
				if(ZeroFoundEffectors)
				{
					EC->ZeroEffector();//Reset the effector, going to alter the Current value of the given stat in StatDictionary
				}
				
			}
		}
	}
	return Sum;
	*/
}


float UStatManager::GetStatTotal(const FName StatName, bool bUseRawForSelf)
{
	float Sum = 0.0f;
	for ( UStatManager* OtherStatComponent : OtherStatManagers)
	{
		if(OtherStatComponent) //Check pointer
		{
			if(OtherStatComponent->bStatDictionaryCanModifyOtherStatDictionary) 
			{
				float Value = OtherStatComponent->GetStatValue(StatName);
				Sum += Value;
			}
		}

	}
	
	if(bUseRawForSelf)
	{
		Sum += GetCurrentValueRaw(StatName);
	}
	else
	{
		Sum += GetStatValue(StatName);
	}
	return Sum;
}


float UStatManager::GetStatValue(const FName StatName)
{
	
	return GetStatValueAsStat(StatName).CurrentValue;
}



FStat UStatManager::GetStatValueAsStat(const FName StatName)
{
	
	bool bStatInDic = false;
	FStat Stat = FStat(); //CurrentValue = 0

	if(StatDictionary.Contains(StatName))
	{
		bStatInDic = true; //don't clamp if we don't have a stat
		Stat = *StatDictionary.Find(StatName);
	}
	 
	
	float EffectTotal = GetEffectorTotal(StatName); //Status Effects

	bool bHasBinding = false;
	FStatBind StatBind = FStatBind();

	if(StatBindings.Contains(StatName))
	{
		StatBind = StatBindings[StatName];
		bHasBinding = true;


	}
	



	if(!bStatInDic) 
	{
		//Best way to handle stat not found in the StatDictionary, but in other places
		Stat = FStat(EffectTotal,EffectTotal,EffectTotal*2);

	
		if(bHasBinding && StatDictionary.Contains(StatBind.NameOfStatBindingModifier))
		{
			// stat that will modify Stat based on the rules defined in StatBindings
			FStat Modifier = StatDictionary[StatBind.NameOfStatBindingModifier];
			StatBind.CalculateNewStat(Stat,Modifier); //passed by reference, 

		}

		return Stat;
	}
	else
	{
		Stat = Stat + EffectTotal;

		if(bHasBinding && StatDictionary.Contains(StatBind.NameOfStatBindingModifier))
		{
			// stat that will modify Stat based on the rules defined in StatBindings
			FStat Modifier = StatDictionary[StatBind.NameOfStatBindingModifier];
			StatBind.CalculateNewStat(Stat,Modifier); //passed by reference, 

		}
		
	}
	return Stat;
}

	
FStat  UStatManager::GetStatTotalAsStat(const FName StatName,bool bUseRawForSelf)
{
	FStat TotalStat = FStat();
	float Sum = 0.0f;
	for ( UStatManager* OtherStatComponent : OtherStatManagers)
	{
		if(OtherStatComponent) //Check pointer
		{
			if(OtherStatComponent->bStatDictionaryCanModifyOtherStatDictionary) 
			{
				float Value = OtherStatComponent->GetStatValue(StatName);
				Sum += Value;
			}
		}

	}
	
	if(bUseRawForSelf)
	{
		TotalStat = GetRawStat(StatName);
	}
	else
	{
		TotalStat = GetStatValueAsStat(StatName);
	}
	TotalStat = TotalStat + Sum;
	return TotalStat;
}


FStat UStatManager::GetRawStat(const FName StatName) const
{
	if(!StatDictionary.Contains(StatName))
	{
		return FStat();
	}
	FStat Stat = *StatDictionary.Find(StatName);
	return Stat;
}

float UStatManager::GetRawStatTotal(const FName StatName, bool bUseRawForSelf)
{
	float Sum = 0.0f;
	for ( UStatManager* OtherStatComponent : OtherStatManagers)
	{
		if(OtherStatComponent) //Check pointer
		{
			if(OtherStatComponent->bStatDictionaryCanModifyOtherStatDictionary) 
			{
				float Value = OtherStatComponent->GetCurrentValueRaw(StatName);
				Sum += Value;
			}
		}

	}

	if(bUseRawForSelf)
	{
		Sum += GetCurrentValueRaw(StatName);
	}
	else
	{
		Sum += GetStatValue(StatName);
	}
	
	
	return Sum;
}



bool UStatManager::IsStatGreaterThan(const FName StatName, const float Value)
{
	if(!StatDictionary.Contains(StatName))
	{
		return false;
	}

	FStat Stat = *StatDictionary.Find(StatName);
	return (Stat > Value);
}


bool UStatManager::IsStatGreaterThanOrEqualTo(const FName StatName, const float Value)
{
	if(!StatDictionary.Contains(StatName))
	{
		return false;
	}

	FStat Stat = *StatDictionary.Find(StatName);
	return (Stat >= Value);
}


bool UStatManager::IsStatLessThan(const FName StatName, const float Value)
{
	if(!StatDictionary.Contains(StatName))
	{
		return false;
	}

	FStat Stat = *StatDictionary.Find(StatName);
	return (Stat < Value);
}


bool UStatManager::IsStatLessThanOrEqualTo(const FName StatName, const float Value)
{
	if(!StatDictionary.Contains(StatName))
	{
		return false;
	}

	FStat Stat = *StatDictionary.Find(StatName);
	return (Stat <= Value);
}


bool UStatManager::IsStatEqualTo(const FName StatName, const float Value)
{
	if(!StatDictionary.Contains(StatName))
	{
		return false;
	}

	FStat Stat = *StatDictionary.Find(StatName);
	return (Stat == Value);
}

bool UStatManager::IsStatNotEqualTo(const FName StatName, const float Value)
{
	if(!StatDictionary.Contains(StatName))
	{
		return false;
	}

	FStat Stat = *StatDictionary.Find(StatName);
	return (Stat != Value);
}

//UTILITY

FStatusEffect UStatManager::ReadEffectData(const FName EffectToRead,bool& bValidData, bool bDebugWarnings)
{
	
	FStatusEffect StatusEffect = FStatusEffect();

	if(!EffectData)
	{
		if(bDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("Data table given is not valid, returning blank struct"))
		}
		bValidData = false;
		return StatusEffect;
	}


	/*
	Reach here if the given structure Name Matches the data table AND the table is not a nu/lptr
	*/
	TArray<FName> InputDataTableRowName = EffectData->GetRowNames();
	if(!InputDataTableRowName.Contains(EffectToRead))
	{
		if(bDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("The given EffectToRead %s was not found in the EffectData. Returning blank template object"), *(EffectToRead.ToString()))
		}
		bValidData = false;
		
		return StatusEffect;
	}
	//Reach here when we know the row to read is in the data table
	FStatusEffect* PointerOfTypeT = EffectData->FindRow<FStatusEffect>(EffectToRead, FString(""));
	StatusEffect = *PointerOfTypeT; //Deference

	bValidData = true;
	return StatusEffect;
	
}

TArray<FStatusEffect> UStatManager::ReadEffects(const TArray<FName>& EffectsToRead)
{
	
	TArray<FStatusEffect> StatusArray = {};


	if(!EffectData)
	{
		return StatusArray;
	}
	

	for (FName Row : EffectsToRead )
	{
		bool bValidData = false;
		FStatusEffect Data = ReadEffectData(Row,bValidData,bShowDebugWarnings);
		if(bValidData)
		{
			StatusArray.Add(Data);
		}
	}
	

	return StatusArray;
}

bool UStatManager::ReadStatDataTable()
{
	if(!StatDataTable)
	{
		if(bShowDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("StatDataTable inReadStatDataTable() is not valid, stats won't be read!"))
		}
		return false;
	}
	
	TArray<FName> InputDataTableRowNames = StatDataTable->GetRowNames();

	for (FName Row : InputDataTableRowNames)
	{
		FStat* PointerOfTypeT = StatDataTable->FindRow<FStat>(Row, FString(""));
		FStat StructObject = *PointerOfTypeT; //Deference
		StatDictionary.Add(Row,StructObject);

	}


	OnDataTableInitialized.Broadcast();
	return true;
}


//DICTIONARY

void UStatManager::EmptyDictionaries(bool bEmptyStatDictionary, bool bEmptyStatBindingDictionary, bool bEmptyEffectorDictionary)
{
	AActor* OwnerActor = GetOwner();

	if(!OwnerActor)
	{
		return;
	}
	

	

	if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy )
	{
		EmptyDictionariesServer(bEmptyStatDictionary,bEmptyStatBindingDictionary,bEmptyEffectorDictionary); 
	}
	else if(OwnerActor->HasAuthority())
	{
		EmptyDictionariesServer(bEmptyStatDictionary,bEmptyStatBindingDictionary,bEmptyEffectorDictionary);
	}
}


void UStatManager::EmptyDictionariesMulticast_Implementation(bool bEmptyStatDictionary, bool bEmptyStatBindingDictionary, bool bEmptyEffectorDictionary)
{
	if(bEmptyStatDictionary)
	{
		StatDictionary.Empty(0);
	}

	if(bEmptyStatBindingDictionary)
	{
		StatBindings.Empty(0);
	}

	if(bEmptyEffectorDictionary)
	{
		EffectorDictionary.Empty(0);
	}
}


void UStatManager::EmptyDictionariesServer_Implementation(bool bEmptyStatDictionary, bool bEmptyStatBindingDictionary, bool bEmptyEffectorDictionary)
{
	EmptyDictionariesMulticast(bEmptyStatDictionary,bEmptyStatBindingDictionary,bEmptyEffectorDictionary);
}




void UStatManager::SetStatDictionary(const TMap<FName,FStat>& InputStatDictionary)
{
	AActor* OwnerActor = GetOwner();

	if(!OwnerActor)
	{
		return;
	}
	

	

	if(!OwnerActor->HasAuthority() && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy )
	{
		SetStatDictionaryServer(InputStatDictionary); 
	}
	else if(OwnerActor->HasAuthority())
	{
		SetStatDictionaryMulticast(InputStatDictionary);
	}
}	


void UStatManager::SetStatDictionaryMulticast_Implementation(const TMap<FName,FStat>& InputStatDictionary)
{
	StatDictionary = InputStatDictionary;
}


void UStatManager::SetStatDictionaryServer_Implementation(const TMap<FName,FStat>& InputStatDictionary)
{
	SetStatDictionaryMulticast(InputStatDictionary);
}




TArray<FStatusEffectSaveData> UStatManager::GenerateStatusEffectSaveData() const
{
	TArray<FStatusEffectSaveData> SaveData = {};
	float SaveTime = GetWorld()->GetTimeSeconds();

	for(UStatusEffectComponent* EC : EffectComponentList)
	{
		if(!EC)
		{
			continue;
		}


		FStatusEffectSaveData NewSaveData = FStatusEffectSaveData(EC->StatusEffect,EC->Effector,EC->EffectName, EC->EffectStartTime,SaveTime);
		SaveData.Add(NewSaveData);

	}



	return SaveData;
}





void UStatManager::LoadSaveData(const TMap<FName,FStat>& InputStatDictionary,const TMap<FName,FStatBind>& InputStatBindings, const TArray<FStatusEffectSaveData>& InputSavedStatusEffectData)
{
	EmptyDictionaries(true,true,true);
	SetStatDictionary(InputStatDictionary);
	SetStatBindings(InputStatBindings);

	for (const FStatusEffectSaveData& SESD : InputSavedStatusEffectData)
	{
		InitializeEffect(SESD.EffectName,SESD.EffectData,SESD.Effector,SESD.EffectSaveTime,SESD.EffectStartTime);
	}
}



