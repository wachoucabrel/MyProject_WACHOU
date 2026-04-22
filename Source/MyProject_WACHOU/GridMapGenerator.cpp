// ============================================================
// GridMapGenerator.cpp  -  MyProject_WACHOU
// ============================================================
// Implementazione della generazione mappa.
//
// FUNZIONI PRINCIPALI:
//   GeneraMappa()                → entry point, rigenera tutto
//   GeneraElevazione()           → applica Perlin, quantizza
//   ControllaConnettivitaMappa() → BFS, no isole irraggiungibili
//   PosizionaTorri()             → simmetrico adattivo
//   CreaMeshCelle()              → istanze + colori PerInstance
//   ImpostaCamera()              → camera ortogonale dall'alto
//   SetColoreRange()             → colora celle range visivo
//   AggiornaCellaTorre()         → cambia colore torre stato
// ============================================================
#include "GridMapGenerator.h"
#include "CellData.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "TimerManager.h"

AGridMapGenerator::AGridMapGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshCelle = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MeshCelle"));
    RootComponent = MeshCelle;
    MeshCelle->NumCustomDataFloats = 3;

    CameraMappa = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraMappa"));
    CameraMappa->SetupAttachment(RootComponent);
}

void AGridMapGenerator::BeginPlay()
{
    Super::BeginPlay();

    if (MeshCubo)
        MeshCelle->SetStaticMesh(MeshCubo);

    if (MaterialeCelle)
        MeshCelle->SetMaterial(0, MaterialeCelle);

    GeneraMappa();
    ImpostaCamera();

    FTimerHandle T;
    GetWorldTimerManager().SetTimer(T, [this]()
    {
        if (APlayerController* PC =
            UGameplayStatics::GetPlayerController(GetWorld(), 0))
        {
            PC->SetViewTarget(this);
        }
    }, 0.1f, false);
}

// ============================================================
//  GeneraMappa
// ============================================================
void AGridMapGenerator::GeneraMappa()
{
    MeshCelle->ClearInstances();
    GrigliaDati.Empty();
    CelleRange.Empty();

    GeneraElevazione();
    ControllaConnettivitaMappa();  // Assicura che tutte le celle siano raggiungibili
    PosizionaTorri();
    CreaMeshCelle();

    UE_LOG(LogTemp, Log,
        TEXT("[WACHOU] Mappa generata: %dx%d seed=%d"),
        DimensioneGriglia, DimensioneGriglia, SemeCasuale);
}

// ============================================================
//  GeneraElevazione
// ============================================================
void AGridMapGenerator::GeneraElevazione()
{
    const int32 N = DimensioneGriglia;
    GrigliaDati.Reserve(N * N);

    TArray<float> Noise;
    Noise.Reserve(N * N);

    for (int32 Y = 0; Y < N; ++Y)
        for (int32 X = 0; X < N; ++X)
            Noise.Add(CalcolaPerlinNoise(X, Y));

    float MinN = FLT_MAX, MaxN = -FLT_MAX;
    for (float v : Noise) { MinN = FMath::Min(MinN, v); MaxN = FMath::Max(MaxN, v); }
    const float Range = FMath::Max(MaxN - MinN, 0.001f);

    for (int32 Y = 0; Y < N; ++Y)
    {
        for (int32 X = 0; X < N; ++X)
        {
            const float n = (Noise[Y * N + X] - MinN) / Range;

            int32 Elev;
            if      (n < SogliaAcqua) Elev = 0;
            else if (n < SogliaPiano) Elev = 1;
            else
            {
                const float t = (n - SogliaPiano) / (1.f - SogliaPiano);
                Elev = FMath::Clamp(2 + FMath::FloorToInt(t * 3.f), 2, MaxElevazione);
            }

            FCellData C(X, Y, Elev);
            const float Half = (N - 1) * DimensioneCella * 0.5f;
            C.PosizioneMondo = FVector(
                X * DimensioneCella - Half,
                Y * DimensioneCella - Half,
                Elev * DimensioneCella * 0.5f);
            GrigliaDati.Add(C);
        }
    }
}

// ============================================================
//  CalcolaPerlinNoise  — fBm
// ============================================================
float AGridMapGenerator::CalcolaPerlinNoise(int32 X, int32 Y)
{
    float Val = 0.f, Amp = 1.f, Freq = ScalaPerlin, Max = 0.f;
    const float Ox = SemeCasuale * 1000.f;
    const float Oy = SemeCasuale * 1000.f;

    for (int32 i = 0; i < Octaves; ++i)
    {
        Val += FMath::PerlinNoise2D(
            FVector2D((X + Ox) * Freq, (Y + Oy) * Freq)) * Amp;
        Max  += Amp;
        Amp  *= Persistence;
        Freq *= Lacunarity;
    }
    return (Val / Max + 1.f) * 0.5f;
}

// ============================================================
//  PosizionaTorri
// ============================================================
void AGridMapGenerator::PosizionaTorri()
{
    const int32 N = DimensioneGriglia;
    const TPair<int32,int32> Pos[3] = {
        { N/2,  N/2 },
        { 5,    N/2 },
        { N-6,  N/2 }
    };

    for (const auto& P : Pos)
    {
        for (int32 R = 0; R <= N; ++R)
        {
            bool Found = false;
            for (int32 DY = -R; DY <= R && !Found; ++DY)
            {
                for (int32 DX = -R; DX <= R && !Found; ++DX)
                {
                    if (FMath::Abs(DX) != R && FMath::Abs(DY) != R) continue;
                    const int32 NX = P.Key + DX;
                    const int32 NY = P.Value + DY;
                    if (!IsValidCoord(NX, NY)) continue;
                    FCellData* C = &GrigliaDati[Idx(NX, NY)];
                    if (C->IsCalpestabile())
                    {
                        C->bHaTorre = true;
                        Found = true;
                    }
                }
            }
            if (Found) break;
        }
    }
}

// ============================================================
//  CreaMeshCelle
// ============================================================
void AGridMapGenerator::CreaMeshCelle()
{
    if (!MeshCelle) return;

    const float S = DimensioneCella / 100.f;

    for (const FCellData& C : GrigliaDati)
    {
        FTransform T;
        T.SetLocation(C.PosizioneMondo);
        T.SetScale3D(FVector(S, S, S * 0.5f));

        const int32 Idx2 = MeshCelle->AddInstance(T, true);

        FLinearColor Col = C.bHaTorre
            ? FLinearColor(0.05f, 0.05f, 0.05f)
            : GetColoreDaElevazione(C.Elevazione);

        MeshCelle->SetCustomDataValue(Idx2, 0, Col.R);
        MeshCelle->SetCustomDataValue(Idx2, 1, Col.G);
        MeshCelle->SetCustomDataValue(Idx2, 2, Col.B);
    }

    MeshCelle->MarkRenderStateDirty();
}

// ============================================================
//  GetColoreDaElevazione
// ============================================================
FLinearColor AGridMapGenerator::GetColoreDaElevazione(int32 Elev) const
{
    switch (Elev)
    {
    case 0: return ColoreAcqua;
    case 1: return ColorePiano;
    case 2: return ColoreMtBassa;
    case 3: return ColoreMtMedia;
    case 4: return ColoreVetta;
    default: return FLinearColor::White;
    }
}

// ============================================================
//  ImpostaCamera
// ============================================================
void AGridMapGenerator::ImpostaCamera()
{
    if (!CameraMappa) return;
    const float Half = (DimensioneGriglia - 1) * DimensioneCella * 0.5f;
    const float AltezzaCamera = DimensioneGriglia * DimensioneCella * 1.1f;
    CameraMappa->SetRelativeLocation(FVector(0.f, 0.f, AltezzaCamera));
    CameraMappa->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
}

// ============================================================
//  GetCella / IsValidCoord
// ============================================================
const FCellData* AGridMapGenerator::GetCella(int32 X, int32 Y) const
{
    if (!IsValidCoord(X, Y)) return nullptr;
    const int32 I = Idx(X, Y);
    return GrigliaDati.IsValidIndex(I) ? &GrigliaDati[I] : nullptr;
}

bool AGridMapGenerator::IsValidCoord(int32 X, int32 Y) const
{
    return X >= 0 && X < DimensioneGriglia
        && Y >= 0 && Y < DimensioneGriglia;
}

// ============================================================
//  API COLORAZIONE RANGE (Step 3)
// ============================================================
void AGridMapGenerator::SetColoreRange(int32 X, int32 Y, FLinearColor Colore)
{
    if (!IsValidCoord(X, Y) || !MeshCelle) return;
    const int32 I = Idx(X, Y);
    CelleRange.Add(I, Colore);
    MeshCelle->SetCustomDataValue(I, 0, Colore.R);
    MeshCelle->SetCustomDataValue(I, 1, Colore.G);
    MeshCelle->SetCustomDataValue(I, 2, Colore.B);
    MeshCelle->MarkRenderStateDirty();
}

void AGridMapGenerator::ResetColoreCella(int32 X, int32 Y)
{
    if (!IsValidCoord(X, Y) || !MeshCelle) return;
    const int32 I = Idx(X, Y);
    CelleRange.Remove(I);

    const FCellData* C = GetCella(X, Y);
    if (!C) return;

    FLinearColor Col = C->bHaTorre
        ? FLinearColor(0.05f, 0.05f, 0.05f)
        : GetColoreDaElevazione(C->Elevazione);

    MeshCelle->SetCustomDataValue(I, 0, Col.R);
    MeshCelle->SetCustomDataValue(I, 1, Col.G);
    MeshCelle->SetCustomDataValue(I, 2, Col.B);
    MeshCelle->MarkRenderStateDirty();
}

void AGridMapGenerator::ResetTuttiColori()
{
    if (!MeshCelle) return;

    for (auto& Pair : CelleRange)
    {
        const int32 I = Pair.Key;
        const int32 X = I % DimensioneGriglia;
        const int32 Y = I / DimensioneGriglia;
        const FCellData* C = GetCella(X, Y);
        if (!C) continue;

        FLinearColor Col = C->bHaTorre
            ? FLinearColor(0.05f, 0.05f, 0.05f)
            : GetColoreDaElevazione(C->Elevazione);

        MeshCelle->SetCustomDataValue(I, 0, Col.R);
        MeshCelle->SetCustomDataValue(I, 1, Col.G);
        MeshCelle->SetCustomDataValue(I, 2, Col.B);
    }

    CelleRange.Empty();
    MeshCelle->MarkRenderStateDirty();
}

// ============================================================
//  AggiornaCellaTorre — colora la cella torre in base allo stato
// ============================================================
void AGridMapGenerator::AggiornaCellaTorre(int32 X, int32 Y, int32 Stato)
{
    if (!IsValidCoord(X, Y) || !MeshCelle) return;
    const int32 I = Idx(X, Y);

    FLinearColor Col;
    switch (Stato)
    {
    case 1:  Col = FLinearColor(0.f, 0.6f, 1.f);   break; // HUMAN - ciano
    case 2:  Col = FLinearColor(1.f, 0.2f, 0.2f);  break; // AI    - rosso
    case 3:  Col = FLinearColor(1.f, 0.8f, 0.f);   break; // CONTESA - giallo
    default: Col = FLinearColor(0.1f, 0.1f, 0.1f); break; // NEUTRALE - nero
    }

    MeshCelle->SetCustomDataValue(I, 0, Col.R);
    MeshCelle->SetCustomDataValue(I, 1, Col.G);
    MeshCelle->SetCustomDataValue(I, 2, Col.B);
    MeshCelle->MarkRenderStateDirty();
}


// ============================================================
//  ControllaConnettivitaMappa
// ============================================================
// Design doc: "tutte le celle devono essere comunque raggiungibili (no isole)"
// Usa BFS per identificare la componente connessa piu' grande di celle calpestabili.
// Se ci sono isole separate, le celle di acqua che le separano vengono convertite
// in terreno piano per garantire la connettivita'.
void AGridMapGenerator::ControllaConnettivitaMappa()
{
    const int32 N = DimensioneGriglia;
    if (GrigliaDati.Num() != N * N) return;

    TArray<int32> Componente;
    Componente.Init(-1, N * N);

    // Trova tutte le componenti connesse con BFS
    int32 IdComp = 0;
    TArray<int32> Dimensioni;
    for (int32 i = 0; i < N * N; ++i)
    {
        if (GrigliaDati[i].Elevazione == 0) continue;  // Salta acqua
        if (Componente[i] != -1) continue;             // Gia' visitata

        // BFS dalla cella i
        TArray<int32> Queue;
        Queue.Add(i);
        Componente[i] = IdComp;
        int32 Size = 0;

        int32 Head = 0;
        while (Head < Queue.Num())
        {
            const int32 Curr = Queue[Head++];
            ++Size;
            const int32 Cx = Curr % N;
            const int32 Cy = Curr / N;

            const int32 DX[] = {0, 0, -1, 1};
            const int32 DY[] = {-1, 1, 0, 0};
            for (int32 k = 0; k < 4; ++k)
            {
                const int32 Nx = Cx + DX[k];
                const int32 Ny = Cy + DY[k];
                if (Nx < 0 || Nx >= N || Ny < 0 || Ny >= N) continue;
                const int32 Idx = Ny * N + Nx;
                if (Componente[Idx] != -1) continue;
                if (GrigliaDati[Idx].Elevazione == 0) continue;
                Componente[Idx] = IdComp;
                Queue.Add(Idx);
            }
        }

        Dimensioni.Add(Size);
        ++IdComp;
    }

    if (Dimensioni.Num() <= 1) return;  // Gia' tutto connesso

    // Trova la componente piu' grande
    int32 MainId = 0;
    for (int32 c = 1; c < Dimensioni.Num(); ++c)
        if (Dimensioni[c] > Dimensioni[MainId]) MainId = c;

    // Converti in piano (elev=1) tutte le celle acqua adiacenti a componenti isolate
    // Questo "tappa" i buchi facendo da ponte
    int32 CelleConvertite = 0;
    for (int32 i = 0; i < N * N; ++i)
    {
        if (GrigliaDati[i].Elevazione != 0) continue;
        const int32 Cx = i % N;
        const int32 Cy = i / N;

        bool bAdjMain = false, bAdjOther = false;
        const int32 DX[] = {0, 0, -1, 1};
        const int32 DY[] = {-1, 1, 0, 0};
        for (int32 k = 0; k < 4; ++k)
        {
            const int32 Nx = Cx + DX[k];
            const int32 Ny = Cy + DY[k];
            if (Nx < 0 || Nx >= N || Ny < 0 || Ny >= N) continue;
            const int32 Idx = Ny * N + Nx;
            if (Componente[Idx] == MainId) bAdjMain = true;
            else if (Componente[Idx] >= 0) bAdjOther = true;
        }

        // Cella di acqua che separa main da isola → la trasformiamo in ponte
        if (bAdjMain && bAdjOther)
        {
            GrigliaDati[i].Elevazione = 1;
            ++CelleConvertite;
        }
    }

    if (CelleConvertite > 0)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[GridMap] Connettivita' mappa: convertite %d celle acqua in ponti"),
            CelleConvertite);
        // Ricorsione per verificare se ora e' tutto connesso
        ControllaConnettivitaMappa();
    }
}
