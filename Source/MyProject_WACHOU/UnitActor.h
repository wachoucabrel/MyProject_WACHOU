// ============================================================
// UnitActor.h  -  MyProject_WACHOU
// ============================================================
// Actor visivo che rappresenta UNA singola unita' sulla mappa.
//
//   COMPONENTI:
//     CorpoMesh       → sfera colorata (M_Unita material)
//     BarraHPMesh     → cubo verticale che indica HP residui
//     AnelloSelezione → cubo piatto giallo sotto l'unita'
//
//   API:
//     Init()              → inizializza tipo, colore, dimensioni
//     SetPosizioneGriglia() → posizione istantanea (al piazzamento)
//     AnimaMovimento()    → avvia animazione lungo percorso
//     SetSelezionata()    → mostra/nasconde anello selezione
//     AggiornaHP()        → ridimensiona/ricolora barra HP
//
//   COLORI:
//     Ciano  = giocatore Human
//     Rosso  = giocatore AI
//     Giallo = anello selezione
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnitData.h"
#include "UnitActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class MYPROJECT_WACHOU_API AUnitActor : public AActor
{
    GENERATED_BODY()

public:
    AUnitActor();
    virtual void Tick(float DeltaTime) override;

    void Init(const FUnita& Unita, float DimensioneCella);
    void SetPosizioneGriglia(int32 GridX, int32 GridY,
                             float DimensioneCella, float HalfGrid);
    void AnimaMovimento(const TArray<TPair<int32,int32>>& Percorso,
                        float DimensioneCella, float HalfGrid, float ZBase);
    void SetSelezionata(bool bSel);
    void AggiornaHP(int32 HPCorrente, int32 HPMax);
    bool IsInMovimento() const { return bInMovimento; }
    int32 GetUnitaID() const { return UnitaID; }

private:
    UPROPERTY(VisibleAnywhere) USceneComponent*      Radice;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* CorpoMesh;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* BarraHPMesh;
    UPROPERTY(VisibleAnywhere) UStaticMeshComponent* AnelloSelezione;
    UPROPERTY() UMaterialInstanceDynamic* MatCorpo;
    UPROPERTY() UMaterialInstanceDynamic* MatBarraHP;

    int32 UnitaID = -1;
    FLinearColor ColoreUnita = FLinearColor::White;

    bool  bInMovimento  = false;
    float VelocitaAnim  = 250.f;
    TArray<FVector> PuntiPercorso;
    int32 PuntoCorrente = 0;

    static FLinearColor GetColoreHP(float Pct);
};
