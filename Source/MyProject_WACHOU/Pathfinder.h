// ============================================================
// Pathfinder.h  -  MyProject_WACHOU
// ============================================================
// Algoritmi di pathfinding per il gioco WACHOU.
// Implementati SENZA STL (no std::queue / priority_queue)
// per compatibilita' con il linker di Unreal Engine.
//
//   API:
//     CalcolaRaggiungibili() → Dijkstra con costi elevazione
//                              (piano=1, salita=2, discesa=1)
//     TrovaPercorso()        → ricostruzione cammino A→B
//     CalcolaRangeAttacco()  → BFS con regole speciali:
//                              - Sniper attraversa acqua
//                              - Brawler bloccato da acqua
//                              - Solo nemici stesso livello o inferiore
//     IsCellaRaggiungibile() → query rapida
//     CostoMovimento()       → costo singola transizione
//
//   STRUCT:
//     FCellaRaggiungibile → (X, Y, Costo) per range mov.
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "UnitData.h"

class AGridMapGenerator;
struct FCellData;

// Risultato di una cella raggiungibile
struct FCellaRaggiungibile
{
    int32 X    = 0;
    int32 Y    = 0;
    int32 Costo= 0;  // costo totale per arrivare qui
};

class MYPROJECT_WACHOU_API FPathfinder
{
public:
    // ── Movimento ────────────────────────────────────────────
    // Restituisce tutte le celle raggiungibili entro il budget
    // di movimento dell'unita'. Esclude acqua, torri, unita'.
    static TArray<FCellaRaggiungibile> CalcolaRaggiungibili(
        const FUnita& Unita,
        const AGridMapGenerator* Mappa,
        const TArray<FUnita>& TutteUnita);

    // Restituisce il percorso minimo da (StartX,StartY) a (EndX,EndY).
    // Array di coordinate in ordine dalla partenza alla destinazione.
    // Array vuoto = percorso non trovato.
    static TArray<TPair<int32,int32>> TrovaPercorso(
        int32 StartX, int32 StartY,
        int32 EndX,   int32 EndY,
        const FUnita& Unita,
        const AGridMapGenerator* Mappa,
        const TArray<FUnita>& TutteUnita);

    // ── Attacco ───────────────────────────────────────────────
    // Restituisce le celle attaccabili dall'unita' nella sua
    // posizione corrente. Regole design doc:
    //   - Solo livello uguale o inferiore all'attaccante
    //   - Distanza = costo movimento senza costo salita doppio
    //   - Ranged (Sniper): puo' attraversare acqua
    //   - Melee (Brawler): non puo' attraversare acqua
    static TArray<TPair<int32,int32>> CalcolaRangeAttacco(
        const FUnita& Attaccante,
        const AGridMapGenerator* Mappa,
        const TArray<FUnita>& TutteUnita);

    // Verifica se una cella e' nella lista raggiungibili
    static bool IsCellaRaggiungibile(
        int32 X, int32 Y,
        const TArray<FCellaRaggiungibile>& Raggiungibili);

    // Costo movimento tra due celle adiacenti (ortogonali)
    // -1 = non percorribile
    static int32 CostoMovimento(
        int32 FromX, int32 FromY,
        int32 ToX,   int32 ToY,
        const FUnita& Unita,
        const AGridMapGenerator* Mappa,
        const TArray<FUnita>& TutteUnita);

private:
    // Indice 1D da coordinate
    static int32 Idx(int32 X, int32 Y, int32 N) { return Y * N + X; }

    // Direzioni ortogonali (no obliquo)
    static const int32 DX[4];
    static const int32 DY[4];
};
