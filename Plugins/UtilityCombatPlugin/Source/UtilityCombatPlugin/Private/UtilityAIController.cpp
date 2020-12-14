// Copyright Zachary Kolansky, 2020


#include "UtilityAIController.h"
#include "Kismet/KismetMathLibrary.h"



AUtilityAIController::AUtilityAIController()
{
    SetGenericTeamId(FGenericTeamId(TeamIDNumber));
    

}

FVector AUtilityAIController::GetFocalPointOnActor(const AActor* Actor) const
{
    if( Actor && UKismetMathLibrary::ClassIsChildOf(Actor->GetClass(),APawn::StaticClass()) )
    {
        return (Actor->GetActorLocation() + FVector(0.0f,0.0f,FocusEyeHeight)* (Actor->GetActorScale() ).Z  );
    }
    
    return Super::GetFocalPointOnActor(Actor);
} 


 ETeamAttitude::Type AUtilityAIController::GetTeamAttitudeTowards(const AActor& Other) const 
 {
    if (const APawn* OtherPawn = Cast<APawn>(&Other)) 
    {
        if (const IGenericTeamAgentInterface* OtherTeamAgent = Cast<IGenericTeamAgentInterface>(OtherPawn->GetController()))
        {
            //FGenericTeamId OtherTeamID = OtherTeamAgent->GetGenericTeamId();
            /*
            By default the GetTeamAttitudeTowards compares the TeamIDs 
            of the sensed actor with the teamID specified in this controller, 
            if they are different they will be considered hostile to each other.
            */
            return Super::GetTeamAttitudeTowards(*OtherPawn->GetController());
        }
    }

    return ETeamAttitude::Neutral;
}

void AUtilityAIController::SetTeamID(int32 NewTeamID)
{
    TeamIDNumber = NewTeamID;
    SetGenericTeamId(FGenericTeamId(TeamIDNumber));
}