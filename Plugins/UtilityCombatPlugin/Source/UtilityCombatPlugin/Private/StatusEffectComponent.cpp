// Copyright Zachary Kolansky, 2020


#include "StatusEffectComponent.h"
#include "StatManager.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UStatusEffectComponent::UStatusEffectComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	
}


// Called when the game starts
void UStatusEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


void UStatusEffectComponent::GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(UStatusEffectComponent,Effector);
}


void UStatusEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(EffectHandle);
	GetWorld()->GetTimerManager().ClearTimer(DurationHandle);
	Super::EndPlay(EndPlayReason);
}


// Called every frame
void UStatusEffectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UStatusEffectComponent::InitializeStatusEffectComponent(UStatManager* InputMasterStatManager,const FName InputEffectName, const FStatusEffect& InputStatusEffect, float InputEffector, float InputSaveTime, float InputStartTime)
{
	EffectName = InputEffectName;
	MasterStatManager = InputMasterStatManager;
	StatusEffect = InputStatusEffect;

	float TimeElasped = InputSaveTime - InputStartTime;
	Effector = InputEffector; //Resume how much it applied



	
	if (StatusEffect.Duration < 0) //Effect lasts forever
	{
	
		EffectDelegate.BindUFunction(this,FName("ApplyEffect"));
		GetWorld()->GetTimerManager().SetTimer(EffectHandle,EffectDelegate,StatusEffect.TickTime,true);
		
	}
	else if(StatusEffect.Duration - TimeElasped > 0.0f)
	{
		

		DurationDelegate.BindUFunction(this,FName("ClearEffect"));
		GetWorld()->GetTimerManager().SetTimer(DurationHandle,DurationDelegate,StatusEffect.Duration-TimeElasped,false);

		EffectDelegate.BindUFunction(this,FName("ApplyEffect"));
		GetWorld()->GetTimerManager().SetTimer(EffectHandle,EffectDelegate,StatusEffect.TickTime,true);

		StatusEffect.Duration = StatusEffect.Duration - TimeElasped; //Update saved duration
		
	}
	EffectStartTime = GetWorld()->GetTimeSeconds();
	
}




void UStatusEffectComponent::ApplyEffect()
{
	
	Effector = Effector + StatusEffect.Intensity;
	if(MasterStatManager)
	{
		MasterStatManager->ApplyStatusEffectMulticast(StatusEffect,Effector); 
	}
}


void UStatusEffectComponent::ClearEffect()
{
	bIsCleared = true;
	GetWorld()->GetTimerManager().ClearTimer(EffectHandle);
	GetWorld()->GetTimerManager().ClearTimer(DurationHandle);

	if(MasterStatManager)
	{
		//Add the final value of the effector to the actual stat in the Dictionary. 
		
		//The stat component will automatically get the effector value from its Effector Map, and zero the map. 
		//We have to zero the effector map and add the effector to the corresponding FStat to save the changes it made to the stat
		//Moreover, Modify stat will ve multicast, and ClearEffectMulticast ensures the OnCleared delegate is called on the client and server
		MasterStatManager->ModifyStat(StatusEffect.ActionName,0.0f); 
		MasterStatManager->ClearEffectMulticast(EffectName,StatusEffect);
	}

	this->DestroyComponent();
}

void UStatusEffectComponent::PauseEffect()
{
	GetWorld()->GetTimerManager().PauseTimer(EffectHandle);
	GetWorld()->GetTimerManager().PauseTimer(DurationHandle);
	EffectPauseTime = GetWorld()->GetTimeSeconds();
}



void UStatusEffectComponent::ResumeEffect()
{

	GetWorld()->GetTimerManager().UnPauseTimer(EffectHandle);
	GetWorld()->GetTimerManager().UnPauseTimer(DurationHandle);
}



void UStatusEffectComponent::ZeroEffector()
{
	Effector = 0.0f;
}