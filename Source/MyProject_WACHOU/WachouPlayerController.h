// ============================================================
// WachouPlayerController.h  -  MyProject_WACHOU
// ============================================================
// Controller del giocatore umano.
//
//   GESTIONE INPUT:
//     - Click sinistro mouse (binding diretto)
//     - Cursore visibile (bShowMouseCursor=true)
//     - Modalita' GameAndUI per accettare click su HUD e mondo
//
//   ROUTING CLICK:
//     1. Schermata Configurazione → +/- parametri
//     2. Pulsante TERMINA TURNO → fine turno
//     3. Cella griglia → GameManager::OnCellaCliccata()
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WachouPlayerController.generated.h"

class AGameManager;

UCLASS()
class MYPROJECT_WACHOU_API AWachouPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AWachouPlayerController();

protected:
    virtual void SetupInputComponent() override;
    virtual void BeginPlay() override;

private:
    void OnClickSinistro();
    bool GetCellaCliccata(int32& OutX, int32& OutY) const;

    UPROPERTY()
    AGameManager* GameManager = nullptr;
};
