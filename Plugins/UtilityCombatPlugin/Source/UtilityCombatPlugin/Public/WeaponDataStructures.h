// Copyright Zachary Kolansky, 2020

#pragma once
#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Engine/DataTable.h"
#include "WeaponDataStructures.generated.h"

class UAnimMontage;


/*
Weapon Management
*/
UENUM(BlueprintType)
enum class EWeaponAttachType : uint8 {NOTATTACHED, ArmedSocket, HolsterSocket};

/*
Class and Socket Data
*/
USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	/*
	GUN,SWORD,MAGIC,etc

	"" means no weapon class
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FName WeaponType = FName("");

	/*
	Where on the weapon we want the sphere trace to start
	during  MeleeDamage notify state
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FName WeaponDamageStartSocket = FName("");

	
	/*
	Where on the weapon we want the sphere trace to end
	during  MeleeDamage notify state
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FName WeaponDamageEndSocket = FName("");


	

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float MeleeCapsuleTraceRadius = 30.0f;

    /*
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float ThreatCapsuleTraceRadius = 50.0f;

    UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float WeaponThreatTraceDistance = 2000.0f;
    */
	
	FWeaponData()
	{

	}

};

/*
Given a weapon class name, determine what montages
*/
USTRUCT(BlueprintType)
struct FEquipData :  public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TSoftObjectPtr<UAnimMontage> SoftEquipMontage = nullptr;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TSoftObjectPtr<UAnimMontage> SoftHolsterMontage = nullptr;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float EquipPlayrate = 1.0f;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float HolsterPlayrate = 1.0f;

	/*
	Where on the parent mesh do we attach the weapon when armed?
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FName ArmedSocket = FName();

	/*
	Where on the parent mesh do we attach the weapon when Holstered?
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FName HolsterSocket = FName();


	FEquipData()
	{

	}
};



UENUM(BlueprintType)
enum class ERotationType : uint8 {WorldSpaceOfComponent,ControllerRotation};


UENUM(BlueprintType)
enum class EPatternType : uint8 {Staggered,Spread,Line,SingleProjectile};

UENUM(BlueprintType)
enum class ERandomSpreadType : uint8 {NoSpread,RandomSpread};

UENUM(BlueprintType)
enum class ERotationClampType : uint8 {NoClamp,Pitch,Roll,Yaw,PitchRoll,PitchYaw,RollYaw,PitchRollYaw};


UENUM(BlueprintType)
enum class EFiringStyle : uint8 {SingleFire,BurstFire};




USTRUCT(BlueprintType)
struct FProjectileFiringData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pattern)
	TSoftClassPtr<class AActor> ProjectileSoftClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pattern)
	EPatternType PatternType = EPatternType::Spread;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pattern)
	int32 ProjectilesPerShot = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pattern)
	float DegreeSpacingForSpreadPattern = 10.0f;

	//How far apart additional bullets will be in unreal units
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pattern)
	float BulletSpacingForNonSpreadPattern = 100.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RandomSpread)
	ERandomSpreadType SpreadType = ERandomSpreadType::NoSpread;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RandomSpread)
	float MinRandomDegreeSpread = -10.0f; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RandomSpread)
	float MaxRandomDegreeSpread = 10.0f;


	FProjectileFiringData()
	{

	}
};
