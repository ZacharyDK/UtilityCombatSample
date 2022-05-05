// Copyright Zachary Kolansky, 2020

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatDataStructures.h"
#include "StatManager.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDataTableInitialized);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStatModified, FName, StatName, FStat, Stat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEffectStateChange, FName, EffectName, FStatusEffect, StatusEffect);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStatApplied, FStatusEffect, StatusEffect, float, Effector );


class UStatusEffectComponent;

/*
Data collection for all the values needed 
to add some some FStats
*/
USTRUCT(BlueprintType)
struct FStatOperation
{
	GENERATED_BODY();
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = StatOperation)
	FName StatName = FName("");

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = StatOperation)
	float Value = 0.0f;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = StatOperation)
	EStatValueType ValueType = EStatValueType::CurrentValue;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	EStatModificationOperation StatOperation = EStatModificationOperation::Addition;

	FStatOperation(FName InputName, float InputValue,EStatValueType InputValueType = EStatValueType::CurrentValue, EStatModificationOperation InputStatOperation = EStatModificationOperation::Addition)
	{
		StatName = InputName;
		Value = InputValue;
		ValueType = InputValueType;
		StatOperation = InputStatOperation;

	}
	FStatOperation()
	{

	}
	FORCEINLINE bool operator==(const FStatOperation &Other) const 
    {
        return (StatName == Other.StatName && ValueType == Other.ValueType && StatOperation == Other.StatOperation );
    }
	FORCEINLINE bool operator!=(const FStatOperation &Other) const 
    {
        return (StatName != Other.StatName || ValueType != Other.ValueType || StatOperation != Other.StatOperation );
    }
	FORCEINLINE FStatOperation operator+(const FStatOperation &Other) const 
	{
		float NewValue = Value;
		if(*this == Other )
		{
			NewValue = NewValue + Other.Value;
		}	
		return FStatOperation(StatName,NewValue,ValueType,StatOperation);
	}
	FORCEINLINE bool operator<(const FStatOperation &Other) const
	{
		if(*this == Other)
		{
			if(Value < Other.Value)
			{
				return true;
			}
		}
		return false;
	}
};



/*
A replicated component that manages a StatDictionary (Key Name, Value FStat) and
uses multicast RPC to keep client and servers in sync.

Also manages complicated stat changes to get a total of a stat. These include:

StatusEffects
Stat Bindings (have one stat influence the value of another stat )
Influence of other UStatManagers for equipment (Player has a Damage Stat of 100, sword has a damage Stat of 100, we can add those together)
*/
UCLASS(  Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTILITYCOMBATPLUGIN_API UStatManager : public UActorComponent 
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStatManager();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	
	/*
	Event Fires whenever ModifyStat() is called
	*/
	UPROPERTY(BlueprintAssignable)
	FStatModified OnStatModified;


	/*
	Called when the effect has been managed by InitializeEffect()
	*/
	UPROPERTY(BlueprintAssignable)
	FEffectStateChange OnEffectInitialize;


	/*
	Called when the effect has been managed by ClearEffects()
	*/
	UPROPERTY(BlueprintAssignable)
	FEffectStateChange OnEffectClear;

	UPROPERTY(BlueprintAssignable)
	FEffectStateChange OnEffectPause;

	UPROPERTY(BlueprintAssignable)
	FEffectStateChange OnEffectResume;

	UPROPERTY(BlueprintAssignable)
	FDataTableInitialized OnDataTableInitialized;

	/*
	Called when the StatusEffectComponenent EffectDelegate (timer) ticks.
	Used to Update the UI
	*/
	UPROPERTY(BlueprintAssignable)
	FStatApplied OnStatApplied;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug)
	bool bShowDebugWarnings = false;



	/*
	Instead of setting variables into StatDictionary, you can read a data table into it! (FName rows are the corresponding keys!)
	Table must be a type FStat
	Call ReadStatDataTable() to set StatDictionary accordingly.
	Note this will override any values set in StatDictionary
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	UDataTable* StatDataTable = nullptr;


	/*
	Key Stat that is bound
	Value: Type of binding that will effect the stat

	Not replicated, but kept in sync by muliticast RPC
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category = Stats)
	TMap<FName,FStatBind> StatBindings = {};

	/*
	The main dictionary that holds data about the stats
	Key FName of the stat
	Value FStat

	Not replicated, but kept in sync by muliticast RPC
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Stats)
	TMap<FName,FStat> StatDictionary = {};	


		/*
	Data table to read the effect from. StatName is the FName of the stat to look up
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StatusEffects)
	UDataTable* EffectData = nullptr;

	/*
	All the status effects this stat component manages
	Note this will only be created on the server, and the effect components will
	tick and pass multicast events.
	*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = StatusEffects)
	TArray<UStatusEffectComponent*> EffectComponentList = {};

	/*
	Used to keep a running total, each time an effect calls ApplyStatusEffectMulticast 

	Not replicated, but kept in sync by muliticast RPC
	*/
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = StatusEffects)
	TMap<FName,float> EffectorDictionary;


	/*
	Whether this stat component can effect the value of a master stat statcomponent. Useful for changing 
	equipped status.
	*/	
	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category = OtherStatComponentManagement)
	bool bStatDictionaryCanModifyOtherStatDictionary = true;


	/*
	Other stat managers can effect the final value of a stat.
	For example a character can have a speed, but then have a sword of speed equipped.
	The final value of the char's speed would be his base speed, plus whatever the sword grants
	The swords value will only be included if bStatDictionaryCanModifyOtherStatDictionary = true.
	The character's stat component should be treated as the master stat statcomponent, and equippable items stored here.
	*/
	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category = OtherStatComponentManagement)
	TArray<UStatManager*> OtherStatManagers = {};


	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = ReplicationAssist)
	float TimeLastCalledCollectionRPC = -1.0f;

	/*
	Used to help manage the number of modify stat RPC's.
	*/
	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadWrite,Category = ReplicationAssist)
	TArray<FStatOperation> StatOperations = {};


	//UPROPERTY(VisibleAnywhere)
    FTimerHandle ModifyStatRPCHandle; //Set to the world's timer manager

    FTimerDelegate ModifyStatRPCTimer;


	
	/*
	Calls OnDataTableInitialized.Broadcast(). 
	You want tobind update functions to this method
	*/
	UFUNCTION(BlueprintCallable,NetMulticast, Reliable, Category = Important)
	void ForceUpdateUIMulticast();
	

	/*
	Works whether has authority or not, but UI will NOT updated unless called from server. 
	Calls an RPC if the owner does not have authority and is ROLE_AutonomousProxy.

	If the owner has authority StatArrayForReplication variable is modified. 
	The stat changes made  in StatArrayForReplication will then be 
	replicated to all clients when OnRep_StatArray() is processed.
	*/
	UFUNCTION(BlueprintCallable, Category = Stat)
	void AddStat(FName StatName, UPARAM(ref) const FStat& Stat );

	UFUNCTION(NetMulticast,Reliable, Category = Stat)
	void AddStatMulticast(FName StatName, const FStat& Stat );

	UFUNCTION(Server,Reliable, Category = Stat)
	void AddStatServer(FName StatName, const FStat& Stat );


		/*
	Calls the RPC if the owner does not have authority and is ROLE_AutonomousProxy
	If the caller has authority, then the changes will be passed into StatArrayForReplication. 
	Clients will update themselves when processing the On_Rep function
	*/
	UFUNCTION(BlueprintCallable, Category = Stat)
	void ModifyStat(const FName StatName, const float Value,  const EStatValueType ValueType = EStatValueType::CurrentValue, const EStatModificationOperation StatOperation = EStatModificationOperation::Addition);

	UFUNCTION(NetMulticast,Reliable,  Category = Stat)
	void ModifyStatMulticast(const FName StatName, const float Value,  const EStatValueType ValueType, const EStatModificationOperation StatOperation);

	UFUNCTION(Server,Reliable, Category = Stat)
	void ModifyStatServer(const FName StatName, const float Value,  const EStatValueType ValueType, const EStatModificationOperation StatOperation);

	/*
	Instead of one multicast RPC per ModifyStat() call,
	we collect and combine many modify stat operations over a defined period of time, then
	call one RPC
	*/
	UFUNCTION(BlueprintCallable, Category = Stat)
	void ModifyStatManyModifications(const FName StatName, const float Value,  const EStatValueType ValueType = EStatValueType::CurrentValue, const EStatModificationOperation StatOperation = EStatModificationOperation::Addition, float TimeToCollectModifications = 0.1f);

	//Uses the class variable StatArrayForReplication
	UFUNCTION()
	void ModifyStatByArray();

	UFUNCTION(NetMulticast,Reliable, Category = Stat)
	void ModifyStatByArrayMulticast(const TArray<FStatOperation>& StatOperationArray);

	UFUNCTION(Server,Reliable, Category = Stat)
	void ModifyStatByArrayServer(const TArray<FStatOperation>& StatOperationArray);

	

	/*
	Call server RPC if this is called from  ROLE_AutonomousProxy. 
	Then the server will call RemoveStatMulticast()
	*/
	UFUNCTION(BlueprintCallable, Category = Stat)
	void RemoveStat(FName StatName);

	//Will run on server and all clients if called from the server
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = Stat)
	void RemoveStatMulticast(FName StatName);

	UFUNCTION(BlueprintCallable, Server,Reliable, Category = Stat)
	void RemoveStatServer(FName StatName);


	UFUNCTION(BlueprintCallable, Category = StatBinding)
	void AddStatBinding(FName BoundStat, UPARAM(ref) const FStatBind& NewStatBinding);

	UFUNCTION(NetMulticast,Reliable, Category = StatBinding)
	void AddStatBindingMulticast(FName BoundStat, const FStatBind& NewStatBinding);

	UFUNCTION(Server,Reliable, Category = StatBinding)
	void AddStatBindingServer(FName BoundStat, const FStatBind& NewStatBinding);


	UFUNCTION(BlueprintCallable, Category = StatBinding)
	void RemoveStatBinding(FName StatName);

	UFUNCTION( NetMulticast, Reliable, Category = StatBinding)
	void RemoveStatBindingMulticast(FName StatName);

	UFUNCTION(Server,Reliable, Category = StatBinding)
	void RemoveStatBindingServer(FName StatName);
	

	UFUNCTION(BlueprintCallable, Category = StatBinding)
	void SetStatBindings(UPARAM(ref) const TMap<FName,FStatBind>& InputStatBindings);

	UFUNCTION( NetMulticast, Reliable, Category = StatBinding)
	void SetStatBindingsMulticast(const TArray<FName>& Names, const TArray<FStatBind>& StatBinds);
	//void SetStatBindingsMulticast(const TMap<FName,FStatBind>& InputStatBindings);

	UFUNCTION(Server,Reliable, Category = StatBinding)
	void SetStatBindingsServer(const TArray<FName>& Names, const TArray<FStatBind>& StatBinds);
	//void SetStatBindingsServer(const TMap<FName,FStatBind>& InputStatBindings);


	UFUNCTION(NetMulticast,Reliable, Category = Status)
	void ApplyStatusEffectMulticast(const  FStatusEffect StatusEffect,float Effector );
	

	UFUNCTION(NetMulticast,Reliable,Category = Status)
	void ClearEffectMulticast(const FName EffectName, const FStatusEffect StatusEffect);	

	/*
	Calls the RPC if the owner does not have authority and is ROLE_AutonomousProxy
	*/
	UFUNCTION(BlueprintCallable,Category = Status)
	void ClearEffects(EStatusClearCondition ClearCondition);

	UFUNCTION(NetMulticast,Reliable,Category = Status)
	void ClearEffectsMulticast(EStatusClearCondition ClearCondition);

	UFUNCTION(Server,Reliable,Category = Status)
	void ClearEffectsServer(EStatusClearCondition ClearCondition);
	


		/*
	Given a FStatusEffect, create an actor component to manage it.
	
	Works whether has authority or not, but UI will might not be updated unless called from server. 
	Calls an RPC if the owner does not have authority and is ROLE_AutonomousProxy.

	If the caller has authority, a replicated UStatusEffectComponent will be created, 
	and propagate the changes to clients.

	NOTE: EffectSaveTime, EffectStartTime are exposed to BP, but don't use them. They are for 
	initializing the component properly after loading from save data. (i.e adjust the given duration)


	*/
	UFUNCTION(BlueprintCallable,Category = Status)
	void InitializeEffect(const FName EffectName, UPARAM(ref) const  FStatusEffect& EffectToInitalize, float StartEffector = 0.0f, float EffectSaveTime = 0.0f, float EffectStartTime = 0.0f);

	UFUNCTION(NetMulticast,Reliable,Category = Status)
	void InitializeEffectMulticast(const FName EffectName, const  FStatusEffect EffectToInitalize);

	UFUNCTION(Server,Reliable,Category = Status)
	void InitializeEffectServer(const FName EffectName, const  FStatusEffect EffectToInitalize, float StartEffector = 0.0f, float EffectSaveTime = 0.0f, float EffectStartTime = 0.0f);



	/*
	More efficient to do loops through an array in C++ than in BP
	*/
	UFUNCTION(BlueprintCallable,Category = Status)
	void InitializeEffectArray(UPARAM(ref) const TArray<FName>& EffectNames, UPARAM(ref) const TArray<FStatusEffect>& EffectArray);



	UFUNCTION(BlueprintCallable, Category = Status)
	void PauseEffectByName(const FName EffectName);

	UFUNCTION(NetMulticast,Reliable, Category = Status)
	void PauseEffectByNameMulticast(const FName EffectName,const  FStatusEffect StatusEffect);

	UFUNCTION( Server,Reliable, Category = Status)
	void PauseEffectByNameServer(const FName EffectName);

	UFUNCTION(BlueprintCallable, Category = Status)
	void ResumeEffectByName(const FName EffectName);

	UFUNCTION(NetMulticast,Reliable, Category = Status)
	void ResumeEffectByNameMulticast(const FName EffectName,const  FStatusEffect StatusEffect);

	UFUNCTION(Server,Reliable, Category = Status)
	void ResumeEffectByNameServer(const FName EffectName);


	/* 
	SIMPLE Stat management. 
	Only cares about StatDictionary
	*/

	/*
	Returns Zero if not found.
	Only includes the value found in 
	StatDictionary.

	Does NOT consider
	Effectors
	StatBindings
	Or other stat Managers

	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	float GetCurrentValueRaw(const FName StatName) const;
	




	/*
	Worried about threading problems.
	Finds the sum of all the status effects that modify a stat by FName.
	ZeroFoundEffectors will cause all effectors from relevant StatusEffectComponents to be zeroed if true
	*/
	UFUNCTION(BlueprintPure, Category = StatQuery)
	float GetEffectorTotal(const FName StatName, bool ZeroFoundEffectors = false);

		
	/*
	Sums up the FStat in StatDictionary with
	StatBindings
	Effectors
	Doesn't include OtherStatManagers.

	Handles stats not in StatDictionary but defined elsewhere. 
	Will Clamp the total by the Minimum and Maximum of the FStat defined
	in StatDictionary, if found. Otherwise the total of InstantStatOffset and EffectComponentList
	is returned
	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	float GetStatValue(const FName StatName);


	/*
	Sums up the value of a given stat for all stat components defined OtherStatManagers.

	General Code:
	For UStatManager in OtherStatManagers,
		Call GetStatValue() 
		Sum those up.
	
	Add in GetStatValue() from this component
	return Total.

	if  bUseRawForSelf is true.  GetCurrentValueRaw() will be used for value stored in this component

	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	float GetStatTotal(const FName StatName, bool bUseRawForSelf = false);

	




	/*
	StatDictionary  + EffectTotal + StatBoundEffector;
	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	FStat GetStatValueAsStat(const FName StatName);

	/*
	Includes other stat managers, calling GetStatValue on them and self.
	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	FStat GetStatTotalAsStat(const FName StatName,  bool bUseRawForSelf = false);


		
	/*
	Returns Zero if not found.
	Only includes the value found in 
	StatDictionary.

	Does NOT consider
	Effectors
	StatBindings
	Or other stat Managers

	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	FStat GetRawStat(const FName StatName) const;


	/*
	Same as GetStatTotal except GetCurrentValueRaw() is called on
	other stat managers
	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	float GetRawStatTotal(const FName StatName, bool bUseRawForSelf = true);






	//is stat >=,>,==, <,<=,!=

	/*
	Returns false if stat is not in the dictionary
	Checks the CurrentValue of the stat
	Only checks the stat in StatDictionary,
	Does NOT consider
	Effector/EffectComponentList
	StatBindings
	Or other stat Managers
	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	bool IsStatGreaterThan(const FName StatName, const float Value = 0.0f);

	/*
	Returns false if stat is not in the dictionary
	Checks the CurrentValue of the stat.
	Only checks the stat in StatDictionary,
	Does NOT consider
	Effector/EffectComponentList
	StatBindings
	Or other stat Managers
	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	bool IsStatGreaterThanOrEqualTo(const FName StatName, const float Value = 0.0f);

	/*
	Returns false if stat is not in the dictionary
	Checks the CurrentValue of the stat
	Only checks the stat in StatDictionary,
	Does NOT consider
	Effector/EffectComponentList
	StatBindings
	Or other stat Managers
	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	bool IsStatLessThan(const FName StatName, const float Value = 0.0f);

	/*
	Returns false if stat is not in the dictionary
	Checks the CurrentValue of the stat
	Only checks the stat in StatDictionary,
	Does NOT consider
	Effector/EffectComponentList
	StatBindings
	Or other stat Managers
	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	bool IsStatLessThanOrEqualTo(const FName StatName, const float Value = 0.0f);

	/*
	Returns false if stat is not in the dictionary
	Checks the CurrentValue of the stat
	Only checks the stat in StatDictionary,
	Does NOT consider
	Effector/EffectComponentList
	StatBindings
	Or other stat Managers
	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	bool IsStatEqualTo(const FName StatName, const float Value = 0.0f);

	/*
	Returns false if stat is not in the dictionary
	Checks the CurrentValue of the stat
	Only checks the stat in StatDictionary,
	Does NOT consider
	Effector/EffectComponentList
	StatBindings
	Or other stat Managers
	*/
	UFUNCTION(BlueprintPure,Category = StatQuery)
	bool IsStatNotEqualTo(const FName StatName, const float Value = 0.0f);






		
	UFUNCTION(BlueprintCallable,Category = Utility)
	FStatusEffect ReadEffectData(const FName EffectToRead, bool& bValidData, bool bDebugWarnings = false);

	UFUNCTION(BlueprintPure,Category = Utility)
	TArray<FStatusEffect> ReadEffects(UPARAM(ref) const TArray<FName>& EffectsToRead);

	/*
	returns false if data table isn't valid
	*/
	UFUNCTION(BlueprintCallable, Category = Utility)
	bool ReadStatDataTable();


	/*
	Bulk RPC managment
	*/


	UFUNCTION(BlueprintCallable, Category = Utility)
	void EmptyDictionaries(bool bEmptyStatDictionary, bool bEmptyStatBindingDictionary, bool bEmptyEffectorDictionary);

	UFUNCTION( NetMulticast, Reliable, Category = Utility)
	void EmptyDictionariesMulticast(bool bEmptyStatDictionary, bool bEmptyStatBindingDictionary, bool bEmptyEffectorDictionary);

	UFUNCTION(Server,Reliable, Category = Utility)
	void EmptyDictionariesServer(bool bEmptyStatDictionary, bool bEmptyStatBindingDictionary, bool bEmptyEffectorDictionary);



	UFUNCTION(BlueprintCallable, Category = Utility)
	void SetStatDictionary(UPARAM(ref) const TMap<FName,FStat>& InputStatDictionary);

	UFUNCTION( NetMulticast, Reliable, Category = Utility)
	void SetStatDictionaryMulticast(const TArray<FName>& Names, const TArray<FStat>& Stats);
	//void SetStatDictionaryMulticast(const TMap<FName,FStat>& InputStatDictionary);

	UFUNCTION(Server,Reliable, Category = Utility)
	void SetStatDictionaryServer(const TArray<FName>& Names, const TArray<FStat>& Stats);
	//void SetStatDictionaryServer(const TMap<FName,FStat>& InputStatDictionary);





	/*
	SAVING AND LOADING
	*/

	UFUNCTION(BlueprintPure, Category = SavingAndLoading)
	TArray<FStatusEffectSaveData> GenerateStatusEffectSaveData() const;

	UFUNCTION(BlueprintCallable, Category = SavingAndLoading)
	void LoadSaveData(const TMap<FName,FStat>& InputStatDictionary, const TMap<FName,FStatBind>& InputStatBindings,const TArray<FStatusEffectSaveData>& InputSavedStatusEffectData);
	

};
