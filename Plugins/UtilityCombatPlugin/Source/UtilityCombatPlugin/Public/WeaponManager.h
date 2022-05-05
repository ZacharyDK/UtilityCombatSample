// Copyright Zachary Kolansky, 2020

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponDataStructures.h"
#include "StatDataStructures.h"
#include "WeaponManager.generated.h"

class AWeaponActor;
class ACharacter;
class UStatManager;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWeaponSpawnFinish);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWeaponState, AWeaponActor*, WeaponActor);


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWeaponEquipState, FName, WeaponType, bool , bEquipState);

/*
A replicated component that manages the inventory of AWeaponActor's. 
Meant to attached to ACharacter with a skeletal mesh.

Responsible for: 
1. Attaching and detaching weapon to the owner's skeletal mesh at specific sockets
2. Playing equip and holster montages, (optional to change the equipped weapon)
3. Having an active weapon, but have said equip weapon be either equipped or holstered
4.Manage changes in the Owner's UStatManager and the Weapon's UStatManager to respond to equipment changes. Replicated.
When EquipWeapon() and HolsterWeapon() are called with the parameter bImmediatelyChangeActorSocket = true, then the AWeaponActor's 
Equip() and Holster() will be called respectively. This passes down Owner's UStatManager to the weapon, such that the weapon can 
add it's stat component to the owner's OtherStatManagers Array. 
The AWeaponActor also changes bStatDictionaryCanModifyOtherStatDictionary in its UStatManager to respond to whether it is equipped or holstered. 

*/
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UTILITYCOMBATPLUGIN_API UWeaponManager : public UActorComponent //, public IDungeonSaveDataInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponManager();


	UPROPERTY(BlueprintAssignable)
	FWeaponState OnActiveWeaponChange;

	/*

	*/
	UPROPERTY(BlueprintAssignable)
	FWeaponState OnWeaponEquipped;

	/*

	*/
	UPROPERTY(BlueprintAssignable)
	FWeaponState OnWeaponHolstered;

	/*
	
	*/
	UPROPERTY(BlueprintAssignable)
	FWeaponEquipState OnNewWeaponEquip;

	/*
	Called after SpawnWeaponsFromSavedData() is done
	*/
	UPROPERTY(BlueprintAssignable)
	FWeaponSpawnFinish OnFinishSpawningWeapons;


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray < class FLifetimeProperty > & OutLifetimeProps) const override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	/*
	Is the weapon in the active slot equipped
	*/
	UPROPERTY(ReplicatedUsing = OnRep_EquipState,VisibleAnywhere,BlueprintReadOnly,Category = CurrentEquip)
	bool bEquipState = false;

	/*
	What weapon will be equipped if can equip multiple weapons
	*/
	UPROPERTY(ReplicatedUsing = OnRep_ActiveWeaponSlot,VisibleAnywhere,BlueprintReadOnly, Category = CurrentEquip)
	int32 ActiveWeaponSlot = 0;


	UPROPERTY(ReplicatedUsing = OnRep_ActiveWeapon, VisibleAnywhere,BlueprintReadOnly, Category = CurrentEquip)
	AWeaponActor* ActiveWeapon = nullptr; 

	UPROPERTY(ReplicatedUsing = OnRep_CurrentEquipWeaponType, VisibleAnywhere,BlueprintReadOnly, Category = CurrentEquip)
	FName CurrentEquippedWeaponType = FName("");



	/*
	Data table is read into this.
	FName is the Key. Describes the weapon type
	*/
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite, Category = Data)
	TMap<FName,FEquipData> EquipDataDictionary; 

	/*
	Data table we read into EquipDataDictionary in Intialize()
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = Data)
	UDataTable* WeaponEquipData = nullptr;


	/*
	Print warnings?
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = Debug)
	bool bDebugLog = false;


	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category = Options)
	bool bMakeHolsteredWeaponInvisible = true;
	
	/*
	If true, we will resize the weapon to it's original world scale after attaching
	*/	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = Options)
	bool bUseWeaponScaleInsteadOfSocketScale = false;
	
	/*
	How many weapons can we carry?
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = Options)
	int32 WeaponSlots = 1;

	/*
	What kind of weapons can we equip
	*/
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = Options)
	TArray<FName> EquippableWeaponTypes = {FName("Sword"),FName("Gun"),FName("Magic")};


		/*
	Slot whose socket was not updated immediately when it was holstered.
	We want to wait for the animation to finish.
	*/
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly, Category = Utility)
	int32 DelayedEquipSocketChangeSlot = -1;

	/*
	Slot whose socket was not updated immediately when it was holstered.
	We want to wait for the animation to finish.
	*/
	UPROPERTY(Replicated,VisibleAnywhere,BlueprintReadOnly, Category = Utility)
	int32 DelayedHolsterSocketChangeSlot = -1;



	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = Utility)
	UAnimMontage* LastEquipMontage = nullptr;

	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = Utility)
	UAnimMontage* LastHolsterMontage = nullptr;


	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = Utility)
	ACharacter* Owner = nullptr;


	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = Utility)
	UStatManager* OwnerStatManager = nullptr;

	UPROPERTY(Replicated,EditAnywhere,BlueprintReadWrite, Category = WeaponInventory)
	TArray<AWeaponActor*> Weapons;


	/*
	FUNCTIONS. Note Initialize is not in alphabetical order because its important.
	PlayMontage is also out of order because it is the only static function here.
	*/

	/*
	Get and sets Owner variable.
	Fills EquipDataDictionary from WeaponEquipData
	*/
	UFUNCTION(BlueprintCallable, Category = Initialize)
	void Initialize();
	
	/*
	Plays montage safely, and handles nullptr, negative playrate. 
	return OwnerChar->PlayAnimMontage(Montage,PlayRate,StartSectionName);
	returns 0.0f if can't play the montage cause of nullptrs
	*/
	static float PlayMontage( ACharacter* OwnerChar, UAnimMontage* Montage, float PlayRate = 1.0f, FName StartSectionName = FName(""), bool bShowDebugWarnings = false);



	/*
	Will call the server RPC if we have authority OR we are an autonomous proxy
	Will only call RPC if the given weapon is valid and not in an inventory
	
	Server side: Weapon will only be added to the Array
	if EquippableWeaponTypes contains InputWeaponActor->WeaponData.WeaponType. 
	if  InputWeaponActor->WeaponData.WeaponType is a blank FName(), then we equip it ignoring restrictions
	if NumberOfWeaponsAcquired > WeaponSlots, equip will fail server side.
	if we already have an ActiveWeapon, then we holster the current weapon and equip the new weapon
	*/
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void AddWeaponToArray(AWeaponActor* InputWeaponActor);

	/*
	Calls OnActiveWeaponChange
	*/
	UFUNCTION(NetMulticast,Reliable)
	void AddWeaponToArrayMulticast(AWeaponActor* InputWeaponActor);

	UFUNCTION(Server,Reliable)
	void AddWeaponToArrayServer(AWeaponActor* InputWeaponActor);



	/*
	Calls Server RPC if we have authority or ROLE_AutonomousProxy.
	*/
	void AttachWeaponToOwner(AWeaponActor* InputWeaponActor,FName SocketName);

	/*
	Attaches the given weapon to the owner mesh at the desired socket.
	Also changes InputWeaponActor->IsInWeaponInventory = true;
	This method will fail if InputWeaponActor->IsInWeaponInventory = false
	*/
	UFUNCTION(Server,Reliable)
	void AttachWeaponToOwnerServer(AWeaponActor* InputWeaponActor,FName SocketName);






	/*
	returns false if WeaponActor || Owner is a nullptr
	Returns true if pointers are valid
	*/
	bool Check(AWeaponActor* InputWeaponActor);

	/*
	Calls Server RPC if we have authority or ROLE_AutonomousProxy.
	If NumberOfWeapons <= 1, nothing will change
	NextWeaponSlot = (ActiveWeaponSlot - 1);  If < 0, then roll back to NumberOfWeapons-1;
	*/
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void DecreaseActiveSlotIndex(bool bPlayMontage = true);

	UFUNCTION(Server,Reliable)
	void DecreaseActiveSlotIndexServer(bool bPlayMontage = true);

	/*
	Calls Server RPC if we have authority or ROLE_AutonomousProxy.

	Detaches the given weapon from the owner mesh.
	Sets InputWeaponActor->IsInWeaponInventory = false;
	Sets weapon visibility to true.
	*/
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void DetachWeaponFromOwner(AWeaponActor* InputWeaponActor);

	UFUNCTION(Server,Reliable)
	void DetachWeaponFromOwnerServer(AWeaponActor* InputWeaponActor);	


	/*
	Calls Server RPC if we have authority or ROLE_AutonomousProxy.
	No effect if Slot < 0 OR Solt >= Number Of Weapons

	Gets the Montage data from EquipDataDictionary.
	Makes the weapon visible and attaches it to EquipData.ArmedSocket.
	Calls Equip() on InputWeaponActor

	If bImmediatelyChangeActorSocket = true, then InputWeaponActor will be changed to Slot,
	and WeaponActor will immediably change the socket on the mesh.
	If false, then we will set DelayedEquipSocketChangeSlot = Slot if bPlayEquipMontage = true;

	Then the actor will play the montage, and you will call this method via a notify state, except we 
	input  DelayedEquipSocketChangeSlot, bImmediatelyChangeActorSocket = true, bool bPlayEquipMontage = false


	While you can call this method with both parameters false, nothing will happen except OnWeaponEquipped will still be called. 

	*/
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void EquipWeapon(int32 Slot, bool bImmediatelyChangeActorSocket = true, bool bPlayEquipMontage = false);

	UFUNCTION(NetMulticast, Reliable)
	void EquipWeaponMulticast(bool bPlayMontage = true, AWeaponActor* InputWeaponActor = nullptr);

	UFUNCTION(Server,Reliable)
	void EquipWeaponServer(int32 Slot, bool bImmediatelyChangeActorSocket = true, bool bPlayEquipMontage = false);


	/*
	Calls Server RPC if we have authority or ROLE_AutonomousProxy.
	No effect if Slot < 0 OR Solt >= Number Of Weapons

	Gets the Montage data from EquipDataDictionary.
	Makes the weapon visible and attaches it to EquipData.ArmedSocket.
	Calls Holster() on InputWeaponActor

	If bImmediatelyChangeActorSocket = true, then InputWeaponActor will be changed to Slot,
	and WeaponActor will immediably change the socket on the mesh.
	If false, then we will set DelayedHolsterSocketChangeSlot = Slot if bPlayEquipMontage = true;

	Then the actor will play the montage, and you will call this method via a notify state, except we 
	input  DelayedHolsterSocketChangeSlot, bImmediatelyChangeActorSocket = true, bool bPlayEquipMontage = false


	While you can call this method with both parameters false, nothing will happen except OnWeaponHolstered will still be called. 

	*/
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void HolsterWeapon(int32 Slot, bool bImmediatelyChangeActorSocket = true, bool bPlayHolsterMontage = false);
	
	UFUNCTION(NetMulticast, Reliable)
	void HolsterWeaponMulticast(bool bPlayMontage = true,AWeaponActor* InputWeaponActor = nullptr);

	UFUNCTION(Server,Reliable)
	void HolsterWeaponServer(int32 Slot, bool bImmediatelyChangeActorSocket = true, bool bPlayHolsterMontage = false);



	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void IncreaseActiveSlotIndex(bool bPlayMontage = true);

	UFUNCTION(Server,Reliable)
	void IncreaseActiveSlotIndexServer(bool bPlayMontage = true);


	bool IsWeaponVisible(AWeaponActor* InputWeaponActor);

	UFUNCTION()
	void OnRep_ActiveWeaponSlot();

	UFUNCTION()
	void OnRep_ActiveWeapon();

	UFUNCTION()
	void OnRep_CurrentEquipWeaponType();

	UFUNCTION()
	void OnRep_EquipState();



	

	bool PlayEquipMontage(AWeaponActor* InputWeaponActor);

	bool PlayEquipMontage(FName WeaponType);

	bool PlayHolsterMontage(AWeaponActor* InputWeaponActor);

	bool PlayHolsterMontage(FName WeaponType);

	/*
	Searches the weapon array for a weapon with the NewWeapon class in its WeaponData
	If found, calls SetActiveEquipSlot() with the found index.
	Useful when you want to equip a weapon of specific type, whereever it is the weapon list
	*/
	UFUNCTION(BlueprintCallable, Category = "Weapons") 
	void SetActiveEquipSlotByType(FName NewWeaponType, bool bPlayEquipMontage = true);

	/*
	Calls Server RPC if we have authority or ROLE_AutonomousProxy.
	*/
	UFUNCTION(BlueprintCallable, Category = "Weapons") 
	void SetActiveEquipSlot(int32 NewEquipSlot, bool bPlayEquipMontage = true);

	UFUNCTION(Server,Reliable)
	void SetActiveEquipSlotServer(int32 NewEquipSlot, bool bPlayEquipMontage = true);

	/*
	Returns true if WeaponActor->GetRootComponent()->SetHiddenInGame() was successful.
	NewWeaponVisibility = true means visible during gameplay
	NewWeaponVisibility = false means NOT visible during gameplay
	
	Calls Server RPC if we have authority or ROLE_AutonomousProxy.

	Since we handle the visibility code in a multicast, we don't have to
	have the skeletal mesh replicate. If visibility was only handled in the server RPC, 
	then the skeletal mesh would have to replicate to propagate the visibility change
	*/
	UFUNCTION(BlueprintCallable, Category = "Weapons") 
	void SetWeaponVisibility(bool NewWeaponVisibility,AWeaponActor* InputWeaponActor);


	UFUNCTION( NetMulticast, Reliable)
	void SetWeaponVisibilityMulticast(bool NewWeaponVisibility,AWeaponActor* InputWeaponActor);

	UFUNCTION(Server,Reliable)
	void SetWeaponVisibilityServer(bool NewWeaponVisibility,AWeaponActor* InputWeaponActor);


	/*
	Calls Server RPC if we have authority or ROLE_AutonomousProxy.


	Spawns the weapon on the server, and tries to add it to the array.
	Weapon spawning is handled server side, and the weapon actor is set to replicate so it
	will appear to all clients.
	Note the spawn location is OwnerLocation + FVector(0.0f,0.0f,600.0f); 
	I didn't bother exposing it because ESpawnActorCollisionHandlingMethod::AlwaysSpawn, and  
	the spawned actor is immediately passed to AddWeaponToArray(NewWeaponActor)
	*/
	UFUNCTION(BlueprintCallable, Category = "Weapons")
	void SpawnWeapon(TSubclassOf<class AWeaponActor> WeaponType);

	
	UFUNCTION(Server,Reliable)
	void SpawnWeaponServer(TSubclassOf<class AWeaponActor> WeaponType);
	

	/*
	Call this server side if you want to properly GenerateStatusEffectSaveData() 
	*/
	UFUNCTION(BlueprintPure,Category = "Saving and Loading")
	TArray<FWeaponSaveData> GenerateWeaponSaveData() const;

	/*

	Destroys all current weapons
	Spawns new weapons with SpawnWeapon()
	Then does the following:
	ActiveWeaponSlot = NewActiveWeaponSlot;
	EquipWeapon(ActiveWeaponSlot,true,false);
	OnFinishSpawningWeapons.Broadcast();

	Note it doesn't manange the weapon stats.
	*/
	UFUNCTION(BlueprintCallable,Category = "Saving and Loading")
	void SpawnWeaponsFromSavedData(UPARAM(ref) const TArray<FWeaponSaveData>& NewWeaponData, int32 NewActiveWeaponSlot );
	
	/*
	Call once you finished spawning all the actors.
	I created a delegate (OnFinishSpawningWeapons) for you to make the timing easier.
	*/
	UFUNCTION(BlueprintCallable,Category = "Saving and Loading")
	void LoadWeaponSavedData(UPARAM(ref) const TArray<FWeaponSaveData>& NewWeaponData);





};
