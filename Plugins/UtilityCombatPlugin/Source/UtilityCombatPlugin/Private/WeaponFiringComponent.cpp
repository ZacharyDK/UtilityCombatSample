// Copyright Zachary Kolansky, 2020

#include "WeaponFiringComponent.h"
#include "GameFramework/Character.h"
#include "Runtime/Engine/Classes/GameFramework/Controller.h"
#include "Runtime/Core/Public/Math/UnrealMathUtility.h"
#include "WeaponManager.h"
#include "Net/UnrealNetwork.h"
#include "Runtime/Engine/Public/DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "WeaponActor.h"

// Sets default values for this component's properties
UWeaponFiringComponent::UWeaponFiringComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	AWeaponActor* OwnerAsWeapon = Cast<AWeaponActor>(GetOwner());
	if(OwnerAsWeapon)
	{
		OwnerAsWeapon->OnWeaponEquipped.AddUniqueDynamic(this,&UWeaponFiringComponent::Initialize);
		//WeaponComponent needs to re-check for Owner Variables when its Equipped.
	}
}


// Called when the game starts
void UWeaponFiringComponent::BeginPlay()
{
	Super::BeginPlay();
	Initialize();
	// ...
	
}

void UWeaponFiringComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(AutoFireHandle);
	GetWorld()->GetTimerManager().ClearTimer(BurstFireHandle);
	Super::EndPlay(EndPlayReason);
}

void UWeaponFiringComponent::GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWeaponFiringComponent,ClipAmmunition);
	DOREPLIFETIME(UWeaponFiringComponent,ReserveAmmunition);


	
}

// Called every frame
void UWeaponFiringComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UWeaponFiringComponent::Initialize()
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
	
	


}

FRotator UWeaponFiringComponent::AddRandomSpreadToRotation(const FRotator InputRotation) const
{
	FRotator NewRotation = InputRotation;

	switch(FiringData.SpreadType)
	{
		case ERandomSpreadType::NoSpread:
		{
			break;
		}
		case ERandomSpreadType::RandomSpread:
		{

			float RandomRoll = FMath::RandRange(FiringData.MinRandomDegreeSpread,FiringData.MaxRandomDegreeSpread);
			float RandomPitch = FMath::RandRange(FiringData.MinRandomDegreeSpread,FiringData.MaxRandomDegreeSpread);
			float RandomYaw = FMath::RandRange(FiringData.MinRandomDegreeSpread,FiringData.MaxRandomDegreeSpread);
			FRotator RandomRotation = FRotator(RandomPitch,RandomYaw,RandomRoll);
			NewRotation += RandomRotation;
		}
	}

	return NewRotation;
}

bool UWeaponFiringComponent::CanFire(int32 BulletsToFire, bool& bPassTimingCheck, bool& bPassAmmunitionConsumptionCheck )
{
	bPassTimingCheck = false;
	bPassAmmunitionConsumptionCheck = false;

	float CurrentTime = GetWorld()->GetTimeSeconds();
	


	if(WorldTimeLastFired <= 0.0f || Cooldown <= 0.0f)
	{
		bPassTimingCheck = true;
	}
	else if(CurrentTime - WorldTimeLastFired >= Cooldown)
	{
		bPassTimingCheck = true;
	}
	

	int32 TotalBullets = BulletsToFire;

	if(bOneAmmoPerShot)
	{
		TotalBullets = 1; //Each multishot counts as one.
	}

	if(StyleType == EFiringStyle::BurstFire)
	{
		TotalBullets = TimesToFirePerBurst*BulletsToFire; //Still care if we have enough ammo to burst fire.
	}


	if(TotalBullets <= ClipAmmunition && ClipAmmunition > 0)
	{
		bPassAmmunitionConsumptionCheck = true;
	}
	else if(bUnlimitedClip)
	{
		bPassAmmunitionConsumptionCheck = true;
	}

	if(bPassTimingCheck && bPassAmmunitionConsumptionCheck)
	{
		return true;
	}

	return false;
}


bool UWeaponFiringComponent::CanReload() const
{

	return (!IsReloading() && ReserveAmmunition >= ClipSize);
}

FRotator UWeaponFiringComponent::ClampRotation(const FRotator InputRotator) const
{
	float Pitch = InputRotator.Pitch;
	float Yaw = InputRotator.Yaw;
	float Roll = InputRotator.Roll;

	
	switch(ClampType)
	{

		case ERotationClampType::NoClamp:
		{
			break;
		}
		case ERotationClampType::Pitch:
		{
			Pitch = FMath::Clamp(Pitch,MinPitch,MaxPitch);
			break;
		}
		case ERotationClampType::Roll:
		{
			Roll = FMath::Clamp(Roll,MinRoll,MaxRoll);
			break;
		}
		case ERotationClampType::Yaw:
		{
			Yaw = FMath::Clamp(Yaw,MinYaw,MaxYaw);
			break;
		}
		
		case ERotationClampType::PitchRoll:
		{
			Pitch = FMath::Clamp(Pitch,MinPitch,MaxPitch);
			Roll = FMath::Clamp(Roll,MinRoll,MaxRoll);
			break;
		}
		case ERotationClampType::PitchYaw:
		{
			Pitch = FMath::Clamp(Pitch,MinPitch,MaxPitch);
			Yaw = FMath::Clamp(Yaw,MinYaw,MaxYaw);
			break;
		}
		case ERotationClampType::RollYaw:
		{
			Roll = FMath::Clamp(Roll,MinRoll,MaxRoll);
			Yaw = FMath::Clamp(Yaw,MinYaw,MaxYaw);
			break;
		}
		case ERotationClampType::PitchRollYaw:
		{
			Pitch = FMath::Clamp(Pitch,MinPitch,MaxPitch);
			Roll = FMath::Clamp(Roll,MinRoll,MaxRoll);
			Yaw = FMath::Clamp(Yaw,MinYaw,MaxYaw);
			break;
		}

	}
	
	//TODO
	return FRotator(Pitch,Yaw,Roll);
}


void UWeaponFiringComponent::FireLine(const FVector& SpawnLocation, const FRotator& InitialRotation, int32 ProjectilesPerSide, bool bFireMiddleShot) const
{
	
	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy ) //is controlled client,
	{
		FireLineServer(SpawnLocation, InitialRotation,ProjectilesPerSide,bFireMiddleShot );
	}
	else if(OwnerChar && OwnerChar->HasAuthority()) //
	{
		FireLineServer(SpawnLocation, InitialRotation,ProjectilesPerSide,bFireMiddleShot );
	}


}


void UWeaponFiringComponent::FireLineServer_Implementation(const FVector& SpawnLocation, const FRotator& InitialRotation, int32 ProjectilesPerSide, bool bFireMiddleShot ) const
{
	FVector RightVector = GetRightVector();

	for (int32 i = 1; i <= ProjectilesPerSide; i++)
	{
		FVector SpawnLocationProjectileA = SpawnLocation + RightVector*FiringData.BulletSpacingForNonSpreadPattern*i;
		FVector SpawnLocationProjectileB = SpawnLocation + RightVector*FiringData.BulletSpacingForNonSpreadPattern*-1*i;

		FRotator InitialRotationProjectileA = AddRandomSpreadToRotation(InitialRotation);
		FRotator InitialRotationProjectileB = AddRandomSpreadToRotation(InitialRotation);

		InitialRotationProjectileA = ClampRotation(InitialRotationProjectileA);
		InitialRotationProjectileB = ClampRotation(InitialRotationProjectileB);

		if(bUseLineTracesInsteadOfProjectiles)
		{
			FireLineTraceServer(SpawnLocationProjectileA,InitialRotationProjectileA);
			FireLineTraceServer(SpawnLocationProjectileB,InitialRotationProjectileB);
		}
		else
		{
			FireSingleProjectileServer(SpawnLocationProjectileA,InitialRotationProjectileA);
			FireSingleProjectileServer(SpawnLocationProjectileB,InitialRotationProjectileB);
		}
		
		


	}
	if(bFireMiddleShot)
	{
		FRotator SingleShotRotation = InitialRotation;
		SingleShotRotation = AddRandomSpreadToRotation(SingleShotRotation);
		SingleShotRotation = ClampRotation(SingleShotRotation);
		
		if(bUseLineTracesInsteadOfProjectiles)
		{
			FireLineTraceServer(SpawnLocation,SingleShotRotation);
		}
		else
		{
			FireSingleProjectileServer(SpawnLocation,SingleShotRotation);
		}
		
	
	}
}

void UWeaponFiringComponent::FireLineTrace(const FVector StartLocation, const FRotator InitialRotation) const
{
	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy ) //is controlled client,
	{
		FireLineTraceServer(StartLocation, InitialRotation );
	}
	else if(OwnerChar && OwnerChar->HasAuthority()) 
	{
		FireLineTraceServer(StartLocation, InitialRotation );
	}
}


void UWeaponFiringComponent::FireLineTraceServer_Implementation(const FVector StartLocation, const FRotator InitialRotation) const
{
	FHitResult OutHitResult = FHitResult();
	
	FVector EndLocation = StartLocation + InitialRotation.Vector()*LineTraceDistance;
	 
	GetWorld()->LineTraceSingleByChannel(
		OutHitResult,
		StartLocation,
		EndLocation,
		ECollisionChannel::ECC_Camera
	);

	if(bShowDebugLineTrace)
	{
		DrawDebugLine(
			GetWorld(),
			StartLocation,
			EndLocation,
			DebugLineTraceColor,
			bDebugLineTracePersistent,
			1.0f,
			1,
			3.0f
		);
	}

	if(AActor* OutHitActor = OutHitResult.GetActor())
	{
		FVector HitFromDirection = FVector(0.0f,0.0f,0.0f);

		FVector OutHitActorForward = OutHitActor->GetActorForwardVector(); //This calculates the HitDirection
		FRotator LookRotation = UKismetMathLibrary::FindLookAtRotation(OutHitActorForward,OutHitResult.ImpactPoint);

		HitFromDirection = LookRotation.Vector();
		
		UGameplayStatics::ApplyPointDamage(

			OutHitActor,
			LineTraceDamage,
			HitFromDirection,
			OutHitResult,
			OwnerController,
			OwnerWeapon,
			LineTraceDamageTypeClass
		);
	}
}

void UWeaponFiringComponent::FirePattern() 
{
	if(IsReloading())
	{
		return;
	}
	
	bool bPassAmmoCheck = false;
	bool bPassTimingCheck = false;
	bool bCanFire = false;

	bCanFire = CanFire(FiringData.ProjectilesPerShot,bPassTimingCheck,bPassAmmoCheck);

	if(!bCanFire && !bPassAmmoCheck)
	{
		if(bShowDebugWarnings && bAutomaticReload)
		{
			UE_LOG(LogTemp,Warning,TEXT("Can't Fire... trying to reload!"))
		}
		
		
		if(CanReload() && bAutomaticReload)
		{
			ReloadWeapon(true,true);
		}
		
		return;
	}
	else if(!bCanFire)
	{
		if(bShowDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("Can't Fire, bCanFire is % s "),  (bCanFire ? TEXT("true") : TEXT("false") ) )
			UE_LOG(LogTemp,Warning,TEXT("Can't Fire, bPassTimingCheck is % s "),  (bPassTimingCheck ? TEXT("true") : TEXT("false") ) )
			UE_LOG(LogTemp,Warning,TEXT("Can't Fire, bPassAmmoCheck is % s "),  (bPassAmmoCheck ? TEXT("true") : TEXT("false") ) )
		}
		return;
	}

	

	if(WorldTimerManager && Bursts >= TimesToFirePerBurst) //If we Burst fired enough
	{
		Bursts = 0;
		WorldTimerManager->PauseTimer(BurstFireHandle);
		return;
	}
	
	
	FRotator InitialRotation = GetFiringInitialRotation(RotationType);

	FVector SpawnLocation = GetComponentLocation();

	bool bFireMiddleShot = false;
	int32 ProjectilesPerSide = 0;

	if(FiringData.ProjectilesPerShot % 2 != 0) //IF ODD
	{
		bFireMiddleShot = true; //Odd number get the middle shot, as FireSingleProjectile()
	}
	
	if(bFireMiddleShot)
	{
		//ODD case
		ProjectilesPerSide = (FiringData.ProjectilesPerShot - 1)/2;
	}
	else
	{
		//EVEN
		ProjectilesPerSide = (FiringData.ProjectilesPerShot)/2;
	}
	

	switch(FiringData.PatternType)
	{
		case  EPatternType::Spread:
			FireSpread(SpawnLocation,InitialRotation,ProjectilesPerSide,bFireMiddleShot);
			break;
		case  EPatternType::Line:
			FireLine(SpawnLocation,InitialRotation,ProjectilesPerSide,bFireMiddleShot);
			break;
		case EPatternType::Staggered:
			FireStaggered(SpawnLocation,InitialRotation,ProjectilesPerSide,bFireMiddleShot);
			break;
		default:
			if(bUseLineTracesInsteadOfProjectiles)
			{
				FireLineTrace(SpawnLocation,InitialRotation);
			}
			else
			{
				FireSingleProjectile(SpawnLocation,InitialRotation);
			}
			break;
	}

	if(StyleType == EFiringStyle::BurstFire)
	{
		Bursts++;
	}

	if(!bUnlimitedClip)
	{
		if(bOneAmmoPerShot)
		{
			ClipAmmunition = ClipAmmunition-1;
		}
		else
		{
			ClipAmmunition = ClipAmmunition-FiringData.ProjectilesPerShot;
		}
	}
	if(CanReload() && bAutomaticReload && ClipAmmunition <= 0)
	{
		ReloadWeapon(true,true);
	}
	
	WorldTimeLastFired = GetWorld()->GetTimeSeconds();
	OnWeaponFire.Broadcast();
}


void UWeaponFiringComponent::FireSingleProjectile(const FVector SpawnLocation, const FRotator SpawnRotation ) const 
{
	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy ) //is controlled client,
	{
		FireSingleProjectileServer(SpawnLocation, SpawnRotation );
	}
	else if(OwnerChar && OwnerChar->HasAuthority()) 
	{
		FireSingleProjectileServer(SpawnLocation, SpawnRotation );
	}
	
	
}

void UWeaponFiringComponent::FireSingleProjectileServer_Implementation(const FVector SpawnLocation, const FRotator SpawnRotation) const
{
	TSubclassOf<AActor> ProjectileClass = FiringData.ProjectileSoftClass.Get();
	
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();

		if (World != NULL)
		{

			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;

			if(OwnerController)
			{
				ActorSpawnParams.Instigator = OwnerController->GetPawn();
			}

			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

			AActor* Projectile = World->SpawnActor<AActor>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);

			
		}
	}
	else 
	{
		UE_LOG(LogTemp, Error, TEXT("Soft pointer Projectile class data not loaded"))
	}
}


void UWeaponFiringComponent::FireSpread( const FVector& SpawnLocation,const FRotator& InitialRotation, int32 ProjectilesPerSide, bool bFireMiddleShot) const
{
	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy ) //is controlled client,
	{
		FireSpreadServer(SpawnLocation, InitialRotation,ProjectilesPerSide,bFireMiddleShot );
	}
	else if(OwnerChar && OwnerChar->HasAuthority()) //called from server
	{
		FireSpreadServer(SpawnLocation, InitialRotation,ProjectilesPerSide,bFireMiddleShot );
	}

	
}

void UWeaponFiringComponent::FireSpreadServer_Implementation(const FVector& SpawnLocation, const FRotator& InitialRotation, int32 ProjectilesPerSide, bool bFireMiddleShot) const
{
	FVector UpVector = GetUpVector();

	for (int32 i = 1; i <= ProjectilesPerSide; i++)
	{
		
		float Degrees = i*FiringData.DegreeSpacingForSpreadPattern;


		FRotator InitialRotationProjectileA = (InitialRotation.Vector()).RotateAngleAxis(Degrees,UpVector).Rotation();
		FRotator InitialRotationProjectileB = (InitialRotation.Vector()).RotateAngleAxis(-Degrees,UpVector).Rotation();
	

		//Z Axis Symmetry here. (Up) //Pitch yaw roll
		//FRotator InitialRotationProjectileA = InitialRotation + FRotator(0.0f,i*DegreeSpacingForSpreadPattern,0.0f);
		//FRotator InitialRotationProjectileB = InitialRotation + FRotator(0.0f,-i*DegreeSpacingForSpreadPattern,0.0f);
	
		//Right axis Symmetry
		/*
		FRotator InitialRotationProjectileA = InitialRotation + FRotator(i*DegreeSpacingForSpreadPattern,0.0f,0.0f);
		FRotator InitialRotationProjectileB = InitialRotation + FRotator(-i*DegreeSpacingForSpreadPattern,0.0f,0.0f);
		*/

		/*
		100% Z axis symmetry when roll = 0
		0% Z axis Symmetry when roll = 90
		*/		

		InitialRotationProjectileA = AddRandomSpreadToRotation(InitialRotationProjectileA);
		InitialRotationProjectileB = AddRandomSpreadToRotation(InitialRotationProjectileB);

		InitialRotationProjectileA = ClampRotation(InitialRotationProjectileA);
		InitialRotationProjectileB = ClampRotation(InitialRotationProjectileB);

		if(bUseLineTracesInsteadOfProjectiles)
		{
			FireLineTraceServer(SpawnLocation,InitialRotationProjectileA);
			FireLineTraceServer(SpawnLocation,InitialRotationProjectileB);
			
		}
		else
		{
			FireSingleProjectileServer(SpawnLocation,InitialRotationProjectileA);
			FireSingleProjectileServer(SpawnLocation,InitialRotationProjectileB);
		}
		

		


	}
	if(bFireMiddleShot)
	{
		FRotator SingleShotRotation = InitialRotation;
		SingleShotRotation = AddRandomSpreadToRotation(SingleShotRotation);
		SingleShotRotation = ClampRotation(SingleShotRotation);
		if(bUseLineTracesInsteadOfProjectiles)
		{
			FireLineTraceServer(SpawnLocation,SingleShotRotation);
		}
		else
		{
			FireSingleProjectileServer(SpawnLocation,SingleShotRotation);
		}
	}
}

void UWeaponFiringComponent::FireStaggered(const FVector& SpawnLocation, const FRotator& InitialRotation, int32 ProjectilesPerSide, bool bFireMiddleShot) const
{
	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy ) //is controlled client,
	{
		FireStaggeredServer(SpawnLocation, InitialRotation,ProjectilesPerSide,bFireMiddleShot );
	}
	else if(OwnerChar && OwnerChar->HasAuthority()) //called from server
	{
		FireStaggeredServer(SpawnLocation, InitialRotation,ProjectilesPerSide,bFireMiddleShot );
	}
	
	

}

void UWeaponFiringComponent::FireStaggeredServer_Implementation(const FVector& SpawnLocation, const FRotator& InitialRotation, int32 ProjectilesPerSide, bool bFireMiddleShot) const
{
	FVector RightVector = GetRightVector();
	FVector ForwardVector = GetForwardVector();

	for (int32 i = 1; i <= ProjectilesPerSide; i++)
	{
		FVector SpawnLocationProjectileA = SpawnLocation + RightVector*FiringData.BulletSpacingForNonSpreadPattern*i + ForwardVector*-i*FiringData.BulletSpacingForNonSpreadPattern;
		FVector SpawnLocationProjectileB = SpawnLocation + RightVector*FiringData.BulletSpacingForNonSpreadPattern*-1*i + ForwardVector*-i*FiringData.BulletSpacingForNonSpreadPattern;

		FRotator InitialRotationProjectileA = AddRandomSpreadToRotation(InitialRotation);
		FRotator InitialRotationProjectileB = AddRandomSpreadToRotation(InitialRotation);

		InitialRotationProjectileA = ClampRotation(InitialRotationProjectileA);
		InitialRotationProjectileB = ClampRotation(InitialRotationProjectileB);

		if(bUseLineTracesInsteadOfProjectiles)
		{
			FireLineTraceServer(SpawnLocationProjectileA,InitialRotationProjectileA);
			FireLineTraceServer(SpawnLocationProjectileB,InitialRotationProjectileB);
		}
		else
		{
			FireSingleProjectileServer(SpawnLocationProjectileA,InitialRotationProjectileA);
			FireSingleProjectileServer(SpawnLocationProjectileB,InitialRotationProjectileB);
		}

	}
	if(bFireMiddleShot)
	{
		FRotator SingleShotRotation = InitialRotation;
		SingleShotRotation = AddRandomSpreadToRotation(SingleShotRotation);
		SingleShotRotation = ClampRotation(SingleShotRotation);
		if(bUseLineTracesInsteadOfProjectiles)
		{
			FireLineTraceServer(SpawnLocation,SingleShotRotation);
		}
		else
		{
			FireSingleProjectileServer(SpawnLocation,SingleShotRotation);
		}
	}
}


void UWeaponFiringComponent::FireWeaponComponent()
{
	
	
	
	if(StyleType == EFiringStyle::BurstFire && WorldTimerManager)
	{
		if(!BurstFireDelegate.IsBound())
		{
			BurstFireDelegate.BindUFunction(this,FName("FirePattern"));
		}
	
		if(!WorldTimerManager->TimerExists(BurstFireHandle))
		{
			WorldTimerManager->SetTimer(BurstFireHandle,BurstFireDelegate,TimeBetweenEachBurst,true);
		}
		else if(WorldTimerManager->IsTimerPaused(BurstFireHandle))
		{
			bool bPassTimingCheck = false;
			bool bTemp = false;
			CanFire(0,bPassTimingCheck,bTemp);
			if(bPassTimingCheck) //if the weapon isn't on cooldown
			{
				WorldTimerManager->UnPauseTimer(BurstFireHandle);
			}
		}
	}
	else
	{
		FirePattern();
	}
	
	
}

FRotator UWeaponFiringComponent::GetFiringInitialRotation(const ERotationType InputRotationType) const
{
	
	FRotator InitialRotation = FRotator();

	switch(InputRotationType)
	{

		case ERotationType::WorldSpaceOfComponent:
		{
			InitialRotation = GetComponentRotation(); //rotation in world space
			break;
		}
		case ERotationType::ControllerRotation:
		{
			if(OwnerController)
			{
				InitialRotation = OwnerController->GetControlRotation();
			}
			
			break;
		}

	}

	return InitialRotation;
}

bool UWeaponFiringComponent::IsFiring() const
{
	bool bIsFiringMontagePlaying = false;
	UAnimMontage* Montage = FiringSoftAnimMontage.Get();
	if(OwnerChar && Montage)
	{
		if(USkeletalMeshComponent* MeshComp = OwnerChar->GetMesh())
		{
			if(UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
			{
				bIsFiringMontagePlaying = AnimInst->Montage_IsPlaying(Montage);
			}
		}
	}

	bool bAutoFiring = false;
	bool bBurstingFiring = false;

	if(WorldTimerManager)
	{
		bAutoFiring = WorldTimerManager->IsTimerActive(AutoFireHandle);
		bBurstingFiring = WorldTimerManager->IsTimerActive(BurstFireHandle);
	}

	

	return (bIsFiringMontagePlaying || bBurstingFiring || bAutoFiring);
}

bool UWeaponFiringComponent::IsReloading() const
{
	bool bIsReloadMontagePlaying = false;
	UAnimMontage* ReloadMontage = ReloadSoftAnimMontage.Get();
	if(OwnerChar && ReloadMontage)
	{
		if(USkeletalMeshComponent* MeshComp = OwnerChar->GetMesh())
		{
			if(UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
			{
				bIsReloadMontagePlaying = AnimInst->Montage_IsPlaying(ReloadMontage);
			}
		}
	}
	return bIsReloadMontagePlaying;
}



void UWeaponFiringComponent::PlayFiringMontage()
{
	
	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy)
	{
		PlayFiringMontageServer();
	}
	else if(OwnerWeapon && OwnerWeapon->HasAuthority())
	{
		PlayFiringMontageServer();
	}

}

void UWeaponFiringComponent::PlayFiringMontageMulticast_Implementation()
{
	UAnimMontage* Montage = FiringSoftAnimMontage.Get();
	UWeaponManager::PlayMontage(OwnerChar,Montage,FiringMontagePlayrate,FName(""),true);

	
}

void UWeaponFiringComponent::PlayFiringMontageServer_Implementation()
{

	
	bool bIsMontagePlaying = false;
	UAnimMontage* Montage = FiringSoftAnimMontage.Get();
	if(OwnerChar && Montage)
	{
		if(USkeletalMeshComponent* MeshComp = OwnerChar->GetMesh())
		{
			if(UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
			{
				bIsMontagePlaying = AnimInst->Montage_IsPlaying(Montage);
			}
			else if(bShowDebugWarnings)
			{
				UE_LOG(LogTemp,Warning,TEXT("AnimInst is nullptr, "))
			}
		}
		else if(bShowDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("MeshComp is nullptr, "))
		}
		
	}
	else
	{
		if(bShowDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("OwnerChar or Montage is nullptr, Did you load the soft pointers? "))
		}
		
		return;
	}
	

	if(!bIsMontagePlaying)
	{
		
		PlayFiringMontageMulticast();
	}
	
}

void UWeaponFiringComponent::ReloadWeapon(bool bChangeAmmoCount, bool bPlayMontage)
{
	
	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy ) 
	{
		ReloadWeaponServer( bChangeAmmoCount,  bPlayMontage);
		
	}
	else if(OwnerWeapon && OwnerWeapon->HasAuthority())
	{
		ReloadWeaponServer( bChangeAmmoCount,  bPlayMontage);
	}
	
	
	
	

}


void UWeaponFiringComponent::ReloadWeaponMulticast_Implementation()
{

	UAnimMontage* ReloadMontage = ReloadSoftAnimMontage.Get();
	if(!ReloadMontage)
	{
		UE_LOG(LogTemp,Warning,TEXT("ReloadSoftAnimMontage returns a nullptr, remember to load the soft pointers into memory before use! "))
		return;
	}
	UWeaponManager::PlayMontage(OwnerChar,ReloadMontage,ReloadMontagePlayrate,FName(""),true);
	OnWeaponReload.Broadcast();
}

void UWeaponFiringComponent::ReloadWeaponServer_Implementation(bool bChangeAmmoCount, bool bPlayMontage)
{
	
	
	if(bChangeAmmoCount)
	{
		bool bEnoughAmmoReserve = false;
		if(ReserveAmmunition >= ClipSize && !bUnlimitedClip)
		{
			ClipAmmunition = ClipSize;
			ReserveAmmunition = ReserveAmmunition-ClipSize;
			bEnoughAmmoReserve = true;
		}
		else if(bUnlimitedClip)
		{
			ClipAmmunition = ClipSize;
			bEnoughAmmoReserve = true;
		}


		if(bPlayMontage)
		{
			ReloadWeaponMulticast();
		}

	}
	else
	{
		if(bPlayMontage)
		{
			ReloadWeaponMulticast();
		}
	}
}


void UWeaponFiringComponent::RestoreAmmunition(bool bRestoreClip, bool bRestoreReserve)
{
	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy ) 
	{
		RestoreAmmunitionServer( bRestoreClip,  bRestoreReserve);
		
	}
	else if(OwnerWeapon && OwnerWeapon->HasAuthority())
	{
		RestoreAmmunitionServer( bRestoreClip,  bRestoreReserve);
	}
}


void UWeaponFiringComponent::RestoreAmmunitionMulticast_Implementation(bool bRestoreClip, bool bRestoreReserve)
{
	OnAmmunitionRestore.Broadcast();
}


void UWeaponFiringComponent::RestoreAmmunitionServer_Implementation(bool bRestoreClip, bool bRestoreReserve)
{
	if(bRestoreClip)
	{
		ClipAmmunition = ClipSize;
	}

	if(bRestoreReserve)
	{
		ReserveAmmunition = ReserveSize;
	}

	if(bRestoreClip || bRestoreReserve )
	{
		RestoreAmmunitionMulticast(bRestoreClip,bRestoreReserve);
	}
}

void UWeaponFiringComponent::StartFiringWeaponComponent()
{
	if(!WorldTimerManager)
	{
		return;
	}

	
	if(!bAutomaticFire)
	{
		if(bShowDebugWarnings)
		{
			UE_LOG(LogTemp,Warning,TEXT("Weapon is not automatic, just call FireWeaponComponent() instead"))
		}
		
		FireWeaponComponent();
		return;
	}
	
	if(!AutoFireDelegate.IsBound())
	{
		AutoFireDelegate.BindUFunction(this,FName("FireWeaponComponent"));
		
	}
	
	if(!WorldTimerManager->IsTimerActive(AutoFireHandle))
	{
		FireWeaponComponent();//Start firing.
		WorldTimerManager->SetTimer(AutoFireHandle,AutoFireDelegate,Cooldown,true);
	}

	
	
	
}

void UWeaponFiringComponent::StopFiringWeaponComponent()
{
	if(!WorldTimerManager)
	{
		return;
	}

	WorldTimerManager->ClearTimer(AutoFireHandle);
	UAnimMontage* Montage = FiringSoftAnimMontage.Get();
	float BlendOut = 1.0f;

	if(!Montage)
	{
		return;
	}


	StopMontage(BlendOut,Montage);



}

void UWeaponFiringComponent::StopMontage(float BlendOutTime, UAnimMontage* MontageToStop)
{
	if(OwnerChar && !OwnerChar->HasAuthority() && OwnerChar->GetLocalRole() == ROLE_AutonomousProxy)
	{
		StopMontageServer(BlendOutTime,MontageToStop);
	}
	else if(OwnerWeapon && OwnerWeapon->HasAuthority())
	{
		StopMontageServer(BlendOutTime,MontageToStop);
	}

}

void UWeaponFiringComponent::StopMontageMulticast_Implementation(float BlendOutTime,UAnimMontage* MontageToStop)
{
	if(OwnerChar && MontageToStop)
	{
		if(USkeletalMeshComponent* MeshComp = OwnerChar->GetMesh())
		{
			if(UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
			{
				AnimInst->Montage_Stop(BlendOutTime,MontageToStop);
			}
		}
	}
}


void UWeaponFiringComponent::StopMontageServer_Implementation(float BlendOutTime,UAnimMontage* MontageToStop)
{
	StopMontageMulticast(BlendOutTime,MontageToStop);
}