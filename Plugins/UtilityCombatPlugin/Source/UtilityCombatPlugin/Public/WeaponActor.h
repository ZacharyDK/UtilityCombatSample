// Copyright Zachary Kolansky, 2020

#pragma once

#include "CoreMinimal.h"
#include "Animation/SkeletalMeshActor.h"
#include "WeaponDataStructures.h"
#include "WeaponActor.generated.h"

class UStatManager;
class UWeaponFiringComponent;
class UNetConnection;
class UMeleeMontageComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWeaponStateChange);

/**
 * 
 * Base class for a replicated weapon, meant to be attached to character skeletal mesh.
 * Notably overrides GetNetConnection() to return the net connection of GetAttachParentActor() , so firing components attached to this actor can
 * call RPCs.
 */
UCLASS()
class UTILITYCOMBATPLUGIN_API AWeaponActor : public ASkeletalMeshActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const override;

	virtual UNetConnection * GetNetConnection() const override;

public:

	UPROPERTY(BlueprintAssignable)
	FWeaponStateChange OnWeaponEquipped;

	UPROPERTY(BlueprintAssignable)
	FWeaponStateChange OnWeaponHolstered;


	AWeaponActor(const FObjectInitializer& ObjectInitializer);



	UPROPERTY(VisibleAnywhere,BlueprintReadWrite, Category = Components)
	UStatManager* WeaponStatManager = nullptr;


	UPROPERTY(VisibleAnywhere,BlueprintReadWrite, Category = Components)
	UWeaponFiringComponent* PrimaryWeaponFiringComponent = nullptr;

	UPROPERTY(VisibleAnywhere,BlueprintReadWrite, Category = Components)
	UMeleeMontageComponent* MeleeMontageComponent = nullptr;

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = Equip)
	FWeaponData WeaponData = FWeaponData();

	UFUNCTION(BlueprintPure, Category = Query)
	bool HasFiringComponent();

	UFUNCTION(BlueprintPure, Category = Query)
	bool HasMeleeComponent();

	/*
	Is this Weapon associated with a WeaponManagerComponent?
	*/
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = Equip)
	bool IsInWeaponInventory = false;

	/*
	Determines whether this stat component can modify the values in the Master stat component
	Returns whether we were successful
	*/
	bool SetStatusOfStatDictionary(bool bNewStatus);


	void Equip(UStatManager* WielderWeaponStatManager);

	UFUNCTION(NetMulticast, Reliable)
	void EquipMulticast(UStatManager* WielderWeaponStatManager);

	UFUNCTION(Server, Reliable)
	void EquipServer(UStatManager* WielderWeaponStatManager);

	/*
	Override this BP
	*/
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable,Category = Basic)
	void FireWeapon();

	/*
	Override this BP
	Useful if you want to stop firing behavior.
	*/
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable, Category = Basic)
	void FireWeaponEnd();



	void Holster(UStatManager* WielderWeaponStatManager);

	UFUNCTION(NetMulticast, Reliable)
	void HolsterMulticast(UStatManager* WielderWeaponStatManager);

	UFUNCTION(Server, Reliable)
	void HolsterServer(UStatManager* WielderWeaponStatManager);




	/*
	Override this BP
	*/
	UFUNCTION(BlueprintNativeEvent,BlueprintCallable, Category = Basic)
	void ReloadWeapon();

	/*
	Weapon no longer inventory. This is what you call when a weapon is dropped from ACharacter to clean up
	*/
	void Remove(UStatManager* WielderWeaponStatManager);

	UFUNCTION(NetMulticast, Reliable)
	void RemoveMulticast(UStatManager* WielderWeaponStatManager);

	UFUNCTION(Server, Reliable)
	void RemoveServer(UStatManager* WielderWeaponStatManager);

	
};

