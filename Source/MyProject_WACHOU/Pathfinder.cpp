// ============================================================
// Pathfinder.cpp  -  MyProject_WACHOU
// ============================================================
// Implementazioni Dijkstra/BFS senza STL.
//
//   DIJKSTRA (CalcolaRaggiungibili / TrovaPercorso):
//     - Coda priorita' usando TArray con scan lineare del minimo
//     - Distanze inizializzate a INT_MAX
//     - Predecessori (Pred[]) per ricostruire percorso minimo
//     - Movimento: solo ortogonale (no obliquo) — design doc
//
//   BFS (CalcolaRangeAttacco):
//     - Coda FIFO via TArray + indice Head
//     - Sniper (Distanza): puo' passare sopra acqua nel range
//     - Brawler (CorpoACorpo): bloccato dall'acqua
//     - Filtra nemici per elevazione (solo <=)
//
//   COSTI (CostoMovimento):
//     - Stesso livello / piano = 1
//     - Salita verso piu' alto = 2
//     - Discesa verso piu' basso = 1
//     - Acqua/torri/unita' = -1 (non percorribile)
// ============================================================
#include "Pathfinder.h"
#include "GridMapGenerator.h"
#include "CellData.h"

// Direzioni ortogonali: destra, sinistra, giu, su
const int32 FPathfinder::DX[4] = { 1, -1,  0, 0 };
const int32 FPathfinder::DY[4] = { 0,  0,  1,-1 };

// ============================================================
//  CostoMovimento
// ============================================================
int32 FPathfinder::CostoMovimento(
    int32 FromX, int32 FromY,
    int32 ToX,   int32 ToY,
    const FUnita& Unita,
    const AGridMapGenerator* Mappa,
    const TArray<FUnita>& TutteUnita)
{
    if (!Mappa) return -1;
    if (!Mappa->IsValidCoord(ToX, ToY)) return -1;

    const FCellData* Dest = Mappa->GetCella(ToX, ToY);
    if (!Dest) return -1;

    if (Dest->IsAcqua())   return -1;
    if (Dest->HasTower())  return -1;

    for (const FUnita& U : TutteUnita)
        if (U.bViva && U.GridX == ToX && U.GridY == ToY) return -1;

    const FCellData* Orig = Mappa->GetCella(FromX, FromY);
    if (!Orig) return -1;

    const int32 DeltaElev = Dest->Elevazione - Orig->Elevazione;
    if (DeltaElev > 0) return 2;
    if (DeltaElev < 0) return 1;
    return 1;
}

// ============================================================
//  CalcolaRaggiungibili  -  Dijkstra con TArray
// ============================================================
TArray<FCellaRaggiungibile> FPathfinder::CalcolaRaggiungibili(
    const FUnita& Unita,
    const AGridMapGenerator* Mappa,
    const TArray<FUnita>& TutteUnita)
{
    TArray<FCellaRaggiungibile> Risultato;
    if (!Mappa) return Risultato;

    const int32 N = Mappa->DimensioneGriglia;

    TArray<int32> Dist;
    Dist.Init(INT_MAX, N * N);

    // Coda con TArray - elemento: (costo, X, Y)
    TArray<TTuple<int32,int32,int32>> Coda;

    Dist[Idx(Unita.GridX, Unita.GridY, N)] = 0;
    Coda.Add(MakeTuple(0, Unita.GridX, Unita.GridY));

    while (Coda.Num() > 0)
    {
        // Estrai elemento con costo minimo
        int32 MinI = 0;
        for (int32 i = 1; i < Coda.Num(); ++i)
            if (Coda[i].Get<0>() < Coda[MinI].Get<0>())
                MinI = i;

        const int32 Costo = Coda[MinI].Get<0>();
        const int32 X     = Coda[MinI].Get<1>();
        const int32 Y     = Coda[MinI].Get<2>();
        Coda.RemoveAt(MinI);

        if (Costo > Dist[Idx(X, Y, N)]) continue;
        if (Costo > Unita.Movimento)    continue;

        if (!(X == Unita.GridX && Y == Unita.GridY))
        {
            FCellaRaggiungibile CR;
            CR.X = X; CR.Y = Y; CR.Costo = Costo;
            Risultato.Add(CR);
        }

        for (int32 d = 0; d < 4; ++d)
        {
            const int32 NX = X + DX[d];
            const int32 NY = Y + DY[d];
            const int32 C  = CostoMovimento(X, Y, NX, NY, Unita, Mappa, TutteUnita);
            if (C < 0) continue;

            const int32 NC   = Costo + C;
            const int32 NIdx = Idx(NX, NY, N);
            if (NC <= Unita.Movimento && NC < Dist[NIdx])
            {
                Dist[NIdx] = NC;
                Coda.Add(MakeTuple(NC, NX, NY));
            }
        }
    }

    return Risultato;
}

// ============================================================
//  TrovaPercorso  -  Dijkstra con ricostruzione percorso
// ============================================================
TArray<TPair<int32,int32>> FPathfinder::TrovaPercorso(
    int32 StartX, int32 StartY,
    int32 EndX,   int32 EndY,
    const FUnita& Unita,
    const AGridMapGenerator* Mappa,
    const TArray<FUnita>& TutteUnita)
{
    TArray<TPair<int32,int32>> Percorso;
    if (!Mappa) return Percorso;

    const int32 N        = Mappa->DimensioneGriglia;
    const int32 StartIdx = Idx(StartX, StartY, N);

    TArray<int32> Dist; Dist.Init(INT_MAX, N * N);
    TArray<int32> Pred; Pred.Init(-1, N * N);
    TArray<TTuple<int32,int32,int32>> Coda;

    Dist[StartIdx] = 0;
    Coda.Add(MakeTuple(0, StartX, StartY));

    while (Coda.Num() > 0)
    {
        int32 MinI = 0;
        for (int32 i = 1; i < Coda.Num(); ++i)
            if (Coda[i].Get<0>() < Coda[MinI].Get<0>())
                MinI = i;

        const int32 Costo = Coda[MinI].Get<0>();
        const int32 X     = Coda[MinI].Get<1>();
        const int32 Y     = Coda[MinI].Get<2>();
        Coda.RemoveAt(MinI);

        if (Costo > Dist[Idx(X, Y, N)]) continue;
        if (X == EndX && Y == EndY)     break;

        for (int32 d = 0; d < 4; ++d)
        {
            const int32 NX   = X + DX[d];
            const int32 NY   = Y + DY[d];
            const int32 C    = CostoMovimento(X, Y, NX, NY, Unita, Mappa, TutteUnita);
            if (C < 0) continue;

            const int32 NC   = Costo + C;
            const int32 NIdx = Idx(NX, NY, N);
            if (NC < Dist[NIdx])
            {
                Dist[NIdx] = NC;
                Pred[NIdx] = Idx(X, Y, N);
                Coda.Add(MakeTuple(NC, NX, NY));
            }
        }
    }

    // Ricostruzione percorso
    int32 Cur = Idx(EndX, EndY, N);
    if (Dist[Cur] == INT_MAX) return Percorso;

    while (Cur != StartIdx)
    {
        Percorso.Insert(TPair<int32,int32>(Cur % N, Cur / N), 0);
        Cur = Pred[Cur];
        if (Cur < 0) break;
    }

    return Percorso;
}

// ============================================================
//  CalcolaRangeAttacco  -  BFS senza STL
// ============================================================
TArray<TPair<int32,int32>> FPathfinder::CalcolaRangeAttacco(
    const FUnita& Attaccante,
    const AGridMapGenerator* Mappa,
    const TArray<FUnita>& TutteUnita)
{
    TArray<TPair<int32,int32>> Risultato;
    if (!Mappa) return Risultato;

    const int32 N = Mappa->DimensioneGriglia;
    const FCellData* CellaAtt = Mappa->GetCella(Attaccante.GridX, Attaccante.GridY);
    if (!CellaAtt) return Risultato;

    const int32 ElevAtt = CellaAtt->Elevazione;

    TArray<int32> Dist;
    Dist.Init(INT_MAX, N * N);

    // BFS con TArray come coda FIFO (Head = indice di lettura)
    TArray<TPair<int32,int32>> Coda;
    int32 Head = 0;

    Dist[Idx(Attaccante.GridX, Attaccante.GridY, N)] = 0;
    Coda.Add(TPair<int32,int32>(Attaccante.GridX, Attaccante.GridY));

    while (Head < Coda.Num())
    {
        const int32 X = Coda[Head].Key;
        const int32 Y = Coda[Head].Value;
        Head++;

        const int32 CurDist = Dist[Idx(X, Y, N)];
        if (CurDist >= Attaccante.RangeAttacco) continue;

        for (int32 d = 0; d < 4; ++d)
        {
            const int32 NX   = X + DX[d];
            const int32 NY   = Y + DY[d];
            if (!Mappa->IsValidCoord(NX, NY)) continue;

            const int32 NIdx = Idx(NX, NY, N);
            if (Dist[NIdx] != INT_MAX) continue;

            const FCellData* C = Mappa->GetCella(NX, NY);
            if (!C) continue;
            if (C->HasTower()) continue;

            if (C->IsAcqua() &&
                Attaccante.TipoAttacco == ETipoAttacco::CorpoACorpo)
                continue;

            Dist[NIdx] = CurDist + 1;
            Coda.Add(TPair<int32,int32>(NX, NY));
        }
    }

    // Celle con nemici attaccabili
    for (const FUnita& U : TutteUnita)
    {
        if (!U.bViva || U.Owner == Attaccante.Owner) continue;
        if (!Mappa->IsValidCoord(U.GridX, U.GridY))  continue;

        const int32 DI = Dist[Idx(U.GridX, U.GridY, N)];
        if (DI == INT_MAX || DI > Attaccante.RangeAttacco) continue;

        const FCellData* CB = Mappa->GetCella(U.GridX, U.GridY);
        if (!CB || CB->Elevazione > ElevAtt) continue;

        Risultato.Add(TPair<int32,int32>(U.GridX, U.GridY));
    }

    return Risultato;
}

// ============================================================
bool FPathfinder::IsCellaRaggiungibile(
    int32 X, int32 Y,
    const TArray<FCellaRaggiungibile>& Raggiungibili)
{
    for (const auto& CR : Raggiungibili)
        if (CR.X == X && CR.Y == Y) return true;
    return false;
}
