// Copyright Zachary Kolansky, 2020

#pragma once
#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Engine/DataTable.h"
#include "Math/Color.h"
#include "StatDataStructures.generated.h"


/*
Stat management
*/

UENUM(BlueprintType)
enum class EStatValueType : uint8 {CurrentValue,Mininum,Maxinum};

UENUM(BlueprintType)
enum class EStatModificationOperation : uint8 {Addition,Subtraction,Multiplication,Division,Replacement};

/*
Different kinds of things an effect can do.
*/
UENUM(BlueprintType)
enum class EEffectAction : uint8 {ModifyStat,CustomOne,CustomTwo,CustomThree};

UENUM(BlueprintType)
enum class EStatusClearCondition: uint8 {ReceiveDamage,MontageStart,MontageBlendOut,MontageEnd,CustomOne,CustomTwo,CustomThree};


USTRUCT(BlueprintType)
struct FStat : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Minimum = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Maximum = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentValue = 0.0f;

	FStat()
	{

	}
	FStat(const float Current, const float Min,const float Max )
	{
		if(Max < Min )
		{
			Maximum = Min;
			Minimum = Max;
		}
		else
		{
			Minimum = Min;
			Maximum = Max;
		}
		
	
		CurrentValue = FMath::Clamp(Current,Min,Max);

	}
	void ClampCurrentValue()
	{
		
		CurrentValue = FMath::Clamp(CurrentValue,Minimum,Maximum);
	}
	FORCEINLINE FStat operator+(const float &Other ) const
	{
		FStat NewStat = *this;
		NewStat.CurrentValue += Other;
		NewStat.ClampCurrentValue();
		return NewStat;
	}
	FORCEINLINE FStat operator-(const float &Other ) const
	{
		FStat NewStat = *this;
		NewStat.CurrentValue -= Other;
		NewStat.ClampCurrentValue();
		return NewStat;
	}
	FORCEINLINE FStat operator*(const float &Other ) const
	{
		FStat NewStat = *this;
		NewStat.CurrentValue *= Other;
		NewStat.ClampCurrentValue();
		return NewStat;
	}
	FORCEINLINE FStat operator / (const float &Other ) const
	{
		if(Other == 0.0f)
		{
			return *this;
		}
		FStat NewStat = *this;
		NewStat.CurrentValue /= Other;
		NewStat.ClampCurrentValue();
		return NewStat;
	}
	FORCEINLINE FStat operator = (const float &Other ) const
	{
		FStat NewStat = *this;
		NewStat.CurrentValue = Other;
		NewStat.ClampCurrentValue();
		return NewStat;
	}
	FORCEINLINE operator float() const
	{
		return CurrentValue;
	}
	FORCEINLINE bool operator==(const float &Other) const 
    {
        return (CurrentValue == Other );
    }
	FORCEINLINE bool operator>=(const float &Other) const 
    {
        return (CurrentValue >= Other );
    }
	FORCEINLINE bool operator<=(const float &Other) const 
    {
        return (CurrentValue <= Other );
    }
	FORCEINLINE bool operator<(const float &Other) const 
    {
        return (CurrentValue < Other );
    }
	FORCEINLINE bool operator>(const float &Other) const 
    {
        return (CurrentValue > Other );
    }
	FORCEINLINE bool operator!=(const float &Other) const 
    {
        return (CurrentValue != Other );
    }
	FORCEINLINE bool operator==(const FStat &Other) const 
    {
        return (CurrentValue == Other.CurrentValue && Minimum == Other.Minimum && Maximum == Other.Maximum  );
    }
	FORCEINLINE bool operator!=(const FStat &Other) const 
    {
        return !(CurrentValue == Other.CurrentValue && Minimum == Other.Minimum && Maximum == Other.Maximum  );
    }
	
	/*
	Doesn't account for negative numbers, 
	if Minimum becomes > Maximum, it is then capped at Maximum
	*/
	void AddValue(const float Value, const EStatValueType TypeToModify = EStatValueType::CurrentValue)
	{
		switch(TypeToModify)
		{
			case EStatValueType::CurrentValue:
			{
				CurrentValue+=Value;
				break;
			}
			case EStatValueType::Mininum:
			{
				Minimum+=Value;
				break;
			}
			case EStatValueType::Maxinum:
			{
				Maximum+=Value;
				break;
			}
		}

		if(Minimum > Maximum)
		{
			Minimum = Maximum;
		}
		ClampCurrentValue();
	}
	void SubtractValue(const float Value, const EStatValueType TypeToModify = EStatValueType::CurrentValue)
	{
		switch(TypeToModify)
		{
			case EStatValueType::CurrentValue:
			{
				CurrentValue-=Value;
				break;
			}
			case EStatValueType::Mininum:
			{
				Minimum-=Value;
				break;
			}
			case EStatValueType::Maxinum:
			{
				Maximum-=Value;
				break;
			}
		}

		if(Minimum > Maximum)
		{
			Minimum = Maximum;
		}
		ClampCurrentValue();
	}
	void MultiplyValue(const float Value, const EStatValueType TypeToModify = EStatValueType::CurrentValue)
	{
		switch(TypeToModify)
		{
			case EStatValueType::CurrentValue:
			{
				CurrentValue*=Value;
				break;
			}
			case EStatValueType::Mininum:
			{
				Minimum*=Value;
				break;
			}
			case EStatValueType::Maxinum:
			{
				Maximum*=Value;
				break;
			}
		}

		if(Minimum > Maximum)
		{
			Minimum = Maximum;
		}
		ClampCurrentValue();
	}
	/*
	if Value = 0.0f; we return
	*/
	void DivideValue(const float Value, const EStatValueType TypeToModify = EStatValueType::CurrentValue)
	{
		if(Value == 0.0f)
		{
			return;
		}
		
		switch(TypeToModify)
		{
			case EStatValueType::CurrentValue:
			{
				CurrentValue/=Value;
				break;
			}
			case EStatValueType::Mininum:
			{
				Minimum/=Value;
				break;
			}
			case EStatValueType::Maxinum:
			{
				Maximum/=Value;
				break;
			}
		}

		if(Minimum > Maximum)
		{
			Minimum = Maximum;
		}
		ClampCurrentValue();
	}
	void ReplaceValue(const float Value, const EStatValueType TypeToModify = EStatValueType::CurrentValue)
	{
		switch(TypeToModify)
		{
			case EStatValueType::CurrentValue:
			{
				CurrentValue=Value;
				break;
			}
			case EStatValueType::Mininum:
			{
				Minimum=Value;
				break;
			}
			case EStatValueType::Maxinum:
			{
				Maximum=Value;
				break;
			}
		}

		if(Minimum > Maximum)
		{
			Minimum = Maximum;
		}
		ClampCurrentValue();
	}
};



/*
Data structure for handling effects with a duration.
*/
USTRUCT(BlueprintType)
struct FStatusEffect : public FTableRowBase
{
    GENERATED_BODY()
    
    /*
    What the effect of a status effect is. Note that the action must be ModifyStat for it be handled 
    by the StatManager component. 
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EEffectAction EffectAction = EEffectAction::ModifyStat;

    /*
    If EffectAction is set to ModifyStat, this name will the name of the stat that it changes.
    For EffectAction being set to FireProjectile,LaunchSelf this variable will be used to look up data
    from the necessary table.
	//TODO, create look up data for projectiles, launching self
    No effect for ModifyPlayrate,ModifyCooldown,ModifyCastingCost.
    These actions are used change values in FSpellExecutionStruct
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ActionName = FName("");

    /*
    How much to modify the Stat or ModifyPlayrate,ModifyCooldown,ModifyCastingCost. 
    No effect for FireProjectile,LaunchSelf.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Intensity = 0.0f;

    /*
    Apply this effect every X seconds. No effect if duration is set to zero.
	Note that a multicast RPC is called for every X seconds.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TickTime = 0.1f;

    /*
    Time in seconds how long an effect lasts.
    Negative Duration means the effect will last forever.
    Duration = 0 means the effect will not be applied
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Duration = 0.0f;

    /*
    Other clear conditions besides duration
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<EStatusClearCondition> ClearConditions = {};

    /*
    Visual Effects to apply when status effect has been started
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName VisualEffect = FName("");

    FStatusEffect()
    {

    }
};


/*
Used to handle stat binding. For example, a sword could have two stat, BaseDamage and HitCount.
BaseDamage could be bound to hit count, so that we have a sword that increases its damage
as HitCount increases.

Meant to be used with a map. The key is the bound stat, and binding target is the value.
The name will be used to find the stat in its component manager.

*/
USTRUCT(BlueprintType)
struct FStatBind
{
	GENERATED_BODY()

	/*
	The stat whose properties we will combine to.
	(The Modifier)
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName NameOfStatBindingModifier = FName("");

	/*
	What aspect of the base stat do we modify?
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EStatValueType Target = EStatValueType::CurrentValue;

	/*
	How do we modify the stat
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EStatModificationOperation BindingOperation = EStatModificationOperation::Addition;

	FStatBind()
	{

	}
	FStat CalculateNewStat(const FStat& Base,const FStat& ModifierStat) const
	{
		FStat NewStat = Base;
		
		float CurrentValueOfModifier = ModifierStat.CurrentValue;

		switch(BindingOperation)
		{
			case EStatModificationOperation::Addition:
			{
				NewStat.AddValue(CurrentValueOfModifier,Target);
				break;
			}
			case EStatModificationOperation::Subtraction:
			{
				NewStat.SubtractValue(CurrentValueOfModifier,Target);
				break;
			}
			case EStatModificationOperation::Multiplication:
			{
				NewStat.MultiplyValue(CurrentValueOfModifier,Target);
				break;
			}
			case EStatModificationOperation::Division:
			{
				NewStat.DivideValue(CurrentValueOfModifier,Target);
				break;
			}
			case EStatModificationOperation::Replacement:
			{
				NewStat.ReplaceValue(CurrentValueOfModifier,Target);
				break;
			}		
		}

		return NewStat;


	}


};





USTRUCT(BlueprintType)
struct FStatusEffectVisual : public FTableRowBase
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color1 = FLinearColor(0.0f,0.0f,0.0f,1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Color2 = FLinearColor(0.0f,0.0f,0.0f,1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinEmission = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxEmission = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EmissionChangeSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ColorChangeSpeed = 0.0f;

	FStatusEffectVisual()
	{

	}
	FORCEINLINE FStatusEffectVisual operator+(const FStatusEffectVisual &Other ) const
	{
		FStatusEffectVisual NewVisual = *this;
		NewVisual.Color1 = NewVisual.Color1 + Other.Color1;
		NewVisual.Color2 = NewVisual.Color2 + Other.Color2;
		NewVisual.MinEmission= NewVisual.MinEmission+ Other.MinEmission;
		NewVisual.MaxEmission= NewVisual.MaxEmission+ Other.MaxEmission;
		NewVisual.EmissionChangeSpeed= NewVisual.EmissionChangeSpeed+ Other.EmissionChangeSpeed;
		NewVisual.ColorChangeSpeed= NewVisual.ColorChangeSpeed+ Other.ColorChangeSpeed;
		return NewVisual;
	}
	FORCEINLINE FStatusEffectVisual operator / (const int32 &Other ) const
	{
		FStatusEffectVisual NewVisual = *this;
		if(Other == 0)
		{
			return NewVisual;
		}
		NewVisual.Color1 = NewVisual.Color1 / Other;
		NewVisual.Color2 = NewVisual.Color2  / Other;
		NewVisual.MinEmission= NewVisual.MinEmission / Other;
		NewVisual.MaxEmission= NewVisual.MaxEmission / Other;
		NewVisual.EmissionChangeSpeed= NewVisual.EmissionChangeSpeed / Other;
		NewVisual.ColorChangeSpeed= NewVisual.ColorChangeSpeed / Other;


		return NewVisual;
	}

};

USTRUCT(BlueprintType)
struct FStatusEffectVisualFNameParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Color1ParameterName = FName("StatusColor1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Color2ParameterName = FName("StatusColor2");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MinEmissionParameterName = FName("StatusMinEmission");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MaxEmissionParameterName = FName("StatusMaxEmission");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName EmissionChangeSpeedParameterName = FName("StatusEmissionChangeSpeed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ColorChangeSpeedParameterName = FName("StatusColorChangeSpeed");

	FStatusEffectVisualFNameParameters()
	{

	}
}; 



USTRUCT(BlueprintType)
struct FStatusEffectSaveData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FStatusEffect EffectData = FStatusEffect();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Effector = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName EffectName = FName("");

	/*
	GetWorld()->GetTimeSeconds() when the effect has started
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectStartTime = 0.0f;

	/*
	GetWorld()->GetTimeSeconds() when we saved the effect
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EffectSaveTime = 0.0f;

	FStatusEffectSaveData()
	{

	}
	FStatusEffectSaveData(const FStatusEffect& InputStatusEffect, float InputEffector, FName InputEffectName, float InputEffectStartTime, float InputEffectSaveTime  )
	{
		EffectData = InputStatusEffect;
		Effector = InputEffector;
		EffectName = InputEffectName;
		EffectStartTime = InputEffectStartTime;
		EffectSaveTime = InputEffectSaveTime;

	}
};

/*
Saving 
*/

USTRUCT(BlueprintType)
struct FWeaponSaveData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class AWeaponActor> WeaponClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	TMap<FName,FStat> WeaponStats;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = Stats)
	TMap<FName,FStatBind> WeaponStatBindings = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats)
	TArray<FStatusEffectSaveData> WeaponStatusEffects = {};

	

	FWeaponSaveData()
	{

	}

};

USTRUCT(BlueprintType)
struct FCharacterSaveData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = Character)
	TSubclassOf<class ACharacter> CharacterClass = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = Character)
	FTransform Transform = FTransform();

	UPROPERTY(BlueprintReadWrite, Category = Weapon)
	TArray<FWeaponSaveData> WeaponData = {};

	UPROPERTY(BlueprintReadWrite, Category = Weapon)
	int32 ActiveWeaponSlot = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = Stats)
	TMap<FName,FStatBind> StatBindings = {};

	UPROPERTY(BlueprintReadWrite, Category = Stat)
	TMap<FName,FStat> StatDictionary = {};

	UPROPERTY(BlueprintReadWrite, Category = Stat)
	TArray<FStatusEffectSaveData> StatusEffects = {};

	FCharacterSaveData()
	{

	}


};