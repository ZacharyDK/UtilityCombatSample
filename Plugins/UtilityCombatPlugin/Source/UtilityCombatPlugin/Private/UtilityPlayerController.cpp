// Copyright Zachary Kolansky, 2020


#include "UtilityPlayerController.h"
#include "Runtime/Engine/Public/DrawDebugHelpers.h"

AUtilityPlayerController::AUtilityPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    PlayerTeamId = FGenericTeamId(TeamIDNumber);
}


FHitResult AUtilityPlayerController::GetAimLocationInWorld()
{
    //Out parameters
    FHitResult AimLocation = FHitResult(); 
    int32 ViewportSizeX, ViewportSizeY = 0;


    GetViewportSize(ViewportSizeX,ViewportSizeY);

    FVector2D ScreenLocation = FVector2D(ViewportSizeX*CrossHairXLocation,ViewportSizeY*CrossHairYLocation);
    
    //Out parameters
    FVector Location = FVector();
    FVector CameraDirection = FVector();

    bool IsDeProjectSuccessful = DeprojectScreenPositionToWorld(ScreenLocation.X,ScreenLocation.Y,Location,CameraDirection);
    UWorld* World = GetWorld();

    if(bShowDebugWarnings)
    {
        UE_LOG(LogTemp,Warning,TEXT("CAMERA DIRECTION IS %s"),*(CameraDirection.ToString()))
    }

    FVector StartLocation = PlayerCameraManager->GetCameraLocation();
    FVector EndLocation = StartLocation + CameraDirection*SightLength;

    if(IsDeProjectSuccessful && World)
    {
        World->LineTraceSingleByChannel(
            AimLocation,
            StartLocation,
            EndLocation,
            ECollisionChannel::ECC_Camera
        );

        if(bShowDebugLineTrace)
        {
            DrawDebugLine(
                World,
                StartLocation,
                EndLocation,
                FColor(255,0,0, 1),
                false,
                0.2f,
                1,
                3.0f
            );
        }
    }
    return AimLocation;
}

void AUtilityPlayerController::SetTeamID(int32 NewTeamID)
{
    TeamIDNumber = NewTeamID;
    SetGenericTeamId(FGenericTeamId(TeamIDNumber));
}


/*
INTERFACE IMPLEMENTATION
*/

FGenericTeamId AUtilityPlayerController::GetGenericTeamId() const
{
    return PlayerTeamId;
}

void AUtilityPlayerController::SetGenericTeamId(const FGenericTeamId& TeamID)
{
    PlayerTeamId = TeamID;
}