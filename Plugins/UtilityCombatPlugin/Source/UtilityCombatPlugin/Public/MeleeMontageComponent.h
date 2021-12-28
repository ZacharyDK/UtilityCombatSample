// Copyright Zachary Kolansky, 2020

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "MeleeMontageComponent.generated.h"




class USkeletalMeshComponent;
class UAnimInstance;
class UAnimMontage;
class AController;

UENUM(BlueprintType)
enum class EMontagePlayType : uint8 {ALWAYS,IF_NO_BLACKLIST_MONTAGES,IF_NO_BLACKLIST_MONTAGES_IGNORE_GLOBAL_BLACKLIST};


/*
Collection of Data that defines how and when a Montage can play
*/
USTRUCT(BlueprintType)
struct FMeleeMontageCollectionData
{
    GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Montage)
	TSoftObjectPtr<UAnimMontage> SoftMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Montage)
	float MontagePlayrate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Montage)
	FName MontageStartSectionName = FName("");


	/*
	If true, we can only play this montage if another
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PlayType)
	EMontagePlayType MontagePlayType = EMontagePlayType::ALWAYS;

	
	/*
	If any of these montages are playing, then this montage can't play
	No effect MontagePlayType = EMontagePlayType::ALWAYS;
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PlayType)
	TArray<TSoftObjectPtr<UAnimMontage>> SoftBlackListMontages = {};

	/*
	If this montage should stop other montages
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Interrupt)
	bool InterruptOtherMontages = false;

	/*
	Montages that will be stopped when this Montage is played
	If this list is empty, then all montages will be stopped instead
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Interrupt)
	TArray<TSoftObjectPtr<UAnimMontage>> OtherMontagesToStop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Interrupt)
	float MontageInterruptBlendOutTime = 1.0f;


	FMeleeMontageCollectionData()
	{
		
	}
};

/*
The purpose of this component is to manage the montages a character should play when wielding a weapon
*/
UCLASS(  Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTILITYCOMBATPLUGIN_API UMeleeMontageComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMeleeMontageComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	
	FTimerDelegate AutoFireDelegate; //Degates are bound to UFUNCTIONS

	//PROPERTY(VisibleAnywhere,Category=Automatic)
    FTimerHandle AutoFireHandle; //Set to the world's timer manager

		
	FTimerDelegate CleanUpDelegate; //Degates are bound to UFUNCTIONS

	//PROPERTY(VisibleAnywhere,Category=Automatic)
    FTimerHandle CleanUpHandle; //Set to the world's timer manager



	/*
	How frequently do we want PlayNextMontage() to fire when we are are auto swinging. (i.e mouse button down)
	Note that I still check to see if cooldown is appropiate
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Automatic)
	float AutoSwingFireFrequency = 1.0f;
	

	/*
	Time in seconds that must pass before this component can fire again.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Cooldown)
	float Cooldown = 1.0f;

	/*
	Time we last fired.
	*/
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Transient,Category = Cooldown)
	float WorldTimeLastMontage = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debug)
	bool bShowDebugWarnings = false;



	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Owner)
	ACharacter* OwnerChar = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Owner)
	AController* OwnerController = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Owner)
	AActor* OwnerWeapon = nullptr;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Owner)
	USkeletalMeshComponent* OwnerMeshComp = nullptr;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category=Owner)
	UAnimInstance* OwnerCharacterAnimInst = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Montages)
	int32 CurrentMontageIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Montages)
	FMeleeMontageCollectionData BlockMontageData = FMeleeMontageCollectionData();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Montages)
	TArray<FMeleeMontageCollectionData> MontagesToPlay = {};

	/*
	Useful for keeping track of a bunch of montages
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Montages)
	TMap<FName,FMeleeMontageCollectionData> MontageUtilityMap;

	/*
	Can't play if any of these montages are playing.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Montages)
	TArray<TSoftObjectPtr<UAnimMontage>> SoftGlobalBlackListMontages;



	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Montages)
	UAnimMontage* LastMontagePlayed = nullptr;

	FTimerManager* WorldTimerManager = nullptr;


	/*
	Automatically called during Begin play
	Cache's the OwnerActor, MeshComponent, Anim Instance
	*/
	UFUNCTION(BlueprintCallable)
	void Initialize();

	/*
	Returns true if we can play a Montage, false if that is not possible
	Also returns false if the given montage is already playing
	*/
	bool CanPlayMontageData(const FMeleeMontageCollectionData& MontageCollectionData, bool& bPassCooldownCheck, bool& bPassGlobalBlackList, bool& bPassInternalBlackList) const;



	UFUNCTION(BlueprintPure)
	TArray<TSoftObjectPtr<UAnimMontage>> GetMontageListFromCollection(UPARAM(ref)const TArray<FMeleeMontageCollectionData>& CollectionArray) const;

	/*
	returns true if any montage is the list is playing,, false otherwise.
	 Note that soft pointers must have already been loaded.
	*/
	UFUNCTION(BlueprintPure)
	bool IsMontageInListPlaying(UPARAM(ref) const  TArray<TSoftObjectPtr<UAnimMontage>>& MontageList) const;

	

	/*
	Play the next Montage in MontagesToPlay. Index = CurrentMontageIndex
	*/
	UFUNCTION(BlueprintCallable, Category = Montage)
	void PlayNextMontage();

	/*
	Helper Method for determining if we can play a montage, stopping necessary montages, and resolving soft references
	*/
	UFUNCTION(BlueprintCallable, Category = Montage)
	void PlayCollectionMontage(UPARAM(ref) const FMeleeMontageCollectionData& MontageCollectionData);

	/*
	Plays a list of Montages one after another, in order
	returns the total time
	*/
	UFUNCTION(BlueprintCallable, Category = Montage)
	float PlayCollectionMontageSequence(UPARAM(ref) TArray<FMeleeMontageCollectionData>& Montages);

	/*
	Calls server RPC if the OwnerChar is a ROLE_AutonomousProxy
	If the OwnerWeapon has authority
	OR if OwnerCharacter has authority. 

	Server RPC calls multicast RPC, ensures montage is networked appropiately.  
	*/
	UFUNCTION(BlueprintCallable, Category = Montage)
	void PlayMontage(UAnimMontage* PlayMontage, float Playrate = 1.0f, FName StartSection = FName(""));

	UFUNCTION(NetMulticast,Reliable)
	void PlayMontageMulticast(UAnimMontage* PlayMontage, float Playrate = 1.0f, FName StartSection = FName(""));

	UFUNCTION(Server,Reliable)
	void PlayMontageServer(UAnimMontage* PlayMontage, float Playrate = 1.0f, FName StartSection = FName(""));

	/*
	Calls server RPC if the OwnerChar is a ROLE_AutonomousProxy
	If the OwnerWeapon has authority
	OR if OwnerCharacter has authority. 

	Server RPC calls multicast RPC, ensures montage is networked appropiately.  
	*/
	UFUNCTION(BlueprintCallable,Category = Montage)
	void StopMontage(float BlendOutTime, UAnimMontage* MontageToStop);

	UFUNCTION(NetMulticast,Reliable)
	void StopMontageMulticast(float BlendOutTime, UAnimMontage* MontageToStop);

	UFUNCTION(Server,Reliable)
	void StopMontageServer(float BlendOutTime,UAnimMontage* MontageToStop);

	/*
	Calls server RPC if the OwnerChar is a ROLE_AutonomousProxy
	If the OwnerWeapon has authority
	OR if OwnerCharacter has authority. 

	Server RPC calls multicast RPC, ensures montage is networked appropiately.  

	Note the variable CurrentMontageIndex is not replicated, but managed concurrently between the clients and servers. 
	Use this RPC if you want to force CurrentMontageIndex to be a specific value
	*/
	UFUNCTION(BlueprintCallable,Category = Montage)
	void ResetMontageIndex(int32 NewIndex = 0);

	UFUNCTION(NetMulticast,Reliable)
	void ResetMontageIndexMulticast(int32 NewIndex = 0);

	UFUNCTION(Server,Reliable)
	void ResetMontageIndexServer(int32 NewIndex = 0);

	/*
	AUTOMATIC
	*/

	/*
	Will call PlayNextMontage() on a timer, every AutoSwingFireFrequency seconds.
	This is done on the client, and you'll be sending RPCs at the given frequency.
	*/
	UFUNCTION(BlueprintCallable, Category = Automatic)
	void StartAutoMelee();

	UFUNCTION(BlueprintCallable, Category = Automatic)
	void StopAutoMelee(bool bStopLastPlayedMontage = true, float BlendOutTime = 1.0f);

};
