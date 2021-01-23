// Copyright Zachary Kolansky, 2020


#include "MeleeMontageComponent.h"
#include "GameFramework/Character.h"
#include "Runtime/Engine/Classes/GameFramework/Controller.h"
#include "Runtime/Core/Public/Math/UnrealMathUtility.h"
#include "WeaponManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "WeaponActor.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UMeleeMontageComponent::UMeleeMontageComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	

}


// Called when the game starts
void UMeleeMontageComponent::BeginPlay()
{
	Super::BeginPlay();
	
	AWeaponActor* OwnerAsWeapon = Cast<AWeaponActor>(GetOwner());
	if(OwnerAsWeapon)
	{
		OwnerAsWeapon->OnWeaponEquipped.AddUniqueDynamic(this,&UMeleeMontageComponent::Initialize);
		//WeaponComponent needs to re-check for Owner Variables when its Equipped.
	}
	
	Initialize();
	// ...
	
}

void UMeleeMontageComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(AutoFireHandle);
	//GetWorld()->GetTimerManager().ClearTimer(BurstFireHandle);
	Super::EndPlay(EndPlayReason);
}

void UMeleeMontageComponent::GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

// Called every frame
void UMeleeMontageComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UMeleeMontageComponent::Initialize()
{
	OwnerWeapon = GetOwner();
	if(!OwnerWeapon)
	{
		return;
	}
	
	if(!GetOwner()->GetAttachParentActor()) //Not attached
	{
		OwnerChar = Cast<ACharacter>(GetOwner());
		if(OwnerChar)
		{
			OwnerController = OwnerChar->GetController();
		}
	}
	else //attached to a weapon actor that is attached to some character
	{
		OwnerChar = Cast<ACharacter>(GetOwner()->GetAttachParentActor());
		if(OwnerChar)
		{
			OwnerController = OwnerChar->GetController();
		}
	}
	
	WorldTimerManager = &(GetWorld()->GetTimerManager()); 
	
	
	if(OwnerChar)
	{
		OwnerMeshComp = OwnerChar->GetMesh();
		if(OwnerMeshComp)
		{
			OwnerCharacterAnimInst = OwnerMeshComp->GetAnimInstance();
		}
	
	}
	


}


bool UMeleeMontageComponent::CanPlayMontageData(const FMeleeMontageCollectionData& MontageCollectionData, bool& bPassCooldownCheck, bool& bPassGlobalBlackList, bool& bPassInternalBlackList) const
{
	bool bPassAllChecks = false;
	bPassCooldownCheck = false;
	bPassGlobalBlackList = false;
	bPassInternalBlackList = false;

	float CurrentTime = GetWorld()->GetTimeSeconds();

	if(WorldTimeLastMontage <= 0.0f || Cooldown <= 0.0f)
	{
		bPassCooldownCheck = true;
	}
	else if(CurrentTime - WorldTimeLastMontage >= Cooldown)
	{
		bPassCooldownCheck = true;
	}

	UAnimMontage* Montage = MontageCollectionData.SoftMontage.Get();
	if(OwnerCharacterAnimInst && Montage)
	{
		bool bMontagePlaying = OwnerCharacterAnimInst->Montage_IsPlaying(Montage);
		if(bMontagePlaying)
		{
			return false;
		}
	}



	if(MontageCollectionData.MontagePlayType == EMontagePlayType::IF_NO_BLACKLIST_MONTAGES)
	{
		bPassInternalBlackList = !IsMontageInListPlaying(MontageCollectionData.SoftBlackListMontages);
		bPassGlobalBlackList = !IsMontageInListPlaying(SoftGlobalBlackListMontages);
	}
	else if(MontageCollectionData.MontagePlayType == EMontagePlayType::IF_NO_BLACKLIST_MONTAGES_IGNORE_GLOBAL_BLACKLIST)
	{
		bPassGlobalBlackList = true;
		bPassInternalBlackList = !IsMontageInListPlaying(MontageCollectionData.SoftBlackListMontages);

	}
	else if(MontageCollectionData.MontagePlayType == EMontagePlayType::ALWAYS)
	{
		bPassGlobalBlackList = true;
		bPassInternalBlackList = true;
	}

	if(bPassCooldownCheck && bPassGlobalBlackList && bPassInternalBlackList)
	{
		bPassAllChecks = true;
	}



	return bPassAllChecks;
}


TArray<TSoftObjectPtr<UAnimMontage>> UMeleeMontageComponent::GetMontageListFromCollection(UPARAM(ref)const TArray<FMeleeMontageCollectionData>& CollectionArray) const
{
	TArray<TSoftObjectPtr<UAnimMontage>> OutList = {};

	for (const FMeleeMontageCollectionData Collection : CollectionArray)
	{
		OutList.Add(Collection.SoftMontage);
	}

	return OutList;
}

bool UMeleeMontageComponent::IsMontageInListPlaying(const TArray<TSoftObjectPtr<UAnimMontage>>& MontageList) const
{
	bool bMontagePlaying = false;
	if(!OwnerCharacterAnimInst)
	{
		return false;
	}

	for (const TSoftObjectPtr<UAnimMontage> SoftMontage : MontageList )
	{
		UAnimMontage* Montage = SoftMontage.Get();
		if(Montage)
		{
			bMontagePlaying = OwnerCharacterAnimInst->Montage_IsPlaying(Montage);
		}

		if(bMontagePlaying)
		{
			break;
		}
	}

	return bMontagePlaying;
}

void UMeleeMontageComponent::PlayCollectionMontage(const FMeleeMontageCollectionData& MontageCollectionData) 
{
	bool bPassCooldownCheck = false;
	bool bPassGlobalBlackList = false;
	bool bPassInternalBlackList = false;
	
	bool bPass = CanPlayMontageData(MontageCollectionData, bPassCooldownCheck, bPassGlobalBlackList, bPassInternalBlackList);

	if(bPass)
	{	
		LastMontagePlayed = MontageCollectionData.SoftMontage.Get();
		PlayMontage(MontageCollectionData.SoftMontage.Get(),MontageCollectionData.MontagePlayrate,MontageCollectionData.MontageStartSectionName);

		if(MontageCollectionData.InterruptOtherMontages)
		{
			for (const TSoftObjectPtr<UAnimMontage> SoftMontage : MontageCollectionData.OtherMontagesToStop )
			{
				bool bMontagePlaying = false;
				UAnimMontage* Montage = SoftMontage.Get();
				if(Montage)
				{
					bMontagePlaying = OwnerCharacterAnimInst->Montage_IsPlaying(Montage);
				}

				if(bMontagePlaying)
				{
					StopMontage(MontageCollectionData.MontageInterruptBlendOutTime,Montage);
				}
			}
			if(MontageCollectionData.OtherMontagesToStop.Num() == 0)
			{
				StopMontage(MontageCollectionData.MontageInterruptBlendOutTime,nullptr); //will stop all montages
			}
		}

	}
}

void UMeleeMontageComponent::PlayNextMontage()
{
	if(CurrentMontageIndex >= MontagesToPlay.Num() || CurrentMontageIndex < 0)
	{
		CurrentMontageIndex = 0;
	}
	

	const FMeleeMontageCollectionData& MontageCollectionData = MontagesToPlay[CurrentMontageIndex];
	PlayCollectionMontage(MontageCollectionData);
	





	CurrentMontageIndex++;
}


void UMeleeMontageComponent::PlayMontage(UAnimMontage* PlayMontage, float Playrate , FName StartSection)
{
	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy)
	{
		PlayMontageServer(PlayMontage,Playrate,StartSection);
	}
	else if(OwnerWeapon && OwnerWeapon->HasAuthority())
	{
		PlayMontageServer(PlayMontage,Playrate,StartSection);
	}
	else if(OwnerChar && OwnerChar->HasAuthority() )
	{
		PlayMontageServer(PlayMontage,Playrate,StartSection);
	}
}


void UMeleeMontageComponent::PlayMontageMulticast_Implementation(UAnimMontage* PlayMontage, float Playrate , FName StartSection)
{
	UWeaponManager::PlayMontage(OwnerChar,PlayMontage,Playrate,StartSection,bShowDebugWarnings);
}


void UMeleeMontageComponent::PlayMontageServer_Implementation(UAnimMontage* PlayMontage, float Playrate , FName StartSection)
{
	PlayMontageMulticast(PlayMontage,Playrate,StartSection);
}

void UMeleeMontageComponent::StopMontage(float BlendOutTime, UAnimMontage* MontageToStop)
{
	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy)
	{
		StopMontageServer(BlendOutTime,MontageToStop);
	}
	else if(OwnerWeapon && OwnerWeapon->HasAuthority())
	{
		StopMontageServer(BlendOutTime,MontageToStop);
	}
	else if(OwnerChar && OwnerChar->HasAuthority() )
	{
		StopMontageServer(BlendOutTime,MontageToStop);
	}
}


void UMeleeMontageComponent::StopMontageMulticast_Implementation(float BlendOutTime,UAnimMontage* MontageToStop)
{

	if(OwnerCharacterAnimInst)
	{
		OwnerCharacterAnimInst->Montage_Stop(BlendOutTime,MontageToStop);
	}
		
}


void UMeleeMontageComponent::StopMontageServer_Implementation(float BlendOutTime,UAnimMontage* MontageToStop)
{
	StopMontageMulticast(BlendOutTime,MontageToStop);
}


void UMeleeMontageComponent::ResetMontageIndex(int32 NewIndex)
{
	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ResetMontageIndexServer(NewIndex);
	}
	else if(OwnerWeapon && OwnerWeapon->HasAuthority())
	{
		ResetMontageIndexServer(NewIndex);
	}
	else if(OwnerChar && OwnerChar->HasAuthority() )
	{
		ResetMontageIndexServer(NewIndex);
	}
}


void UMeleeMontageComponent::ResetMontageIndexMulticast_Implementation(int32 NewIndex)
{
	if(NewIndex >= 0 && NewIndex < MontagesToPlay.Num())
	{
		CurrentMontageIndex = NewIndex;
	}
}


void UMeleeMontageComponent::ResetMontageIndexServer_Implementation(int32 NewIndex )
{
	ResetMontageIndexMulticast(NewIndex);
}

/*
AUTOMATIC
*/


void UMeleeMontageComponent::StartAutoMelee()
{
	if(!AutoFireDelegate.IsBound())
	{
		AutoFireDelegate.BindUFunction(this,FName("PlayNextMontage"));
		
	}
	
	if(!WorldTimerManager->IsTimerActive(AutoFireHandle))
	{
		PlayNextMontage();
		WorldTimerManager->SetTimer(AutoFireHandle,AutoFireDelegate,AutoSwingFireFrequency,true);
	}
}


void UMeleeMontageComponent::StopAutoMelee(bool bStopLastPlayedMontage, float BlendOutTime)
{
	if(!WorldTimerManager)
	{
		return;
	}

	WorldTimerManager->ClearTimer(AutoFireHandle);

	if(BlendOutTime < 0.0f)
	{
		BlendOutTime = 0.0f;
	}

	if(!LastMontagePlayed || !bStopLastPlayedMontage)
	{
		return;
	}

	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy)
	{
		StopMontage(BlendOutTime,LastMontagePlayed);
	}
	else if(OwnerWeapon && OwnerWeapon->HasAuthority())
	{
		StopMontage(BlendOutTime,LastMontagePlayed);
	}

}