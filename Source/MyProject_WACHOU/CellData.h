// ============================================================
// CellData.h  -  MyProject_WACHOU
// ============================================================
// Struttura dati di UNA singola cella della griglia 25x25.
// Ogni cella contiene:
//   - Coordinate (X, Y) della griglia
//   - Elevazione 0-4 (0=acqua, 1=piano, 2-4=montagne)
//   - Flag bHaTorre (true se ostacolo torre)
//   - PosizioneMondo (FVector 3D per il rendering)
//
// Funzioni di query rapide:
//   IsCalpestabile() → false se acqua o torre
//   IsAcqua()        → true se elevazione=0
//   IsMontagna()     → true se elevazione >= 2
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "CellData.generated.h"

// USTRUCT() dice a Unreal che questa e' una struttura dati.
// NON usiamo BlueprintType perche' vogliamo solo C++.
USTRUCT()
struct MYPROJECT_WACHOU_API FCellData
{
    GENERATED_BODY()

    // ── Posizione nella griglia ──────────────────────────────
    // Es: X=0,Y=0 e' l'angolo in basso a sinistra
    //     X=24,Y=24 e' l'angolo in alto a destra
    UPROPERTY() int32 X = 0;
    UPROPERTY() int32 Y = 0;

    // ── Livello di elevazione ────────────────────────────────
    // 0 = acqua       → colore BLU    → non calpestabile
    // 1 = piano       → colore VERDE  → calpestabile
    // 2 = mt. bassa   → colore GIALLO → calpestabile
    // 3 = mt. media   → colore ARANCIO→ calpestabile
    // 4 = vetta       → colore ROSSO  → calpestabile
    UPROPERTY() int32 Elevazione = 0;

    // ── Torre ────────────────────────────────────────────────
    // true = c'e' una torre su questa cella → non calpestabile
    UPROPERTY() bool bHaTorre = false;

    // ── Posizione nel mondo 3D ───────────────────────────────
    // Calcolata da CalcolaPosizioneMondo() nel .cpp
    // Usata per piazzare il cubo visivo nella scena
    UPROPERTY() FVector PosizioneMondo = FVector::ZeroVector;

    // ============================================================
    // COSTRUTTORI
    // ============================================================

    // Costruttore vuoto (richiesto da Unreal per le USTRUCT)
    FCellData() = default;

    // Costruttore con parametri: crea una cella con X, Y e altezza
    FCellData(int32 InX, int32 InY, int32 InElevazione)
        : X(InX)
        , Y(InY)
        , Elevazione(InElevazione)
        , bHaTorre(false)
        , PosizioneMondo(FVector::ZeroVector)
    {}

    // ============================================================
    // FUNZIONI DI CONTROLLO
    // Queste funzioni NON salvano dati extra: calcolano
    // il risultato al volo dai campi gia' presenti.
    // ============================================================

    // Un'unita' puo' camminare su questa cella?
    // NO se e' acqua (Elevazione==0) o se c'e' una torre
    FORCEINLINE bool IsCalpestabile() const
    {
        return Elevazione > 0 && !bHaTorre;
    }

    // Questa cella e' acqua?
    FORCEINLINE bool IsAcqua() const { return Elevazione == 0; }

    // Questa cella e' montagna (livello 2, 3 o 4)?
    FORCEINLINE bool IsMontagna() const { return Elevazione >= 2; }

    // Questa cella ha una torre sopra?
    FORCEINLINE bool HasTower() const { return bHaTorre; }

    // ============================================================
    // OPERATORI
    // ============================================================

    // Confronta due celle per coordinata (utile per cercare in array)
    bool operator==(const FCellData& O) const { return X == O.X && Y == O.Y; }
    bool operator!=(const FCellData& O) const { return !(*this == O); }
};
