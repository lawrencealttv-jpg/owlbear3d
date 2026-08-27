#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "VTTCameraPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS()
class OWLBEAR3D_API AVTTCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    AVTTCameraPawn();

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> Camera;

    float ForwardInput = 0.0f;
    float RightInput = 0.0f;
    float PanSpeed = 900.0f;

    virtual void Tick(float DeltaSeconds) override;
    void SetForwardInput(float Value);
    void SetRightInput(float Value);
    void Zoom(float Value);
    void RotateLeft();
    void RotateRight();
};

