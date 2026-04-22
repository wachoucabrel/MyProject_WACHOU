// ============================================================
// GridMapGenerator.h  -  MyProject_WACHOU
// ============================================================
// Actor che genera e gestisce la mappa di gioco 25x25.
//
//   GENERAZIONE:
//     - Algoritmo Perlin Noise fBm con seed casuale
//     - Quantizzazione in 5 livelli di elevazione
//     - Colori esatti da design doc:
//       0=BLU, 1=VERDE, 2=GIALLO, 3=ARANCIO, 4=ROSSO
//     - 3 torri con piazzamento simmetrico adattivo
//     - Verifica connettivita' (no isole)
//
//   API COLORAZIONE RANGE:
//     SetColoreRange()       → colora cella per range mov/attacco
//     ResetTuttiColori()     → ripristina colori originali
//     AggiornaCellaTorre()   → aggiorna colore torre (stato)
//
//   PARAMETRI EDITOR (esposti a Blueprint):
//     SemeCasuale, ScalaPerlin, SogliaAcqua, SogliaPiano
//     → modificabili dalla schermata Configurazione
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CellData.h"
#include "GridMapGenerator.generated.h"

class UCameraComponent;
class UInstancedStaticMeshComponent;

UCLASS()
class MYPROJECT_WACHOU_API AGridMapGenerator : public AActor
{
    GENERATED_BODY()

public:
    AGridMapGenerator();

    virtual void BeginPlay() override;

    // ── Parametri editor ──────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generazione Mappa")
    int32 DimensioneGriglia = 25;

    UPROPERTY(EditAnywhere, Category = "Generazione Mappa")
    float DimensioneCella = 50.f;

    UPROPERTY(EditAnywhere, Category = "Generazione Mappa")
    int32 MaxElevazione = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perlin")
    int32 SemeCasuale = 12345;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perlin")
    float ScalaPerlin = 0.15f;

    UPROPERTY(EditAnywhere, Category = "Perlin")
    int32 Octaves = 6;

    UPROPERTY(EditAnywhere, Category = "Perlin")
    float Persistence = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Perlin")
    float Lacunarity = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soglie")
    float SogliaAcqua = 0.38f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soglie")
    float SogliaPiano = 0.55f;

    UPROPERTY(EditAnywhere, Category = "Colori")
    FLinearColor ColoreAcqua = FLinearColor(0.05f, 0.25f, 0.95f);    // BLU (design doc)

    UPROPERTY(EditAnywhere, Category = "Colori")
    FLinearColor ColorePiano = FLinearColor(0.15f, 0.75f, 0.15f);   // VERDE (design doc)

    UPROPERTY(EditAnywhere, Category = "Colori")
    FLinearColor ColoreMtBassa = FLinearColor(1.0f, 0.95f, 0.1f);   // GIALLO (design doc)

    UPROPERTY(EditAnywhere, Category = "Colori")
    FLinearColor ColoreMtMedia = FLinearColor(1.0f, 0.55f, 0.05f);  // ARANCIO (design doc)

    UPROPERTY(EditAnywhere, Category = "Colori")
    FLinearColor ColoreVetta = FLinearColor(1.0f, 0.15f, 0.15f);    // ROSSO (design doc)

    UPROPERTY(EditAnywhere, Category = "Mesh")
    UStaticMesh* MeshCubo = nullptr;

    UPROPERTY(EditAnywhere, Category = "Mesh")
    UMaterialInterface* MaterialeCelle = nullptr;

    // ── API pubblica ──────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Wachou")
    void GeneraMappa();
    const FCellData* GetCella(int32 X, int32 Y) const;
    bool IsValidCoord(int32 X, int32 Y) const;

    // ── API colorazione range (Step 3) ────────────────────────
    // Colora una cella specifica (blu=movimento, rosso=attacco, giallo=selezionata)
    void SetColoreRange(int32 X, int32 Y, FLinearColor Colore);

    // Ripristina il colore originale di una singola cella
    void ResetColoreCella(int32 X, int32 Y);

    // Ripristina tutti i colori alla normalità
    void ResetTuttiColori();

    // Colori standard per il range
    static FLinearColor GetColoreMovimento() { return FLinearColor(0.1f, 0.4f, 1.0f, 1.f);  }
    static FLinearColor GetColoreAttacco()   { return FLinearColor(1.0f, 0.15f, 0.15f, 1.f); }
    static FLinearColor GetColoreSelezionato(){ return FLinearColor(1.0f, 0.9f, 0.1f, 1.f); }

    // Aggiorna il colore di una torre (0=neutrale,1=human,2=AI,3=contesa)
    void AggiornaCellaTorre(int32 X, int32 Y, int32 Stato);

    // Verifica che tutte le celle calpestabili siano raggiungibili (no isole)
    // Se necessario converte alcune celle acqua in piano per garantire la connettivita'
    void ControllaConnettivitaMappa();

private:
    UPROPERTY()
    TArray<FCellData> GrigliaDati;

    UPROPERTY()
    UInstancedStaticMeshComponent* MeshCelle;

    UPROPERTY()
    UCameraComponent* CameraMappa;

    void GeneraElevazione();
    void PosizionaTorri();
    void CreaMeshCelle();
    void ImpostaCamera();

    float CalcolaPerlinNoise(int32 X, int32 Y);
    FLinearColor GetColoreDaElevazione(int32 Elevazione) const;
    FLinearColor GetColoreDaElevazioneConRange(int32 X, int32 Y) const;

    // Indice 1D nella griglia
    int32 Idx(int32 X, int32 Y) const
    { return Y * DimensioneGriglia + X; }

    // Mappa per tenere traccia delle celle colorate dal range
    // Key = indice 1D, Value = colore range attivo
    TMap<int32, FLinearColor> CelleRange;
};
