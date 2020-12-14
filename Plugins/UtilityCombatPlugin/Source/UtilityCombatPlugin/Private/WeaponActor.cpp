// Copyright Zachary Kolansky, 2020


#include "WeaponActor.h"
#include "StatManager.h"
#include "WeaponFiringComponent.h"
#include "MeleeMontageComponent.h"
#include "Net/UnrealNetwork.h"


AWeaponActor::AWeaponActor(const FObjectInitializer& ObjectInitializer)
{
    WeaponStatManager = CreateDefaultSubobject<UStatManager>(TEXT("WeaponStats"));
    bReplicates = true;
    SetReplicateMovement(true);
    /*
    USkeletalMeshComponent* SMC = GetSkeletalMeshComponent();
    if(SMC)
    {
        SMC->SetIsReplicated(true); //Needs to be true so visibility will change as appropiate
    }
    */
}

void AWeaponActor::BeginPlay()
{
    Super::BeginPlay();
    PrimaryWeaponFiringComponent = FindComponentByClass<UWeaponFiringComponent>();
    MeleeMontageComponent = FindComponentByClass<UMeleeMontageComponent>();
} 

void AWeaponActor::GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}


UNetConnection * AWeaponActor::GetNetConnection() const 
{
    
    if(AActor* Owner = GetAttachParentActor())
    {
        return Owner->GetNetConnection();
    }

    return Super::GetNetConnection();
}

bool AWeaponActor::SetStatusOfStatDictionary(bool bNewStatus)
{

    if(!WeaponStatManager)
    {
        return false;
    }
    WeaponStatManager->bStatDictionaryCanModifyOtherStatDictionary = bNewStatus;
    return true;
}




void AWeaponActor::Equip(UStatManager* WielderWeaponStatManager)
{
   
    
    
    if(HasAuthority())
    {
        EquipMulticast(WielderWeaponStatManager);
    }
    else if(!HasAuthority() && GetLocalRole() == ROLE_AutonomousProxy)
    {
        EquipServer(WielderWeaponStatManager);
    }
   
    
}


void AWeaponActor::EquipMulticast_Implementation(UStatManager* WielderWeaponStatManager)
{
    IsInWeaponInventory = true;
    if(WielderWeaponStatManager && !WielderWeaponStatManager->OtherStatManagers.Contains(WeaponStatManager))
    {
        WielderWeaponStatManager->OtherStatManagers.Add(WeaponStatManager);
        
    }
    SetStatusOfStatDictionary(true);
    OnWeaponEquipped.Broadcast();
}

void AWeaponActor::EquipServer_Implementation(UStatManager* WielderWeaponStatManager)
{
    EquipMulticast(WielderWeaponStatManager);
}

void AWeaponActor::FireWeapon_Implementation()
{
    if(PrimaryWeaponFiringComponent)
    {
        if(PrimaryWeaponFiringComponent->bAutomaticFire)
        {
            PrimaryWeaponFiringComponent->StartFiringWeaponComponent();
        }
        else
        {
            PrimaryWeaponFiringComponent->FireWeaponComponent();
        }
        
    }
}

void AWeaponActor::FireWeaponEnd_Implementation()
{
    if(PrimaryWeaponFiringComponent)
    {
        PrimaryWeaponFiringComponent->StopFiringWeaponComponent();
        
    }
}

bool AWeaponActor::HasFiringComponent()
{
    if(PrimaryWeaponFiringComponent)
    {
        return true;
    }
    return false;
}


bool AWeaponActor::HasMeleeComponent()
{
    if(MeleeMontageComponent)
    {
        return true;
    }
    return false;
}

void AWeaponActor::Holster(UStatManager* WielderWeaponStatManager)
{

    
    if(HasAuthority())
    {
        HolsterMulticast(WielderWeaponStatManager);
    }
    else if(!HasAuthority() && GetLocalRole() == ROLE_AutonomousProxy)
    {
        HolsterServer(WielderWeaponStatManager);
    }
}


void AWeaponActor::HolsterMulticast_Implementation(UStatManager* WielderWeaponStatManager)
{
    IsInWeaponInventory = true;
    if(WielderWeaponStatManager && !WielderWeaponStatManager->OtherStatManagers.Contains(WeaponStatManager))
    {
        WielderWeaponStatManager->OtherStatManagers.Add(WeaponStatManager);
    }
    SetStatusOfStatDictionary(false);
    OnWeaponHolstered.Broadcast();
}

void AWeaponActor::HolsterServer_Implementation(UStatManager* WielderWeaponStatManager)
{
    HolsterMulticast(WielderWeaponStatManager);
}

void AWeaponActor::ReloadWeapon_Implementation()
{
    
}

void AWeaponActor::Remove(UStatManager* WielderWeaponStatManager)
{
    if(!WielderWeaponStatManager) {return;}

    if(HasAuthority())
    {
        RemoveMulticast(WielderWeaponStatManager);
    }
    else if(!HasAuthority() && GetLocalRole() == ROLE_AutonomousProxy)
    {
        RemoveServer(WielderWeaponStatManager);
    }
}


void AWeaponActor::RemoveMulticast_Implementation(UStatManager* WielderWeaponStatManager)
{
    IsInWeaponInventory = false;
    if(WielderWeaponStatManager->OtherStatManagers.Contains(WeaponStatManager))
    {
        WielderWeaponStatManager->OtherStatManagers.Remove(WeaponStatManager);
    }
    SetStatusOfStatDictionary(false);
}

void AWeaponActor::RemoveServer_Implementation(UStatManager* WielderWeaponStatManager)
{
    RemoveMulticast(WielderWeaponStatManager);
}
