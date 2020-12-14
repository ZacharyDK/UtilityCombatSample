// Copyright Zachary Kolansky, 2020

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatDataStructures.h"
#include "MaterialManagerComponent.generated.h"


class UMeshComponent;
class UMaterialInterface; //Unifier both regular materials and Instanced Materials
class UMaterialInstance;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVisualEffectState, FName, VisualEffectName, FStatusEffectVisual, VisualData );

/*
The purpose of this class is to create and set dynamic material instances of a skeletal mesh component
In particular, its used to easily set multiple scalar and vector parameters for status effects. 
Because this component automatically creates the dynamic material instances, you just need 
to call SetVectorParameterValueOnMaterials() or SetScalarParameterValueOnMaterials()
on your mesh.
*/
UCLASS(  Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent)  )
class UTILITYCOMBATPLUGIN_API UMaterialManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMaterialManagerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug)
	bool bDebugWarnings = false;


	UPROPERTY(BlueprintAssignable)
	FVisualEffectState OnVisualEffectAdded;

	UPROPERTY(BlueprintAssignable)
	FVisualEffectState OnVisualEffectRemoved;


	/*
	Key = VisualEffect (FName from FStatusEffect)
	Value FStatusEffectVisual
	*/
	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category = Effects)
	TMap<FName,FStatusEffectVisual> CurrentVisualEffects;

	/*
	The name of each scalar/vector parameter in FStatusEffectVisual to assign.
	Defaults match with the material function I provided
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	FStatusEffectVisualFNameParameters FNameParameters = FStatusEffectVisualFNameParameters();

	/*
	The SkeletalMeshComponent we want to manage
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Mesh)
	UMeshComponent* MeshToManage = nullptr;

	/*
	Adds inputs into CurrentVisualEffects
	Then calls ApplyVisualEffect()
	*/
	UFUNCTION(BlueprintCallable, Category = Effects)
	void AddNewVisualEffect(FName VisualEffectName, UPARAM(ref) const FStatusEffectVisual& EffectVisualData);

	/*
	Sets Scalar and Vector parameters on the materials based on FNameParameters
	*/
	void ApplyVisualEffect(const FStatusEffectVisual& InputEffectVisual);

	/*
	Sums up the value list of CurrentVisualEffects TMap 
	*/
	FStatusEffectVisual DetermineCumulativeEffect() const;
	

	/*
	Call during Owner's constructor.
	If this component has a mesh to manage, 
	we remmeber the mesh and call CreateAndSetMaterialInstanceDynamic()
	for each material layer.
	*/
	UFUNCTION(BlueprintCallable, Category = Initialize)
	void Initialize(UMeshComponent* InputMeshToManage);



	/*
	Removes all values associated with the key (of type FName) in CurrentVisualEffects
	Then calls ApplyVisualEffect()
	*/
	UFUNCTION(BlueprintCallable)
	void RemoveVisualEffect(FName VisualEffectName);
		
};
