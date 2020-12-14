// Copyright Zachary Kolansky, 2020

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "WeaponDataStructures.h"
#include "TimerManager.h"
#include "WeaponFiringComponent.generated.h"


class ACharacter;
class AController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFiringEvent);

/*
A replicated component that can fire line traces or any actor as projectiles. Can be attached to a ACharacter or AWeapon,
with no change in functionality or behavior. It handles all of that behind the scenes. 
Also handles:
Ammunition
Single Fire vs Burst Fire. 
Firing Cooldown, and Automatic Firing
Manage Timers, and cleaning up timers.
Reloading, playing reload montages, 
CanFire() method
Firing line traces and projectiles in patterns.
*/
UCLASS( Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTILITYCOMBATPLUGIN_API UWeaponFiringComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponFiringComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(BlueprintAssignable)
	FFiringEvent OnWeaponFire;

	UPROPERTY(BlueprintAssignable)
	FFiringEvent OnWeaponReload;

	UPROPERTY(BlueprintAssignable)
	FFiringEvent OnAmmunitionRestore;

	/*
	If firing in a pattern, only one ammo will be consumed if
	this is true. 
	If False, I'll manage the number projectiles that would be spawned
	from a specific pattern. 
	(Ex. ClipAmmunition will be reduced by the ammount of bullets that would be fired)

	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Ammunition)
	bool bOneAmmoPerShot = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Ammunition)
	bool bUnlimitedClip = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Ammunition)
	bool bUnlimitedReserve = false;

	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category=Ammunition)
	int32 ClipAmmunition = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Ammunition)
	int32 ClipSize = 6;

	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category=Ammunition)
	int32 ReserveAmmunition = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Ammunition)
	int32 ReserveSize = 100;

	/*
	If true, we can fire as long as the button is held down. Fired every X seconds, where X =  Cooldown
	If false, need to click for each shot.

	If true, don't use a notify in a montage to fire the weapon!
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Automatic)
	bool bAutomaticFire = true;

	/*
	If can't fire, try to reload
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Automatic)
	bool bAutomaticReload = true;

	FTimerDelegate AutoFireDelegate; //Degates are bound to UFUNCTIONS

	UPROPERTY(VisibleAnywhere,Category=Automatic)
    FTimerHandle AutoFireHandle; //Set to the world's timer manager

	
	FTimerDelegate BurstFireDelegate; //Degates are bound to UFUNCTIONS

	UPROPERTY(VisibleAnywhere,Category=Burst)
    FTimerHandle BurstFireHandle; //Set to the world's timer manager


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Burst)
	EFiringStyle StyleType = EFiringStyle::SingleFire;

	UPROPERTY(VisibleAnywhere,Category=Burst)
	int32 Bursts = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Burst)
	int32 TimesToFirePerBurst = 3;

	

	/*
	In Seconds
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Burst)
	float TimeBetweenEachBurst = 0.5f;



	/*
	Time in seconds that must pass before this weapon can fire again.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Cooldown)
	float Cooldown = 1.0f;

	/*
	Time we last fired.
	*/
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Transient,Category = Cooldown)
	float WorldTimeLastFired = -1.0f;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debug)
	bool bDebugLineTracePersistent = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debug)
	bool bShowDebugWarnings = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debug)
	bool bShowDebugLineTrace = false;

	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debug)
	FColor DebugLineTraceColor =  FColor(255,0,255, 1);



	

	/*
	Firing Clamp Paramaters
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Firing)
	ERotationClampType ClampType = ERotationClampType::NoClamp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Firing)
	float MinPitch = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Firing)
	float MaxPitch = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Firing)
	float MinYaw = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Firing)
	float MaxYaw = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Firing)
	float MinRoll = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Firing)
	float MaxRoll = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=FiringData)
	FProjectileFiringData FiringData = FProjectileFiringData();


	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = FiringMontage)	
	TSoftObjectPtr<class UAnimMontage> FiringSoftAnimMontage;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = FiringMontage)	
	float FiringMontagePlayrate = 1.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LineTrace)
	bool bUseLineTracesInsteadOfProjectiles = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LineTrace)
	TSubclassOf < class UDamageType > LineTraceDamageTypeClass = NULL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LineTrace)
	float LineTraceDamage = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=LineTrace)
	float LineTraceDistance = 3000.0f;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Options)
	ERotationType RotationType = ERotationType::WorldSpaceOfComponent;

	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Owner)
	ACharacter* OwnerChar = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Owner)
	AController* OwnerController = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Owner)
	AActor* OwnerWeapon = nullptr;



	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = Reload)	
	TSoftObjectPtr<class UAnimMontage> ReloadSoftAnimMontage;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = Reload)	
	float ReloadMontagePlayrate = 1.0f;


	//tired of writing GetWorld()->GetTimerManager()
	FTimerManager* WorldTimerManager = nullptr;


	UFUNCTION(BlueprintCallable)
	void Initialize();

	FRotator AddRandomSpreadToRotation(const FRotator InputRotation) const;


	/*
	If BulletsToFire = 0, then we just check to see if we pass the bPassTimingCheck (i.e cooldown) check.
	*/
	UFUNCTION(BlueprintPure, Category = Firing)
	bool CanFire(int32 BulletsToFire, bool& bPassTimingCheck, bool& bPassAmmunitionConsumptionCheck);

	/*
	return (!IsReloading() && ReserveAmmunition >= ClipSize);
	*/
	UFUNCTION(BlueprintPure, Category = Firing)
	bool CanReload() const;

	/*
	Rotation modifiers
	*/
	FRotator ClampRotation(const FRotator InputRotator) const;


	void FireLine(const FVector& SpawnLocation, const FRotator& InitialRotation, int32 ProjectilesPerSide, bool bFireMiddleShot ) const;

	UFUNCTION(Server,Reliable)
	void FireLineServer(const FVector& SpawnLocation, const FRotator& InitialRotation, int32 ProjectilesPerSide, bool bFireMiddleShot ) const;


	void FireLineTrace(const FVector StartLocation, const FRotator InitialRotation) const;

	UFUNCTION(Server,Reliable)
	void FireLineTraceServer(const FVector StartLocation, const FRotator InitialRotation) const;

	UFUNCTION(BlueprintCallable)
	void FirePattern();



	void FireSingleProjectile(const FVector SpawnLocation, const FRotator SpawnRotation) const;

	UFUNCTION(Server,Reliable)
	void FireSingleProjectileServer(const FVector SpawnLocation, const FRotator SpawnRotation) const;

	void FireSpread(const FVector& SpawnLocation, const FRotator& InitialRotation, int32 ProjectilesPerSide, bool bFireMiddleShot ) const;

	UFUNCTION(Server,Reliable)
	void FireSpreadServer(const FVector& SpawnLocation, const FRotator& InitialRotation, int32 ProjectilesPerSide, bool bFireMiddleShot ) const;

	


	void FireStaggered(const FVector& SpawnLocation, const FRotator& InitialRotation, int32 ProjectilesPerSide, bool bFireMiddleShot) const;

	UFUNCTION(Server,Reliable)
	void FireStaggeredServer(const FVector& SpawnLocation, const FRotator& InitialRotation, int32 ProjectilesPerSide, bool bFireMiddleShot ) const;




	UFUNCTION(BlueprintCallable,Category = Firing)
	void FireWeaponComponent();

	FRotator GetFiringInitialRotation(const ERotationType InputRotationType) const;

	/*
	Returns true if the firing Montage is playing, or if burst firing or auto firing
	*/
	UFUNCTION(BlueprintPure,Category = Firing)
	bool IsFiring() const;

	/*
	returns true if the reload montage is playing. False otherwise.
	bIsReloadMontagePlaying = GetMesh()->GetAnimInstance()->Montage_IsPlaying(ReloadSoftAnimMontage.Get());
	*/
	UFUNCTION(BlueprintPure,Category = Firing)
	bool IsReloading() const;



	UFUNCTION(BlueprintCallable,Category = Firing)
	void PlayFiringMontage();

	UFUNCTION(NetMulticast,Reliable)
	void PlayFiringMontageMulticast();

	UFUNCTION(Server,Reliable)
	void PlayFiringMontageServer();

	UFUNCTION(BlueprintCallable,Category = Firing)
	void ReloadWeapon(bool bChangeAmmoCount = true, bool bPlayMontage = false);

	UFUNCTION(NetMulticast,Reliable)
	void ReloadWeaponMulticast();

	UFUNCTION(Server,Reliable)
	void ReloadWeaponServer(bool bChangeAmmoCount = true, bool bPlayMontage = false);

	UFUNCTION(BlueprintCallable, Category = Firing)
	void RestoreAmmunition(bool bRestoreClip = true, bool bRestoreReserve = true);

	UFUNCTION(NetMulticast,Reliable)
	void RestoreAmmunitionMulticast(bool bRestoreClip = true, bool bRestoreReserve = true);

	UFUNCTION(Server,Reliable)
	void RestoreAmmunitionServer(bool bRestoreClip = true, bool bRestoreReserve = true);
	



	UFUNCTION(BlueprintCallable, Category = Automatic)
	void StartFiringWeaponComponent();


	

	UFUNCTION(BlueprintCallable,Category = Automatic)
	void StopFiringWeaponComponent();


	void StopMontage(float BlendOutTime, UAnimMontage* MontageToStop);

	UFUNCTION(NetMulticast,Reliable)
	void StopMontageMulticast(float BlendOutTime, UAnimMontage* MontageToStop);

	UFUNCTION(Server,Reliable)
	void StopMontageServer(float BlendOutTime,UAnimMontage* MontageToStop);



};
