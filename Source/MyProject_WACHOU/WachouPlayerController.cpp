// ============================================================
// WachouPlayerController.cpp  -  MyProject_WACHOU
// ============================================================
// Implementazione controller giocatore.
//
//   FUNZIONI:
//     SetupInputComponent() → binding LeftMouseButton diretto
//     OnClickSinistro()     → routing in 3 step:
//       1. ClickSuConfigMappa() → schermata config
//       2. ClickSuPulsanteTerminaTurno() → fine turno
//       3. GetCellaCliccata() → cella griglia
//     GetCellaCliccata()    → LineTrace mouse → mondo →
//                             coordinata griglia (X,Y)
// ============================================================
#include "WachouPlayerController.h"
#include "GameManager.h"
#include "GridMapGenerator.h"
#include "WachouHUD.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AWachouPlayerController::AWachouPlayerController()
{
    bShowMouseCursor   = true;
    bEnableClickEvents = true;
}

void AWachouPlayerController::BeginPlay()
{
    Super::BeginPlay();
    SetInputMode(FInputModeGameAndUI());

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(), AGameManager::StaticClass(), Found);
    if (Found.Num() > 0)
        GameManager = Cast<AGameManager>(Found[0]);
}

void AWachouPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed,
        this, &AWachouPlayerController::OnClickSinistro);
}

void AWachouPlayerController::OnClickSinistro()
{
    if (!GameManager) return;

    float MX, MY;
    GetMousePosition(MX, MY);

    // Controlla prima la schermata configurazione
    if (AWachouHUD* HUD = Cast<AWachouHUD>(GetHUD()))
    {
        if (HUD->ClickSuConfigMappa(MX, MY))
            return;

        if (HUD->ClickSuPulsanteTerminaTurno(MX, MY))
        {
            GameManager->TerminaTurno();
            return;
        }
    }

    // Altrimenti gestisci il click sulla mappa
    int32 X = -1, Y = -1;
    if (GetCellaCliccata(X, Y))
        GameManager->OnCellaCliccata(X, Y);
}

bool AWachouPlayerController::GetCellaCliccata(int32& OutX, int32& OutY) const
{
    FVector Loc, Dir;
    if (!DeprojectMousePositionToWorld(Loc, Dir)) return false;

    FHitResult Hit;
    FCollisionQueryParams P;
    P.bTraceComplex = false;

    if (!GetWorld()->LineTraceSingleByChannel(
            Hit, Loc, Loc + Dir * 100000.f, ECC_Visibility, P))
        return false;

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(), AGridMapGenerator::StaticClass(), Found);
    if (Found.IsEmpty()) return false;

    AGridMapGenerator* Mappa = Cast<AGridMapGenerator>(Found[0]);
    if (!Mappa) return false;

    const float CS   = Mappa->DimensioneCella;
    const float Half = (Mappa->DimensioneGriglia - 1) * CS * 0.5f;
    const FVector Local = Hit.Location - Mappa->GetActorLocation();

    OutX = FMath::FloorToInt((Local.X + Half) / CS);
    OutY = FMath::FloorToInt((Local.Y + Half) / CS);

    return Mappa->IsValidCoord(OutX, OutY);
}
