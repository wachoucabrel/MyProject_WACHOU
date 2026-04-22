// ============================================================
// WachouGameMode.h  -  MyProject_WACHOU
// ============================================================
// Game Mode minimale che registra:
//   - PlayerControllerClass = AWachouPlayerController
//   - HUDClass              = AWachouHUD
//
// Non contiene logica di gioco - tutto in GameManager.
// ============================================================
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WachouGameMode.generated.h"

UCLASS()
class MYPROJECT_WACHOU_API AWachouGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AWachouGameMode();
};
