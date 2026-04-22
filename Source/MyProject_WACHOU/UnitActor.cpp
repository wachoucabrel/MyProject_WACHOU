// ============================================================
// UnitActor.cpp  -  MyProject_WACHOU
// ============================================================
// Implementazione dell'actor visivo unita'.
//
//   FUNZIONI:
//     Init()              → carica M_Unita, MID dinamico, colore
//     SetPosizioneGriglia() → traslazione XY+Z fissa
//     AnimaMovimento()    → genera lista FVector da percorso
//     Tick()              → muove la sfera di VelocitaAnim cm/s
//                           verso il prossimo punto del percorso
//     SetSelezionata()    → toggle anello + emissione luminosa
//     AggiornaHP()        → barra HP scalata (verde→arancio→rosso)
//
//   MATERIALE:
//     /Game/Materiali/M_Unita.M_Unita con parametro "ColoreUnita"
//     Shading Model = Unlit per colori puri
// ============================================================
#include "UnitActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

AUnitActor::AUnitActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Radice = CreateDefaultSubobject<USceneComponent>(TEXT("Radice"));
    RootComponent = Radice;

    CorpoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CorpoMesh"));
    CorpoMesh->SetupAttachment(RootComponent);
    CorpoMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CorpoMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    CorpoMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    // Sfera base Unreal
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
        CorpoMesh->SetStaticMesh(SphereMesh.Object);

    // Carica M_Unita dal progetto
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatCell(
        TEXT("/Game/Materiali/M_Unita.M_Unita"));
    if (MatCell.Succeeded())
        CorpoMesh->SetMaterial(0, MatCell.Object);

    // NumCustomDataFloats = 3 (R, G, B) come nelle celle della griglia

    BarraHPMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarraHPMesh"));
    BarraHPMesh->SetupAttachment(RootComponent);
    BarraHPMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
        BarraHPMesh->SetStaticMesh(CubeMesh.Object);

    AnelloSelezione = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnelloSelezione"));
    AnelloSelezione->SetupAttachment(RootComponent);
    AnelloSelezione->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AnelloSelezione->SetVisibility(false);
    if (CubeMesh.Succeeded())
        AnelloSelezione->SetStaticMesh(CubeMesh.Object);
}

void AUnitActor::Init(const FUnita& Unita, float DC)
{
    UnitaID     = Unita.ID;
    ColoreUnita = Unita.GetColore();

    // DC=50: Scala=0.35 -> sfera 35cm di diametro, visibile sulla griglia
    const float Scala   = DC / 100.f * 0.7f;
    const float Altezza = DC * 0.8f;

    CorpoMesh->SetRelativeScale3D(FVector(Scala, Scala, Scala));
    CorpoMesh->SetRelativeLocation(FVector(0.f, 0.f, Altezza * 0.5f));

    // Crea materiale dinamico da M_Unita
    UMaterialInterface* MatBase = Cast<UMaterialInterface>(
        StaticLoadObject(UMaterialInterface::StaticClass(), nullptr,
            TEXT("/Game/Materiali/M_Unita.M_Unita")));
    if (!MatBase) MatBase = CorpoMesh->GetMaterial(0);
    if (MatBase)
    {
        MatCorpo = UMaterialInstanceDynamic::Create(MatBase, this);
        MatCorpo->SetVectorParameterValue(TEXT("ColoreUnita"), ColoreUnita);
        CorpoMesh->SetMaterial(0, MatCorpo);
    }

    // Barra HP
    const float BW = DC / 100.f * 0.6f;
    BarraHPMesh->SetRelativeScale3D(FVector(BW, BW * 0.15f, 0.06f));
    BarraHPMesh->SetRelativeLocation(FVector(0.f, 0.f, Altezza + DC * 0.15f));

    // Barra HP e anello usano lo stesso MatBase gia' caricato
    if (MatBase)
    {
        MatBarraHP = UMaterialInstanceDynamic::Create(MatBase, this);
        MatBarraHP->SetVectorParameterValue(TEXT("ColoreUnita"), FLinearColor::Green);
        BarraHPMesh->SetMaterial(0, MatBarraHP);
    }

    // Anello selezione giallo piatto
    const float RS = DC / 100.f * 0.9f;
    AnelloSelezione->SetRelativeScale3D(FVector(RS, RS, 0.015f));
    AnelloSelezione->SetRelativeLocation(FVector(0.f, 0.f, 1.f));
    AnelloSelezione->SetVisibility(false);
    if (MatBase)
    {
        UMaterialInstanceDynamic* MatAnello =
            UMaterialInstanceDynamic::Create(MatBase, this);
        MatAnello->SetVectorParameterValue(TEXT("ColoreUnita"),
            FLinearColor(1.f, 1.f, 0.f)); // giallo
        AnelloSelezione->SetMaterial(0, MatAnello);
    }
}

void AUnitActor::SetPosizioneGriglia(int32 GridX, int32 GridY,
                                      float DC, float HalfGrid)
{
    const float Z = FMath::Max(GetActorLocation().Z, DC * 0.3f);
    SetActorLocation(FVector(
        GridX * DC - HalfGrid,
        GridY * DC - HalfGrid,
        Z));
}

void AUnitActor::AnimaMovimento(
    const TArray<TPair<int32,int32>>& Percorso,
    float DC, float HalfGrid, float ZBase)
{
    if (Percorso.IsEmpty()) return;

    PuntiPercorso.Empty();
    for (const auto& P : Percorso)
    {
        PuntiPercorso.Add(FVector(
            P.Key   * DC - HalfGrid,
            P.Value * DC - HalfGrid,
            ZBase + DC * 0.3f));
    }

    PuntoCorrente = 0;
    bInMovimento  = true;
}

void AUnitActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bInMovimento || PuntiPercorso.IsEmpty()) return;

    const FVector Target  = PuntiPercorso[PuntoCorrente];
    const FVector Current = GetActorLocation();
    const FVector Dir     = Target - Current;
    const float   Dist    = Dir.Size2D();

    if (Dist < 2.f)
    {
        SetActorLocation(FVector(Target.X, Target.Y, Current.Z));
        PuntoCorrente++;
        if (PuntoCorrente >= PuntiPercorso.Num())
        {
            bInMovimento = false;
            PuntiPercorso.Empty();
        }
    }
    else
    {
        SetActorLocation(Current + Dir.GetSafeNormal2D() * VelocitaAnim * DeltaTime);
    }
}

void AUnitActor::SetSelezionata(bool bSel)
{
    AnelloSelezione->SetVisibility(bSel);

    // Aumenta luminosita' quando selezionata
    if (MatCorpo)
        MatCorpo->SetVectorParameterValue(TEXT("ColoreUnita"),
            bSel ? ColoreUnita * 4.f : ColoreUnita);
}

void AUnitActor::AggiornaHP(int32 HPC, int32 HPMax)
{
    if (HPMax <= 0) return;
    const float Pct = FMath::Clamp((float)HPC / (float)HPMax, 0.f, 1.f);

    const FVector S = BarraHPMesh->GetRelativeScale3D();
    BarraHPMesh->SetRelativeScale3D(FVector(
        FMath::Max(0.001f, S.X * Pct / FMath::Max(Pct, 0.01f)),
        S.Y, S.Z));

    // Aggiorna colore barra HP
    if (MatBarraHP)
        MatBarraHP->SetVectorParameterValue(TEXT("ColoreUnita"), GetColoreHP(Pct));
}

FLinearColor AUnitActor::GetColoreHP(float Pct)
{
    if (Pct > 0.5f)  return FLinearColor::Green;
    if (Pct > 0.25f) return FLinearColor(1.f, 0.6f, 0.f);
    return FLinearColor::Red;
}
