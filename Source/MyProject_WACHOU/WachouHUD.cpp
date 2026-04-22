// ============================================================
// WachouHUD.cpp  -  MyProject_WACHOU
// ============================================================
// Implementazione HUD con disegno diretto su Canvas.
//
//   FUNZIONI:
//     DrawHUD()                → entry, dispatch in base a fase
//     DisegnaSchermataConfig() → pannello configurazione mappa
//     DisegnaPulsante()        → registra hit-area
//     ClickSuConfigMappa()     → switch sui pulsanti tipo 0-8:
//                                0/1 Seed   ±1000
//                                2/3 Perlin ±0.01
//                                4/5 Acqua  ±0.02
//                                6/7 Piano  ±0.02
//                                8   INIZIA PARTITA
//     ClickSuPulsanteTerminaTurno() → hit-test box
//     DisegnaTesto/Rettangolo() → wrapper FCanvasItem
//
//   PANNELLO TORRI:
//     Mostra anche conteggio "Tue torri: X/3" e "Torri AI: X/3"
//     come richiesto dal design doc
// ============================================================
#include "WachouHUD.h"
#include "GameManager.h"
#include "GridMapGenerator.h"
#include "UnitData.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"

AWachouHUD::AWachouHUD() {}

void AWachouHUD::BeginPlay()
{
    Super::BeginPlay();
    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(), AGameManager::StaticClass(), Found);
    if (Found.Num() > 0)
        GameManager = Cast<AGameManager>(Found[0]);

    TArray<AActor*> Found2;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(), AGridMapGenerator::StaticClass(), Found2);
    if (Found2.Num() > 0)
        GridMappa = Cast<AGridMapGenerator>(Found2[0]);
}

void AWachouHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;

    const float SW = Canvas->SizeX;
    const float SH = Canvas->SizeY;

    // ── SCHERMATA CONFIGURAZIONE ─────────────────────────────
    if (GameManager && GameManager->GetFase() == EFaseGioco::Configurazione)
    {
        DisegnaSchermataConfig();
        return;
    }

    if (!GameManager) return;

    const EFaseGioco Fase   = GameManager->GetFase();
    const EOwner     Player = GameManager->GetPlayerCorrente();
    const int32      Turno  = GameManager->GetTurnoCorrente();
    const FString    Status = GameManager->GetStatus();

    // ── PANNELLO TITOLO ──────────────────────────────────────
    DisegnaRettangoloSfondo(10, 10, 520, 70,
        FLinearColor(0.f, 0.f, 0.f, 0.70f));

    FString TitoloTurno;
    FLinearColor ColTurno = FLinearColor::White;
    switch (Fase)
    {
    case EFaseGioco::LancioMoneta:
        TitoloTurno = TEXT("LANCIO MONETA");
        ColTurno = FLinearColor(1.f, 0.8f, 0.f); break;
    case EFaseGioco::Piazzamento:
        TitoloTurno = FString::Printf(TEXT("PIAZZAMENTO - %s"),
            (Player == EOwner::Human) ? TEXT("TUO TURNO") : TEXT("AI"));
        ColTurno = (Player == EOwner::Human)
            ? FLinearColor(0.f, 0.8f, 1.f) : FLinearColor(1.f, 0.3f, 0.3f);
        break;
    case EFaseGioco::GiocoInCorso:
        TitoloTurno = FString::Printf(TEXT("TURNO %d  -  %s"), Turno,
            (Player == EOwner::Human) ? TEXT("TUO TURNO") : TEXT("TURNO AI"));
        ColTurno = (Player == EOwner::Human)
            ? FLinearColor(0.f, 0.8f, 1.f) : FLinearColor(1.f, 0.3f, 0.3f);
        break;
    case EFaseGioco::FinePartita:
        TitoloTurno = TEXT("*** PARTITA FINITA ***");
        ColTurno = FLinearColor(1.f, 0.8f, 0.f); break;
    default: TitoloTurno = TEXT("WACHOU"); break;
    }
    DisegnaTesto(TitoloTurno, 20, 18, ColTurno, 1.4f);
    FString StatusBreve = Status;
    if (StatusBreve.Len() > 75) StatusBreve = StatusBreve.Left(72) + TEXT("...");
    DisegnaTesto(StatusBreve, 20, 50, FLinearColor(0.9f, 0.9f, 0.9f), 0.85f);

    // ── PANNELLO UNITA' ──────────────────────────────────────
    const TArray<FUnita>& Unita = GameManager->GetTutteUnita();
    int32 NumH = 0, NumA = 0;
    for (const FUnita& U : Unita)
        if (U.bViva) { if (U.Owner == EOwner::Human) NumH++; else NumA++; }

    const float PH = 30.f + (FMath::Max(NumH, NumA) + 1) * 22.f;
    DisegnaRettangoloSfondo(10, 90, 520, PH,
        FLinearColor(0.f, 0.f, 0.f, 0.65f));
    DisegnaTesto(TEXT("LE TUE UNITA'"), 20, 96, FLinearColor(0.f, 0.8f, 1.f), 0.95f);
    DisegnaTesto(TEXT("NEMICI AI"),    280, 96, FLinearColor(1.f, 0.3f, 0.3f), 0.95f);

    float YH = 118, YA = 118;
    for (const FUnita& U : Unita)
    {
        if (!U.bViva) continue;
        const FString Tipo  = (U.Tipo == ETipoUnita::Sniper) ? TEXT("Sniper") : TEXT("Brawler");
        const FString Coord = CellaToString(U.GridX, U.GridY);
        if (U.Owner == EOwner::Human)
        {
            const FString Az = U.bHaMosso && U.bHaAttaccato ? TEXT("[FINE]") :
                               U.bHaMosso ? TEXT("[att?]") :
                               U.bHaAttaccato ? TEXT("[mov?]") : TEXT("[OK]");
            DisegnaTesto(FString::Printf(TEXT("%s %s HP:%d/%d %s"),
                *Tipo, *Coord, U.HP, U.HPMax, *Az),
                20, YH, FLinearColor(0.f, 0.9f, 1.f), 0.85f);
            YH += 22;
        }
        else
        {
            DisegnaTesto(FString::Printf(TEXT("%s %s HP:%d/%d"),
                *Tipo, *Coord, U.HP, U.HPMax),
                280, YA, FLinearColor(1.f, 0.5f, 0.5f), 0.85f);
            YA += 22;
        }
    }

    // ── TORRI ────────────────────────────────────────────────
    const int32 NT = GameManager->GetNumTorri();
    DisegnaRettangoloSfondo(SW - 230, 10, 220, 28.f + NT * 22.f,
        FLinearColor(0.f, 0.f, 0.f, 0.65f));
    DisegnaTesto(TEXT("TORRI"), SW - 220, 15, FLinearColor(1.f, 0.8f, 0.f), 1.0f);
    float YT = 38;

    // Design doc: "visualizzare il numero delle torri conquistate"
    int32 TorriH = 0, TorriAI = 0;
    for (int32 i = 0; i < NT; ++i)
    {
        const FString Info = GameManager->GetTorreInfo(i);
        FLinearColor C = FLinearColor(0.7f, 0.7f, 0.7f);
        if (Info.Contains(TEXT("HUMAN")))    { C = FLinearColor(0.f, 0.8f, 1.f); ++TorriH; }
        else if (Info.Contains(TEXT("AI")))  { C = FLinearColor(1.f, 0.3f, 0.3f); ++TorriAI; }
        else if (Info.Contains(TEXT("CONTESA"))) C = FLinearColor(1.f, 0.8f, 0.f);
        DisegnaTesto(Info, SW - 220, YT, C, 0.85f);
        YT += 22;
    }

    // Conteggio torri e turni consecutivi (condizione vittoria)
    YT += 8;
    DisegnaTesto(FString::Printf(TEXT("Tue torri: %d/3"), TorriH),
        SW - 220, YT, FLinearColor(0.f, 0.8f, 1.f), 0.85f);
    YT += 20;
    DisegnaTesto(FString::Printf(TEXT("Torri AI:  %d/3"), TorriAI),
        SW - 220, YT, FLinearColor(1.f, 0.3f, 0.3f), 0.85f);
    YT += 24;
    DisegnaTesto(TEXT("(2 torri x 2 turni = vittoria)"),
        SW - 220, YT, FLinearColor(0.6f, 0.6f, 0.6f), 0.75f);

    // ── ISTRUZIONI + PULSANTE TERMINA TURNO ──────────────────
    bPulsanteVisibile = false;

    if (Fase == EFaseGioco::Piazzamento && Player == EOwner::Human)
    {
        DisegnaRettangoloSfondo(10, SH - 80, 500, 65,
            FLinearColor(0.f, 0.f, 0.f, 0.65f));
        DisegnaTesto(TEXT("CLICCA una cella nelle righe 0-2"),
            20, SH - 70, FLinearColor(0.f, 1.f, 0.5f), 1.0f);
        DisegnaTesto(TEXT("(parte BASSA della mappa)"),
            20, SH - 45, FLinearColor(0.7f, 0.7f, 0.7f), 0.85f);
    }
    else if (Fase == EFaseGioco::GiocoInCorso && Player == EOwner::Human)
    {
        DisegnaRettangoloSfondo(10, SH - 80, 560, 65,
            FLinearColor(0.f, 0.f, 0.f, 0.65f));
        DisegnaTesto(TEXT("CLICCA su una tua unita' per selezionarla"),
            20, SH - 70, FLinearColor(0.f, 0.8f, 1.f), 1.0f);
        DisegnaTesto(TEXT("Poi: cella BLU per muovere  |  cella ROSSA per attaccare"),
            20, SH - 45, FLinearColor(0.7f, 0.7f, 0.7f), 0.85f);

        const float PW = 230.f, PhUD = 44.f;
        const float PX = SW * 0.5f - PW * 0.5f;
        const float PY = SH - 60.f;
        DisegnaRettangoloSfondo(PX, PY, PW, PhUD,
            FLinearColor(0.f, 0.5f, 0.f, 0.85f));
        DisegnaRettangoloSfondo(PX+2, PY+2, PW-4, PhUD-4,
            FLinearColor(0.f, 0.7f, 0.1f, 0.6f));
        DisegnaTesto(TEXT("[ TERMINA TURNO ]"), PX+18, PY+10,
            FLinearColor::White, 1.1f);
        PulsantePos = FVector2D(PX, PY);
        PulsanteDim = FVector2D(PW, PhUD);
        bPulsanteVisibile = true;
    }
    else if (Fase == EFaseGioco::GiocoInCorso && Player == EOwner::AI)
    {
        DisegnaRettangoloSfondo(10, SH - 55, 220, 42,
            FLinearColor(0.f, 0.f, 0.f, 0.65f));
        DisegnaTesto(TEXT("L'AI sta pensando..."),
            20, SH - 48, FLinearColor(1.f, 0.5f, 0.5f), 1.0f);
    }
    else if (Fase == EFaseGioco::FinePartita)
    {
        const FString Vincitore = GameManager->GetVincitore();
        DisegnaRettangoloSfondo(SW*0.3f, SH*0.35f, SW*0.4f, SH*0.3f,
            FLinearColor(0.f, 0.f, 0.f, 0.85f));
        DisegnaTesto(TEXT("PARTITA FINITA!"),
            SW*0.35f, SH*0.4f, FLinearColor(1.f, 0.8f, 0.f), 1.8f);
        DisegnaTesto(Vincitore,
            SW*0.35f, SH*0.5f, FLinearColor(0.f, 1.f, 0.5f), 1.3f);
        DisegnaTesto(TEXT("Premi ESC per uscire"),
            SW*0.37f, SH*0.6f, FLinearColor(0.7f, 0.7f, 0.7f), 0.9f);
    }

    // ── RANGE ────────────────────────────────────────────────
    const auto& RM = GameManager->GetRangeMovimento();
    const auto& RA = GameManager->GetRangeAttacco();
    if (RM.Num() > 0 || RA.Num() > 0)
    {
        DisegnaRettangoloSfondo(SW-290, SH-80, 280, 65,
            FLinearColor(0.f, 0.f, 0.f, 0.65f));
        if (RM.Num() > 0)
            DisegnaTesto(FString::Printf(TEXT("Celle raggiungibili: %d"), RM.Num()),
                SW-280, SH-70, FLinearColor(0.3f, 0.8f, 1.f), 0.9f);
        if (RA.Num() > 0)
            DisegnaTesto(FString::Printf(TEXT("Nemici attaccabili: %d"), RA.Num()),
                SW-280, SH-47, FLinearColor(1.f, 0.4f, 0.4f), 0.9f);
    }
}

// ============================================================
//  SCHERMATA CONFIGURAZIONE
// ============================================================
void AWachouHUD::DisegnaSchermataConfig()
{
    if (!Canvas || !GridMappa) return;

    const float SW = Canvas->SizeX;
    const float SH = Canvas->SizeY;

    PulsantiConfig.Empty();

    // Sfondo scuro
    DisegnaRettangoloSfondo(0, 0, SW, SH, FLinearColor(0.f, 0.f, 0.f, 0.7f));

    // Pannello centrale
    const float PW = 560.f, PH2 = 480.f;
    const float PX = SW * 0.5f - PW * 0.5f;
    const float PY = SH * 0.5f - PH2 * 0.5f;
    DisegnaRettangoloSfondo(PX, PY, PW, PH2,
        FLinearColor(0.05f, 0.05f, 0.15f, 0.95f));
    DisegnaRettangoloSfondo(PX+2, PY+2, PW-4, 4,
        FLinearColor(0.f, 0.6f, 1.f, 1.f));

    // Titolo
    DisegnaTesto(TEXT("CONFIGURAZIONE MAPPA"),
        PX + 60, PY + 20, FLinearColor(0.f, 0.8f, 1.f), 1.5f);

    float RY = PY + 80;
    const float LX = PX + 20;
    const float VX = PX + 260;
    const float BW2 = 40.f, BH = 30.f;

    // Helper lambda per riga parametro
    auto RigaParam = [&](const FString& Nome, float Valore,
                          int32 Decimali, int32 TipoUp, int32 TipoDn)
    {
        DisegnaTesto(Nome, LX, RY + 5, FLinearColor(0.8f, 0.8f, 0.8f), 1.0f);
        FString ValStr = Decimali == 0
            ? FString::Printf(TEXT("%.0f"), Valore)
            : Decimali == 2
                ? FString::Printf(TEXT("%.2f"), Valore)
                : FString::Printf(TEXT("%.3f"), Valore);
        DisegnaTesto(ValStr, VX, RY + 5, FLinearColor(1.f, 1.f, 0.f), 1.0f);

        // Pulsante +
        DisegnaPulsante(VX + 120, RY, BW2, BH, TEXT("+"),
            FLinearColor(0.f, 0.5f, 0.f, 0.9f), TipoUp);
        // Pulsante -
        DisegnaPulsante(VX + 170, RY, BW2, BH, TEXT("-"),
            FLinearColor(0.5f, 0.f, 0.f, 0.9f), TipoDn);

        RY += 55;
    };

    RigaParam(TEXT("Seed"),         (float)GridMappa->SemeCasuale, 0, 0, 1);
    RigaParam(TEXT("Scala Perlin"), GridMappa->ScalaPerlin, 3, 2, 3);
    RigaParam(TEXT("Soglia Acqua"), GridMappa->SogliaAcqua, 2, 4, 5);
    RigaParam(TEXT("Soglia Piano"), GridMappa->SogliaPiano, 2, 6, 7);

    // Pulsante INIZIA
    DisegnaPulsante(PX + PW*0.5f - 120, RY + 20, 240, 50,
        TEXT("[ INIZIA PARTITA ]"),
        FLinearColor(0.f, 0.6f, 0.f, 0.95f), 8);
}

void AWachouHUD::DisegnaPulsante(float X, float Y, float W, float H,
                                  const FString& Label,
                                  FLinearColor Colore, int32 Tipo)
{
    DisegnaRettangoloSfondo(X, Y, W, H, Colore);
    DisegnaTesto(Label, X + 6, Y + 6, FLinearColor::White, 0.9f);

    FPulsanteConfig P;
    P.Pos  = FVector2D(X, Y);
    P.Dim  = FVector2D(W, H);
    P.Tipo = Tipo;
    PulsantiConfig.Add(P);
}

bool AWachouHUD::ClickSuPulsanteTerminaTurno(float MX, float MY) const
{
    if (!bPulsanteVisibile) return false;
    return MX >= PulsantePos.X && MX <= PulsantePos.X + PulsanteDim.X
        && MY >= PulsantePos.Y && MY <= PulsantePos.Y + PulsanteDim.Y;
}

bool AWachouHUD::ClickSuConfigMappa(float MX, float MY)
{
    if (!GridMappa || !GameManager) return false;
    if (GameManager->GetFase() != EFaseGioco::Configurazione) return false;

    for (const FPulsanteConfig& P : PulsantiConfig)
    {
        if (MX >= P.Pos.X && MX <= P.Pos.X + P.Dim.X
         && MY >= P.Pos.Y && MY <= P.Pos.Y + P.Dim.Y)
        {
            switch (P.Tipo)
            {
            case 0: GridMappa->SemeCasuale =
                        FMath::Clamp(GridMappa->SemeCasuale + 1000, 1, 999999); break;
            case 1: GridMappa->SemeCasuale =
                        FMath::Clamp(GridMappa->SemeCasuale - 1000, 1, 999999); break;
            case 2: GridMappa->ScalaPerlin =
                        FMath::Clamp(GridMappa->ScalaPerlin + 0.01f, 0.05f, 0.3f); break;
            case 3: GridMappa->ScalaPerlin =
                        FMath::Clamp(GridMappa->ScalaPerlin - 0.01f, 0.05f, 0.3f); break;
            case 4: GridMappa->SogliaAcqua =
                        FMath::Clamp(GridMappa->SogliaAcqua + 0.02f, 0.2f, 0.5f); break;
            case 5: GridMappa->SogliaAcqua =
                        FMath::Clamp(GridMappa->SogliaAcqua - 0.02f, 0.2f, 0.5f); break;
            case 6: GridMappa->SogliaPiano =
                        FMath::Clamp(GridMappa->SogliaPiano + 0.02f, 0.4f, 0.7f); break;
            case 7: GridMappa->SogliaPiano =
                        FMath::Clamp(GridMappa->SogliaPiano - 0.02f, 0.4f, 0.7f); break;
            case 8:
                // INIZIA PARTITA
                GridMappa->GeneraMappa();
                GameManager->IniziaGioco();
                return true;
            }
            // Rigenera mappa con nuovi parametri
            if (P.Tipo < 8)
                GridMappa->GeneraMappa();
            return true;
        }
    }
    return false;
}

void AWachouHUD::DisegnaTesto(const FString& T, float X, float Y,
                               FLinearColor C, float S)
{
    if (!Canvas) return;
    FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(T),
                         GEngine->GetLargeFont(), C);
    Item.Scale = FVector2D(S, S);
    Item.EnableShadow(FLinearColor::Black);
    Canvas->DrawItem(Item);
}

void AWachouHUD::DisegnaRettangoloSfondo(float X, float Y,
                                          float W, float H, FLinearColor C)
{
    if (!Canvas) return;
    FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(W, H), C);
    Tile.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(Tile);
}
