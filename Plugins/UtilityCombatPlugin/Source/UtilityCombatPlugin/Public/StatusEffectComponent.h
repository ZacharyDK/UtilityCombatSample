// Copyright Zachary Kolansky, 2020

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "StatDataStructures.h"
#include "StatusEffectComponent.generated.h"

/*
Wrapper component created at runtime to be an outer for managing status effects
FTimerDelegate components for their UFUNCTION Binding
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTILITYCOMBATPLUGIN_API UStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UStatusEffectComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


	virtual void GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;




	UPROPERTY(VisibleAnywhere)
	FStatusEffect StatusEffect = FStatusEffect();

	FTimerDelegate EffectDelegate; //Degates are bound to UFUNCTIONS

	UPROPERTY(VisibleAnywhere)
    FTimerHandle EffectHandle; //Set to the world's timer manager

    FTimerDelegate DurationDelegate;

	UPROPERTY(VisibleAnywhere)
    FTimerHandle DurationHandle;

	//Value that this Effect changes.
	UPROPERTY(BlueprintReadOnly,VisibleAnywhere, Category = StatusEffect) 
    float Effector = 0.0f; 

	//TODO Make clear effects multicast
	UPROPERTY(VisibleAnywhere, Category = StatusEffect)
    bool bIsCleared = false;

	UPROPERTY(VisibleAnywhere, Category = StatusEffect)
	FName EffectName = FName("");

	UPROPERTY(VisibleAnywhere, Category = StatusEffect)
	float EffectStartTime = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = StatusEffect)
	float EffectPauseTime = 0.0f;

	UPROPERTY(VisibleAnywhere)
	class UStatManager* MasterStatManager = nullptr;


	void InitializeStatusEffectComponent(UStatManager* InputMasterStatManager,const FName InputEffectName, const FStatusEffect& InputStatusEffect, float InputEffector = 0.0f, float InputSaveTime = 0.0f, float InputStartTime = 0.0f);


	UFUNCTION()
	void ApplyEffect();

	/*
	UFUNCTION(NetMulticast,Reliable)
	void ApplyEffectMulticast();
	*/

	UFUNCTION()
	void ClearEffect();

	void PauseEffect();

	

	void ResumeEffect();


	void ZeroEffector();


};
