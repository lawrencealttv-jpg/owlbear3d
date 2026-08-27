#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VTTGameMode.generated.h"

UCLASS()
class OWLBEAR3D_API AVTTGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AVTTGameMode();

protected:
    virtual void BeginPlay() override;
};

