// ============================================================
// WachouHUD.h  -  MyProject_WACHOU
// ============================================================
// HUD che disegna tutta l'interfaccia su Canvas (no UMG).
//
//   PANNELLI:
//     - Schermata Configurazione (Seed, Perlin, Soglie, INIZIA)
//     - Titolo turno + status
//     - Lista unita' Human (ciano) / AI (rosso) con coord+HP
//     - Pannello torri con conteggio per giocatore
//     - Istruzioni contestuali
//     - Pulsante TERMINA TURNO
//     - Counter celle raggiungibili / nemici attaccabili
//     - Schermata fine partita con vincitore
//
//   API CLICK:
//     ClickSuConfigMappa()           → +/- parametri o INIZIA
//     ClickSuPulsanteTerminaTurno()  → fine turno
//
//   STRUCT:
//     FPulsanteConfig → posizione, dimensione, tipo (0-8)
// ============================================================
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WachouHUD.generated.h"

class AGameManager;
class AGridMapGenerator;

UCLASS()
class MYPROJECT_WACHOU_API AWachouHUD : public AHUD
{
    GENERATED_BODY()
public:
    AWachouHUD();
    virtual void DrawHUD() override;
    virtual void BeginPlay() override;

    // Chiamato dal PlayerController
    bool ClickSuPulsanteTerminaTurno(float MX, float MY) const;
    bool ClickSuConfigMappa(float MX, float MY);

private:
    UPROPERTY() AGameManager*      GameManager  = nullptr;
    UPROPERTY() AGridMapGenerator* GridMappa    = nullptr;

    // Pulsante Termina Turno
    mutable FVector2D PulsantePos = FVector2D::ZeroVector;
    mutable FVector2D PulsanteDim = FVector2D(230.f, 44.f);
    mutable bool bPulsanteVisibile = false;

    // Schermata configurazione
    struct FPulsanteConfig
    {
        FVector2D Pos;
        FVector2D Dim;
        int32     Tipo; // 0=SeedUp 1=SeedDn 2=PerlinUp 3=PerlinDn
                        // 4=AcquaUp 5=AcquaDn 6=PianoUp 7=PianoDn 8=Inizia
    };
    TArray<FPulsanteConfig> PulsantiConfig;

    void DisegnaSchermataConfig();
    void DisegnaPulsante(float X, float Y, float W, float H,
                         const FString& Label, FLinearColor Colore,
                         int32 Tipo);

    void DisegnaTesto(const FString& T, float X, float Y,
                      FLinearColor C = FLinearColor::White, float S = 1.0f);
    void DisegnaRettangoloSfondo(float X, float Y, float W, float H,
                                  FLinearColor C);
};
