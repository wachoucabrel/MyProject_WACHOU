// ============================================================
// WachouGameMode.cpp  -  MyProject_WACHOU
// ============================================================
// Costruttore: assegna controller e HUD personalizzati.
// ============================================================
#include "WachouGameMode.h"
#include "WachouPlayerController.h"
#include "WachouHUD.h"

AWachouGameMode::AWachouGameMode()
{
    PlayerControllerClass = AWachouPlayerController::StaticClass();
    HUDClass              = AWachouHUD::StaticClass();
}
