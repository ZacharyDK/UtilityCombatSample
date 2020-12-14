// Copyright Zachary Kolansky, 2020


#include "WeaponManager.h"
#include "WeaponActor.h"
#include "Engine/EngineTypes.h"
//#include "../FunctionLibrary/UtilityFunctions.h"
#include "StatManager.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UWeaponManager::UWeaponManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicated(true);
	// ...
}


// Called when the game starts
void UWeaponManager::BeginPlay()
{
	Super::BeginPlay();
	Initialize();	
}


void UWeaponManager::GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWeaponManager,bEquipState);
	DOREPLIFETIME(UWeaponManager,ActiveWeaponSlot);
	DOREPLIFETIME(UWeaponManager,ActiveWeapon);
	DOREPLIFETIME(UWeaponManager,CurrentEquippedWeaponType);
	DOREPLIFETIME(UWeaponManager,DelayedEquipSocketChangeSlot);
	DOREPLIFETIME(UWeaponManager,DelayedHolsterSocketChangeSlot);
	DOREPLIFETIME(UWeaponManager,Weapons);

}

// Called every frame
void UWeaponManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UWeaponManager::Initialize()
{
	AActor* OwnerActor = GetOwner();
	Owner = Cast<ACharacter>(OwnerActor);
	if(Owner)
	{
		OwnerStatManager = Owner->FindComponentByClass<UStatManager>();
	}
	

	if(!WeaponEquipData)
	{
		
		UE_LOG(LogTemp,Warning,TEXT("WeaponEquipData given to Initialize() is not valid"))
		return;
	}

	TArray<FName> InputDataTableRowNames = WeaponEquipData->GetRowNames();

	for (FName Row : InputDataTableRowNames)
	{
		FEquipData* PointerOfTypeT = WeaponEquipData->FindRow<FEquipData>(Row, FString(""));
		FEquipData StructObject = *PointerOfTypeT; //Deference
		EquipDataDictionary.Add(Row,StructObject);

	}


}

float UWeaponManager::PlayMontage( ACharacter* OwnerChar, UAnimMontage* Montage, float PlayRate , FName StartSectionName, bool bShowDebugWarnings)
{
	if(!OwnerChar)
	{
		if(bShowDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("OwnerChar is a nullptr, PlayMontage() returning 0.0f "))
		}

		return 0.0f;
	}

	if(!Montage)
	{
		if(bShowDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("Montage is a nullptr, PlayMontage() returning 0.0f "))
		}


		return 0.0f;
	}

	if(PlayRate < 0.0f)
	{
		if(bShowDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("Playrate was < 0, setting Playrate = 1!"))
		}
		PlayRate = 1.0f;
	}
	
	return OwnerChar->PlayAnimMontage(Montage,PlayRate,StartSectionName);
}

void UWeaponManager::AddWeaponToArray(AWeaponActor* InputWeaponActor)
{
	if(!Check(InputWeaponActor))
	{
		return;
	}

	if(InputWeaponActor->IsInWeaponInventory)
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("Given weapon actor is already in the inventoy."))
		}
		return;
	}


	if(!Owner->HasAuthority() && Owner->GetLocalRole() == ROLE_AutonomousProxy )  //!Owner->IsLocallyControlled())
	{
		AddWeaponToArrayServer(InputWeaponActor);
	}
	else if(Owner->HasAuthority())
	{
		AddWeaponToArrayServer(InputWeaponActor);
	}
	
	


}

void UWeaponManager::AddWeaponToArrayMulticast_Implementation(AWeaponActor* InputWeaponActor)
{
	OnActiveWeaponChange.Broadcast(InputWeaponActor);
}

void UWeaponManager::AddWeaponToArrayServer_Implementation(AWeaponActor* InputWeaponActor)
{
	int32 NumberOfWeaponsAcquired = Weapons.Num();

	if(NumberOfWeaponsAcquired > WeaponSlots)
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("NumberOfWeaponsAcquired >= WeaponSlots, no room for another weapon"))
		}
		
		return; //weapon array already filled. Request denied.
	}
	else if(!EquippableWeaponTypes.Contains(InputWeaponActor->WeaponData.WeaponType) && InputWeaponActor->WeaponData.WeaponType != FName(""))
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("We cannot equip a weapon of class %s"),*(InputWeaponActor->WeaponData.WeaponType.ToString()))
		}
		return;
	}


	Weapons.Add(InputWeaponActor); 
	//If we have zero weapons, add to slot zero. If we have one weapon, add to slot 1.
	//Remember counting starts at zero.
	
	if(ActiveWeapon)
	{
		HolsterWeapon(ActiveWeaponSlot,true,false);
	}
	
	EquipWeapon(NumberOfWeaponsAcquired);
	AddWeaponToArrayMulticast(InputWeaponActor); 
}

void UWeaponManager::AttachWeaponToOwner(AWeaponActor* InputWeaponActor,FName SocketName)
{
	if(!Check(InputWeaponActor))
	{
		return;
	}

	if(!Owner->HasAuthority() && Owner->GetLocalRole() == ROLE_AutonomousProxy ) //Checking if Auto proxy solves No owning connection for actor warnings.
	{
		AttachWeaponToOwnerServer(InputWeaponActor,SocketName);
	}
	else if(Owner->HasAuthority())
	{
		AttachWeaponToOwnerServer(InputWeaponActor,SocketName);
	}

	return;
}

void UWeaponManager::AttachWeaponToOwnerServer_Implementation(AWeaponActor* InputWeaponActor,FName SocketName)
{
	
	FAttachmentTransformRules AttachRules = FAttachmentTransformRules(EAttachmentRule::SnapToTarget,true);
	//false means don't weld the bodies together.

	if(InputWeaponActor->IsInWeaponInventory)
	{
		return;
	}

	USkeletalMeshComponent* CharMesh = Owner->GetMesh();

	if(!CharMesh)
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("AttachWeaponToOwner() failed, CharMesh is nullptr"))
		}
		return;
	}

	
	FVector WeaponWorldScale = InputWeaponActor->GetActorScale3D(); //WorldSpace scale

	InputWeaponActor->AttachToComponent(CharMesh,AttachRules,SocketName);
	InputWeaponActor->IsInWeaponInventory = true;

	if(bUseWeaponScaleInsteadOfSocketScale)
	{
		InputWeaponActor->SetActorScale3D(WeaponWorldScale);
	}

}



bool UWeaponManager::Check(AWeaponActor* InputWeaponActor)
{
	if(!Owner)
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("Owner a nullptr. Initialize this component propertly!"))
		}
		
		return false;
	}
	
	
	if(!InputWeaponActor)
	{	
		
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("InputWeaponActor is nullptr!"))
		}
		return false;
	}
	return true;
}

void UWeaponManager::DecreaseActiveSlotIndex(bool bPlayMontage)
{
	if(!Owner->HasAuthority() && Owner->GetLocalRole() == ROLE_AutonomousProxy ) 
	{
		DecreaseActiveSlotIndexServer(bPlayMontage);
	}
	else if(Owner->HasAuthority())
	{
		DecreaseActiveSlotIndexServer(bPlayMontage);
	}



	
	 
}

void UWeaponManager::DecreaseActiveSlotIndexServer_Implementation(bool bPlayMontage)
{
	int32 NumberOfWeapons = Weapons.Num();

	if(NumberOfWeapons <= 1) //No weapon to change
	{
		return;
	}

	int32 NextWeaponSlot = (ActiveWeaponSlot - 1); 
	
	if(NextWeaponSlot < 0)
	{
		NextWeaponSlot = NumberOfWeapons-1;
	}
	else
	{
		//NextWeaponSlot % NumberOfWeapons; 
	}
	
	SetActiveEquipSlot(NextWeaponSlot,bPlayMontage);
}	

void UWeaponManager::DetachWeaponFromOwner(AWeaponActor* InputWeaponActor)
{
	if(!Check(InputWeaponActor))
	{
		return;
	}

	if(!Owner->HasAuthority() && Owner->GetLocalRole() == ROLE_AutonomousProxy ) 
	{
		DetachWeaponFromOwnerServer(InputWeaponActor);
	}
	else if(Owner->HasAuthority())
	{
		DetachWeaponFromOwnerServer(InputWeaponActor);
	}


}

void UWeaponManager::DetachWeaponFromOwnerServer_Implementation(AWeaponActor* InputWeaponActor)
{

	FDetachmentTransformRules DetachRules = FDetachmentTransformRules(EDetachmentRule::KeepRelative,false);
	//false means Mofify() won't be called on "components concerned" when detaching
	
	SetWeaponVisibility(true,InputWeaponActor);
	InputWeaponActor->IsInWeaponInventory = false;
	InputWeaponActor->DetachFromActor(DetachRules);

	return;
}


void UWeaponManager::EquipWeapon(int32 Slot,bool bImmediatelyChangeActorSocket, bool bPlayEquipMontage )
{
	if(Slot < 0 || Slot>= Weapons.Num())
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("Slot is < 0 || Slot > WeaponSlot. EquipWeapon() Failed"))
		}
		
		return;
	}
	

	if(!Owner->HasAuthority() && Owner->GetLocalRole() == ROLE_AutonomousProxy )
	{
		EquipWeaponServer(Slot,bImmediatelyChangeActorSocket,bPlayEquipMontage);
	}
	else if(Owner->HasAuthority())
	{
		EquipWeaponServer(Slot,bImmediatelyChangeActorSocket,bPlayEquipMontage);
	}
	

}

void UWeaponManager::EquipWeaponMulticast_Implementation(bool bPlayMontage,AWeaponActor* InputWeaponActor)
{
	if(bPlayMontage)
	{
		PlayEquipMontage(CurrentEquippedWeaponType);
	}
	
	OnWeaponEquipped.Broadcast(InputWeaponActor);
	
	

}

void UWeaponManager::EquipWeaponServer_Implementation(int32 Slot, bool bImmediatelyChangeActorSocket, bool bPlayEquipMontage)
{
	AWeaponActor* WeaponActor = Weapons[Slot];

	if(!Check(WeaponActor) || !Owner)
	{
		return;
	}

	FName EquipLookupName = WeaponActor->WeaponData.WeaponType;

	if(!EquipDataDictionary.Contains(EquipLookupName))
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("EquipDataDictionary doesn't contain the given key. %s EquipWeapon() Failed"),*(EquipLookupName.ToString()))
		}
		
		return ;
	}

	FEquipData EquipData = EquipDataDictionary[EquipLookupName];
	
	if(bImmediatelyChangeActorSocket)
	{
	
		if(WeaponActor->IsAttachedTo(Owner))
		{
			DetachWeaponFromOwner(WeaponActor); //Need to detach before can change socket
		}

		AttachWeaponToOwner(WeaponActor,EquipData.ArmedSocket);
	
		
		SetWeaponVisibility(true,WeaponActor);
		ActiveWeapon = WeaponActor;
		ActiveWeaponSlot = Slot;
		CurrentEquippedWeaponType = EquipLookupName;
		bEquipState = true;
		WeaponActor->Equip(OwnerStatManager);
	
	
		EquipWeaponMulticast(bPlayEquipMontage,WeaponActor);
		
		
		
	
	}
	else
	{

		if(bPlayEquipMontage)
		{
			
			DelayedEquipSocketChangeSlot = Slot;
		}
		EquipWeaponMulticast(bPlayEquipMontage,WeaponActor);
	}
	

	return;
}

void UWeaponManager::HolsterWeapon(int32 Slot, bool bImmediatelyChangeActorSocket, bool bPlayHolsterMontage)
{
	if(Slot < 0 || Slot>= Weapons.Num())
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("Slot is < 0 || Slot > WeaponSlot. HolsterWeapon() Failed"))
		}
		
		return;
	}
	
	if(!Owner->HasAuthority() && Owner->GetLocalRole() == ROLE_AutonomousProxy ) 
	{
		HolsterWeaponServer(Slot,bImmediatelyChangeActorSocket,bPlayHolsterMontage);
	}
	else if(Owner->HasAuthority())
	{
		HolsterWeaponServer(Slot,bImmediatelyChangeActorSocket,bPlayHolsterMontage);
	}

	

}

void UWeaponManager::HolsterWeaponMulticast_Implementation(bool bPlayMontage,AWeaponActor* InputWeaponActor)
{
	if(bPlayMontage)
	{
		PlayHolsterMontage(CurrentEquippedWeaponType);
	}
	OnWeaponHolstered.Broadcast(InputWeaponActor);

}

void UWeaponManager::HolsterWeaponServer_Implementation(int32 Slot, bool bImmediatelyChangeActorSocket, bool bPlayHolsterMontage)
{
	AWeaponActor* WeaponActor = Weapons[Slot];

	if(!Check(WeaponActor))
	{
		return;
	}

	FName EquipLookupName = WeaponActor->WeaponData.WeaponType;

	if(!EquipDataDictionary.Contains(EquipLookupName))
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("EquipDataDictionary doesn't contain the given key. %s HolsterWeapon() Failed"),*(EquipLookupName.ToString()))
		}
		
		return;
	}
	FEquipData EquipData = EquipDataDictionary[EquipLookupName];


	

	
	if(bImmediatelyChangeActorSocket)
	{
		if(Owner)
		{
			if(WeaponActor->IsAttachedTo(Owner))
			{
				DetachWeaponFromOwner(WeaponActor); //Need to detach before can change socket
			}
		}
		AttachWeaponToOwner(WeaponActor,EquipData.HolsterSocket);
		
		if(bMakeHolsteredWeaponInvisible)
		{
			SetWeaponVisibility(false,WeaponActor);
		}
	
		if(Slot == ActiveWeaponSlot)
		{
			ActiveWeapon = nullptr;
			bEquipState = false;


		}
		HolsterWeaponMulticast(bPlayHolsterMontage,WeaponActor);
		

		
		
		

		WeaponActor->Holster(OwnerStatManager);
	}
	else
	{
		if(bPlayHolsterMontage)
		{
			DelayedHolsterSocketChangeSlot = Slot;
		}
		HolsterWeaponMulticast(bPlayHolsterMontage,WeaponActor);
	}
	
	return;
}

void UWeaponManager::IncreaseActiveSlotIndex(bool bPlayMontage)
{
	if(!Owner->HasAuthority() && Owner->GetLocalRole() == ROLE_AutonomousProxy ) 
	{
		IncreaseActiveSlotIndexServer(bPlayMontage);
	}
	else if(Owner->HasAuthority())
	{
		IncreaseActiveSlotIndexServer(bPlayMontage);
	}


	 
	
}

void UWeaponManager::IncreaseActiveSlotIndexServer_Implementation(bool bPlayMontage)
{	
	int32 NumberOfWeapons = Weapons.Num();

	if(NumberOfWeapons <= 1) //No weapon to change
	{
		return;
	}

	int32 NextWeaponSlot = (ActiveWeaponSlot + 1) % NumberOfWeapons;
	SetActiveEquipSlot(NextWeaponSlot,bPlayMontage);
}

bool UWeaponManager::IsWeaponVisible(AWeaponActor* InputWeaponActor)
{
	if(!InputWeaponActor)
	{
		return false;
	}
	return InputWeaponActor->GetRootComponent()->IsVisible();
}


void UWeaponManager::OnRep_ActiveWeapon()
{
	OnActiveWeaponChange.Broadcast(ActiveWeapon);
}

void UWeaponManager::OnRep_ActiveWeaponSlot()
{
	OnActiveWeaponChange.Broadcast(ActiveWeapon);
}
void UWeaponManager::OnRep_CurrentEquipWeaponType()
{
	OnNewWeaponEquip.Broadcast(CurrentEquippedWeaponType,bEquipState);
}
void UWeaponManager::OnRep_EquipState()
{
	OnNewWeaponEquip.Broadcast(CurrentEquippedWeaponType,bEquipState);
}



bool UWeaponManager::PlayEquipMontage(AWeaponActor* InputWeaponActor)
{
	if(!Check(InputWeaponActor))
	{
		return false;
	}
	else if(!EquipDataDictionary.Contains(InputWeaponActor->WeaponData.WeaponType))
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("Equip data %s not defined in EquipDataDictionary"),*(InputWeaponActor->WeaponData.WeaponType.ToString()))
		}
		
		return false;
	}
	FEquipData EquipData = EquipDataDictionary[InputWeaponActor->WeaponData.WeaponType];
	LastEquipMontage = EquipData.SoftEquipMontage.Get();
	float PlayLength = UWeaponManager::PlayMontage(Owner,EquipData.SoftEquipMontage.Get(),EquipData.EquipPlayrate,FName(""),bDebugLog);

	if(PlayLength == 0.0f)
	{
		return false;
	
	}

	return true;
}

bool UWeaponManager::PlayEquipMontage(FName WeaponType)
{
	if(!Owner)
	{
		return false;
	}
	
	if(!EquipDataDictionary.Contains(WeaponType))
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("Equip data %s not defined in EquipDataDictionary"),*(WeaponType.ToString()))
		}
		
		return false;
	}
	FEquipData EquipData = EquipDataDictionary[WeaponType];
	LastEquipMontage = EquipData.SoftEquipMontage.Get();
	float PlayLength = UWeaponManager::PlayMontage(Owner,EquipData.SoftEquipMontage.Get(),EquipData.EquipPlayrate,FName(""),bDebugLog);

	if(PlayLength == 0.0f)
	{
		return false;
	}

	return true;
}

bool UWeaponManager::PlayHolsterMontage(AWeaponActor* InputWeaponActor)
{
	if(!Check(InputWeaponActor))
	{
		return false;
	}
	else if(!EquipDataDictionary.Contains(InputWeaponActor->WeaponData.WeaponType))
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("Equip data %s not defined in EquipDataDictionary"),*(InputWeaponActor->WeaponData.WeaponType.ToString()))
		}
		
		return false;
	}
	FEquipData EquipData = EquipDataDictionary[InputWeaponActor->WeaponData.WeaponType];
	LastHolsterMontage = EquipData.SoftHolsterMontage.Get();
	float PlayLength = UWeaponManager::PlayMontage(Owner,EquipData.SoftHolsterMontage.Get(),EquipData.HolsterPlayrate,FName(""),bDebugLog);

	if(PlayLength == 0.0f)
	{
		return false;
	}

	return true;
}

bool UWeaponManager::PlayHolsterMontage(FName WeaponType)
{
	if(!Owner)
	{
		return false;
	}
	
	if(!EquipDataDictionary.Contains(WeaponType))
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("Equip data %s not defined in EquipDataDictionary"),*(WeaponType.ToString()))
		}
		
		return false;
	}
	FEquipData EquipData = EquipDataDictionary[WeaponType];
	LastHolsterMontage = EquipData.SoftHolsterMontage.Get();
	float PlayLength = UWeaponManager::PlayMontage(Owner,EquipData.SoftHolsterMontage.Get(),EquipData.HolsterPlayrate,FName(""),bDebugLog);

	if(PlayLength == 0.0f)
	{
		return false;
	}
	return true;
}


void UWeaponManager::SetActiveEquipSlotByType(FName NewWeaponType, bool bPlayEquipMontage )
{	
	int Index = 0;
	bool bFoundWeapon = false;
	for (const AWeaponActor* Wep : Weapons)
	{
		if(Wep)
		{
			if(Wep->WeaponData.WeaponType == NewWeaponType)
			{
				bFoundWeapon = true;
				break;
			}
		

		}

		Index++;
	}
	if(bFoundWeapon)
	{
		SetActiveEquipSlot(Index,bPlayEquipMontage);
	}

}

void UWeaponManager::SetActiveEquipSlot(int32 NewEquipSlot,bool bPlayEquipMontage )
{
	if(NewEquipSlot < 0 || NewEquipSlot >= Weapons.Num())
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("SetActiveEquipSlot() failed, NewEquipSlot < 0 or ActiveWeaponSlot == NewEquipSlot"))
		}
		return;
	}

	if(!Owner->HasAuthority() && Owner->GetLocalRole() == ROLE_AutonomousProxy ) 
	{
		SetActiveEquipSlotServer(NewEquipSlot,bPlayEquipMontage);
	}
	else if(Owner->HasAuthority() )
	{
		SetActiveEquipSlotServer(NewEquipSlot,bPlayEquipMontage);
	}

	


}

void UWeaponManager::SetActiveEquipSlotServer_Implementation(int32 NewEquipSlot, bool bPlayEquipMontage)
{
	if(NewEquipSlot >= Weapons.Num())
	{
		if(bDebugLog)
		{
			UE_LOG(LogTemp,Warning,TEXT("SetActiveEquipSlot() failed, NewEquipSlot not an index in Weapons TArray"))
		}
		return;
	}
	HolsterWeapon(ActiveWeaponSlot);
	EquipWeapon(NewEquipSlot,true, bPlayEquipMontage );
}


void UWeaponManager::SetWeaponVisibility(bool NewWeaponVisibility,AWeaponActor* InputWeaponActor)
{
	if(!Check(InputWeaponActor))
	{
		return;
	}

	if(!Owner->HasAuthority() && Owner->GetLocalRole() == ROLE_AutonomousProxy ) 
	{
		SetWeaponVisibilityServer(NewWeaponVisibility,InputWeaponActor);
	}
	else if(Owner->HasAuthority())
	{
		SetWeaponVisibilityServer(NewWeaponVisibility,InputWeaponActor);
	}



}

void UWeaponManager::SetWeaponVisibilityMulticast_Implementation(bool NewWeaponVisibility,AWeaponActor* InputWeaponActor)
{

	if(IsWeaponVisible(InputWeaponActor) == NewWeaponVisibility)
	{
		return;
	}

	InputWeaponActor->GetRootComponent()->SetVisibility(NewWeaponVisibility,true); //true means propogate to children
}


void UWeaponManager::SetWeaponVisibilityServer_Implementation(bool NewWeaponVisibility,AWeaponActor* InputWeaponActor)
{
	SetWeaponVisibilityMulticast(NewWeaponVisibility,InputWeaponActor);
}


void UWeaponManager::SpawnWeapon(TSubclassOf<class AWeaponActor> WeaponType)
{
	if(!Owner)
	{
		return;
	}

	if(!Owner->HasAuthority() && Owner->GetLocalRole() == ROLE_AutonomousProxy ) 
	{
		SpawnWeaponServer(WeaponType);
	}
	else if(Owner->HasAuthority())//SERVER STUFF ONLY
	{
		SpawnWeaponServer(WeaponType);
	}
	

}


void UWeaponManager::SpawnWeaponServer_Implementation(TSubclassOf<class AWeaponActor> WeaponType)
{
	FVector SpawnLocation = FVector(0.0f,0.0f,0.0f);
	SpawnLocation = Owner->GetActorLocation() + FVector(0.0f,0.0f,600.0f);

	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AWeaponActor* NewWeaponActor = GetWorld()->SpawnActor<AWeaponActor>(WeaponType,SpawnLocation,FRotator(0.0f,0.0f,0.0f),ActorSpawnParams);

	if(NewWeaponActor)
	{
		AddWeaponToArray(NewWeaponActor); //Lets see if this replicates correctly
	}
}



TArray<FWeaponSaveData> UWeaponManager::GenerateWeaponSaveData() const
{


	TArray<FWeaponSaveData> WeaponSaveData = {};

	for (AWeaponActor* WeaponActor : Weapons)
	{
		if(!WeaponActor)
		{
			continue;
		}
		FWeaponSaveData NewSaveData = FWeaponSaveData();
		NewSaveData.WeaponClass = WeaponActor->GetClass();
		if(WeaponActor->WeaponStatManager)
		{
			NewSaveData.WeaponStats = WeaponActor->WeaponStatManager->StatDictionary;
			NewSaveData.WeaponStatusEffects = WeaponActor->WeaponStatManager->GenerateStatusEffectSaveData();
			NewSaveData.WeaponStatBindings = WeaponActor->WeaponStatManager->StatBindings;
		}
		WeaponSaveData.Add(NewSaveData);
		

	}
	return WeaponSaveData;
}

void UWeaponManager::SpawnWeaponsFromSavedData(const TArray<FWeaponSaveData>& NewWeaponData, int32 NewActiveWeaponSlot  )
{
	
	//1) Destroy all actors in the weapon inventory
	//2) Spawn New weapon, add it to weapon dictionary, apply save data
	//3) Set weapon slot
	//4) Equip weapon
	



	for (AWeaponActor* Weapon : Weapons)
	{
		if(!Weapon)
		{
			continue;
		}
		Weapon->Destroy();
	}
	ActiveWeapon = nullptr;
	Weapons.Empty(0);


	for (const FWeaponSaveData& WeaponData : NewWeaponData)
	{
		SpawnWeapon(WeaponData.WeaponClass);

	}

	ActiveWeaponSlot = NewActiveWeaponSlot;
	EquipWeapon(ActiveWeaponSlot,true,false);

	OnFinishSpawningWeapons.Broadcast();


}

void UWeaponManager::LoadWeaponSavedData(UPARAM(ref) const TArray<FWeaponSaveData>& NewWeaponData)
{
	for (int32 i = 0; i < Weapons.Num(); i++)
	{
		if(i < NewWeaponData.Num() && Weapons[i])
		{
			if(Weapons[i]->WeaponStatManager)
			{
				Weapons[i]->WeaponStatManager->LoadSaveData(NewWeaponData[i].WeaponStats,NewWeaponData[i].WeaponStatBindings,NewWeaponData[i].WeaponStatusEffects);
			}
		}
	}
}


