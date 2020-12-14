// Copyright Zachary Kolansky, 2020


#include "MaterialManagerComponent.h"
#include "Components/MeshComponent.h"

#include "Materials/MaterialInstanceDynamic.h"

// Sets default values for this component's properties
UMaterialManagerComponent::UMaterialManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UMaterialManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UMaterialManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UMaterialManagerComponent::Initialize(UMeshComponent* InputMeshToManage)
{
	MeshToManage = InputMeshToManage;
	
	
	if(!MeshToManage)
	{
		if(bDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("MeshToManage was a nullptr, returning from Initialize() "))
		}
		
		return;
	}

	
	int32 NumberOfMaterials = MeshToManage->GetNumMaterials();
	for (int32 i = 0; i < NumberOfMaterials; i++)
	{
		UMaterialInstanceDynamic* DynamicMaterial = MeshToManage->CreateAndSetMaterialInstanceDynamic(i); //Method specified in UPrimitiveComponent, Uses Material there as the parent
	}
	
}

void UMaterialManagerComponent::AddNewVisualEffect(FName VisualEffectName,  const FStatusEffectVisual& EffectVisualData)
{
	CurrentVisualEffects.Add(VisualEffectName,EffectVisualData);
	ApplyVisualEffect(DetermineCumulativeEffect());
	OnVisualEffectAdded.Broadcast(VisualEffectName,EffectVisualData);
}

void UMaterialManagerComponent::ApplyVisualEffect(const FStatusEffectVisual& InputEffectVisual)
{



	if(MeshToManage)
	{
		MeshToManage->SetVectorParameterValueOnMaterials(FNameParameters.Color1ParameterName,FVector(InputEffectVisual.Color1));
		MeshToManage->SetVectorParameterValueOnMaterials(FNameParameters.Color2ParameterName,FVector(InputEffectVisual.Color2));
		MeshToManage->SetScalarParameterValueOnMaterials(FNameParameters.MinEmissionParameterName,InputEffectVisual.MinEmission);
		MeshToManage->SetScalarParameterValueOnMaterials(FNameParameters.MaxEmissionParameterName,InputEffectVisual.MaxEmission);
		MeshToManage->SetScalarParameterValueOnMaterials(FNameParameters.EmissionChangeSpeedParameterName,InputEffectVisual.EmissionChangeSpeed);
		MeshToManage->SetScalarParameterValueOnMaterials(FNameParameters.ColorChangeSpeedParameterName,InputEffectVisual.ColorChangeSpeed);
	}
	else 
	{
		if(bDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("MeshToManage was a nullptr, can't ApplyVisualEffect()"))
		}
		
	}


}

FStatusEffectVisual UMaterialManagerComponent::DetermineCumulativeEffect() const
{
	TArray<FStatusEffectVisual> VisualArray = {};
	CurrentVisualEffects.GenerateValueArray(VisualArray);

	FStatusEffectVisual Sum = FStatusEffectVisual();

	for (const FStatusEffectVisual& Vis : VisualArray)
	{
		Sum = Sum + Vis;
	}

	return Sum;
	
}


void UMaterialManagerComponent::RemoveVisualEffect(FName VisualEffectName)
{
	
	
	FStatusEffectVisual EffectVisualData = FStatusEffectVisual();
	TArray<FName> Keys = {};
	CurrentVisualEffects.GenerateKeyArray(Keys);
	bool bEffectFound = false;

	if(Keys.Contains(VisualEffectName))
	{
		EffectVisualData = CurrentVisualEffects[VisualEffectName];
		bEffectFound = true;
	}
	
	CurrentVisualEffects.Remove(VisualEffectName);
	ApplyVisualEffect(DetermineCumulativeEffect());

	if(bEffectFound)
	{
		OnVisualEffectRemoved.Broadcast(VisualEffectName,EffectVisualData);
	}

}

