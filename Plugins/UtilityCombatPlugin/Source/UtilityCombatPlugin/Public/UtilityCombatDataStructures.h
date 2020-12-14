//Copyright Zachary Kolansky, 2020

#pragma once
#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Engine/DataTable.h"
//#include "Math/IntPoint.h"
#include "Curves/CurveFloat.h"
#include "UtilityCombatDataStructures.generated.h"

/*
Always = This task can always be interrupted by another task. Interruption will occur when a better task to do is found.
Never = This task can never be interrupted by another task, no matter what.
Priority = This task can only be interrupted by tasks with a greater priority number.
*/
UENUM(BlueprintType)
enum class EUtilityInterruptionType : uint8 {NEVER,ALWAYS,PRIORITY};



/*
STAT_BY_FNAME Query the Controlled Pawn for a value, providing an FName

IsCoverSafe: bIsCoverPointSafe = false, 0.0f; bIsCoverPointSafe = true, 1.0f;
IsCoverHitResultValid: bIsCoverHitResultValid = false, 0.0f; bIsCoverHitResultValid = true, 1.0f; (Trend continued for all IsX,HasX )
IsCrouch: bIsCrouched
IsFocusAnyAttack: bIsFocusAnyAttacking

IsFocusInMeleeRange: If DistanceToFocus < MeleeRange = true, 1.0f; 0.0f if no Focus or Otherwise
IsFocusInMeleeRangeFallout: If DistanceToFocus < MeleeRangeFallout = true, 1.0f; 0.0f if no Focus or Otherwise
IsFocusOutOfMeleeRange; If DistanceToFocus >= MeleeRange = true, 1.0f; 0.0f if no Focus or Otherwise
IsFocusOutOfMeleeRangeFallout; If DistanceToFocus >= MeleeRangeFallout = true, 1.0f; 0.0f if no Focus or Otherwise

IsFocusMeleeAttack: bIsFocusMeleeAttacking
IsFocusRangeAttack: bIsFocusRangeAttacking
IsInCover: bIsInCover
IsMeleeEquip:bMeleeWeaponArmed

HasFocus: If ControllerFocus isn't nullptr
HasFocusLastSetPoint: bFocusLastDetectedSet
HasPointOfInterest: bCurrentPointOfInterestSet
NormalizedDistanceToCover:  DistanceToCover/CoverPointSearchDistance

NormalizedDistanceToCurrentPointOfInterest: DistanceToCurrentPointOfInterest/MaxDistanceToCurrentPointOfInterest
NormalizedDistanceToFocus: DistanceToFocus/ComfortableDistance
NormalizedDistanceToFocusLastDetected: DistanceToFocusLastDetectedPoint/MaxFocusSearchDistance

*/
UENUM(BlueprintType)
enum class ECurveInputQuery : uint8 {STAT_BY_FNAME,

                                    IsCoverSafe, IsCoverHitResultValid, IsCrouch,
                                    IsFocusAnyAttack, IsFocusInMeleeRange, IsFocusInMeleeRangeFallout, IsFocusOutOfMeleeRange, IsFocusOutOfMeleeRangeFallout,  IsFocusMeleeAttack,IsFocusRangeAttack,
                                    IsInCover, IsMeleeEquip,

                                    HasFocus,HasFocusLastSetPoint,HasPointOfInterest, 
                                    NormalizedDistanceToCover,NormalizedDistanceToCurrentPointOfInterest,NormalizedDistanceToFocus,NormalizedDistanceToFocusLastDetected
                                    
                                    };

/*
While I could keep a running Multiple, this is way easier conceptually.
And will allow for easier debuging
*/

USTRUCT(BlueprintType)
struct FUtilityCurveCollection
{
    GENERATED_BODY()


    /*
    What sort of normalized values we going to input into the curve.
    If CurveInputQuery = ECurveInputQuery::STAT_BY_FNAME. then
    The UtilityCombatTaskComponent  will ask the UtilityAIManagerComponent for a float with the FName as the parameter.
    The ManagerComponent will get a value from a controlled pawn via an interface.

    The other enumarated values aren't stats, buts variables the UtilityAIManagerComponent is responsible for managing. 
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CurveQuery)
    ECurveInputQuery CurveInputQuery = ECurveInputQuery::STAT_BY_FNAME;

    /*
    Only Relevant if CurveInputQuery = ECurveInputQuery::STAT_BY_FNAME
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CurveQuery)
    FName StatName = FName("");

    
    /*
    Inputs and Outputs are normalized from 0-1; 
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Curve)
    UCurveFloat* CurveFloat = nullptr;

    /*
    Multiply the output of the CurveFloat by this number before adding OR multipling to 
    the running total
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Curve)
    float CurveDampen = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Curve)
    float CurveOutput = 0.0f;

    /*
    If true, the output of the curve will be multiplied to the running total of all previous curves.
    If false, it will be added.
    NOTE: I won't bother to clamp the curves between 0-1.0f. Its on you to make sure the math works.
    NOTE2: A bunch of curves multiplied together is like an AND statement. Adding the output is like 
    using OR statement against all the previous curves.
    NOTE3: The curves output will be computed in the order they are in list. This means when you make some
    curves act as in OR operator, the sequence may no longer be communitive. 
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Curve)
    bool bMultiplyThisCurveOutputToRunningTotal = true;


    
    FUtilityCurveCollection()
    {

    }
};