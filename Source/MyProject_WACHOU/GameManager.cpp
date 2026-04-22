// ============================================================
// GameManager.cpp  -  MyProject_WACHOU
// ============================================================
// Implementazione completa della logica di gioco.
//
//   FUNZIONI CHIAVE:
//
//   Setup:
//     IniziaGioco()        → chiamata dal pulsante INIZIA PARTITA
//     InizializzaTorri()   → trova le 3 torri sulla mappa
//     StartLancioMoneta()  → random 50/50, log esito
//     StartPiazzamento()   → costruisce sequenza alternata
//
//   Gioco:
//     OnCellaCliccata()    → routing in base alla fase
//     SelezionaUnita()     → calcola range, evidenzia celle
//     DeselezionaUnita()   → ripristina griglia (anche 2nd click)
//     EseguiMovimento()    → percorso minimo + animazione
//     EseguiAttacco()      → danno random + contrattacco Sniper
//     FineTurnoUnita()     → controlla se tutte azioni fatte
//     FineTurnoGiocatore() → passa al prossimo, aggiorna torri
//
//   AI:
//     TurnoAI()            → priorita' torri, poi nemici
//     MostraRangeAI()      → evidenzia mossa AI sulla griglia
//
//   Torri:
//     AggiornaTorri()      → macchina a stati distanza Chebyshev
//     ControllaTurnoFinePartita() → 2 torri x 2 turni
//
//   Storico mosse:
//     LogMossa()           → formato "HP: S B4 -> D6"
//     StampaStorico()      → output completo a fine turno
// ============================================================
#include "GameManager.h"
#include "GridMapGenerator.h"
#include "UnitActor.h"
#include "CellData.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"

AGameManager::AGameManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGameManager::BeginPlay()
{
    Super::BeginPlay();
    // Inizia in fase Configurazione — aspetta che il giocatore prema INIZIA PARTITA
    FaseCorrente = EFaseGioco::Configurazione;
    SetStatus(TEXT("Configura i parametri e premi INIZIA PARTITA"));
}

void AGameManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Il Tick e' usato dagli UnitActor per l'animazione movimento
}

// ============================================================
//  INIZIO GIOCO
// ============================================================
void AGameManager::IniziaGioco()
{
    if (!MappaRiferimento)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[GameManager] MappaRiferimento non assegnato!"));
        return;
    }

    Unita.Empty();
    UnitaActors.Empty();
    StoricoMosse.Empty();
    NextUnitaID = 0;
    TurnoCorrente = 1;
    TurniConsecutiviHuman = 0;
    TurniConsecutiviAI = 0;

    InizializzaTorri();
    StartLancioMoneta();
}

// ============================================================
//  TORRI
// ============================================================
void AGameManager::InizializzaTorri()
{
    Torri.Empty();

    // Cerca le celle con torre nella griglia
    const int32 N = MappaRiferimento->DimensioneGriglia;
    for (int32 Y = 0; Y < N; ++Y)
    {
        for (int32 X = 0; X < N; ++X)
        {
            const FCellData* C = MappaRiferimento->GetCella(X, Y);
            if (C && C->HasTower())
            {
                FTorre T;
                T.GridX = X; T.GridY = Y;
                T.Stato = EStatoTorre::Neutrale;
                Torri.Add(T);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[GameManager] %d torri trovate."), Torri.Num());
}

void AGameManager::AggiornaTorri()
{
    for (FTorre& Torre : Torri)
    {
        // Conta unita' Human e AI nella zona di cattura (Chebyshev <= 2)
        int32 CountH = 0, CountAI = 0;

        for (const FUnita& U : Unita)
        {
            if (!U.bViva) continue;
            if (DistanzaChebyshev(U.GridX, U.GridY, Torre.GridX, Torre.GridY) <= 2)
            {
                if (U.Owner == EOwner::Human) CountH++;
                if (U.Owner == EOwner::AI)    CountAI++;
            }
        }

        // Aggiorna stato torre
        if (CountH > 0 && CountAI > 0)
            Torre.Stato = EStatoTorre::Contesa;
        else if (CountH > 0)
            Torre.Stato = EStatoTorre::Human;
        else if (CountAI > 0)
            Torre.Stato = EStatoTorre::AI;
        // Se nessuno: mantiene lo stato precedente (regola design doc)

        // Aggiorna colore visivo torre sulla mappa
        if (MappaRiferimento)
        {
            int32 StatoInt = 0;
            if      (Torre.Stato == EStatoTorre::Human)   StatoInt = 1;
            else if (Torre.Stato == EStatoTorre::AI)      StatoInt = 2;
            else if (Torre.Stato == EStatoTorre::Contesa) StatoInt = 3;
            MappaRiferimento->AggiornaCellaTorre(Torre.GridX, Torre.GridY, StatoInt);
        }
    }

    // Log stato torri
    for (int32 i = 0; i < Torri.Num(); ++i)
    {
        const FString Stato =
            Torri[i].Stato == EStatoTorre::Human   ? TEXT("HUMAN")   :
            Torri[i].Stato == EStatoTorre::AI       ? TEXT("AI")      :
            Torri[i].Stato == EStatoTorre::Contesa  ? TEXT("CONTESA") :
                                                       TEXT("NEUTRALE");
        UE_LOG(LogTemp, Log,
            TEXT("[Torri] Torre %d (%s): %s"),
            i, *CellaToString(Torri[i].GridX, Torri[i].GridY), *Stato);
    }
}

bool AGameManager::ControllaTorreProssima(int32 X, int32 Y,
                                           int32& OutIdx) const
{
    for (int32 i = 0; i < Torri.Num(); ++i)
    {
        if (DistanzaChebyshev(X, Y, Torri[i].GridX, Torri[i].GridY) <= 2)
        {
            OutIdx = i;
            return true;
        }
    }
    return false;
}

EOwner AGameManager::ControllaPossessoTorri() const
{
    int32 TorriH = 0, TorriAI = 0;
    for (const FTorre& T : Torri)
    {
        if (T.Stato == EStatoTorre::Human) TorriH++;
        if (T.Stato == EStatoTorre::AI)    TorriAI++;
    }
    if (TorriH >= 2) return EOwner::Human;
    if (TorriAI >= 2) return EOwner::AI;
    return EOwner::Nessuno;
}

// ============================================================
//  LANCIO MONETA
// ============================================================
void AGameManager::StartLancioMoneta()
{
    FaseCorrente = EFaseGioco::LancioMoneta;
    const EOwner Vincitore = (FMath::RandRange(0, 1) == 0)
        ? EOwner::Human : EOwner::AI;

    const FString Chi = (Vincitore == EOwner::Human) ? TEXT("TU") : TEXT("AI");
    SetStatus(FString::Printf(
        TEXT("*** LANCIO MONETA: %s inizia! ***"), *Chi));

    StartPiazzamento(Vincitore);
}

// ============================================================
//  PIAZZAMENTO
// ============================================================
void AGameManager::StartPiazzamento(EOwner Primo)
{
    FaseCorrente = EFaseGioco::Piazzamento;

    // Crea le 4 unita'
    Unita.Add(FUnita::Crea(NextUnitaID++, EOwner::Human, ETipoUnita::Sniper));
    Unita.Add(FUnita::Crea(NextUnitaID++, EOwner::Human, ETipoUnita::Brawler));
    Unita.Add(FUnita::Crea(NextUnitaID++, EOwner::AI,    ETipoUnita::Sniper));
    Unita.Add(FUnita::Crea(NextUnitaID++, EOwner::AI,    ETipoUnita::Brawler));

    const EOwner Secondo = (Primo == EOwner::Human)
        ? EOwner::AI : EOwner::Human;

    SequenzaPiazzamento.Empty();
    SequenzaPiazzamento.Add({ Primo,   ETipoUnita::Sniper  });
    SequenzaPiazzamento.Add({ Secondo, ETipoUnita::Sniper  });
    SequenzaPiazzamento.Add({ Primo,   ETipoUnita::Brawler });
    SequenzaPiazzamento.Add({ Secondo, ETipoUnita::Brawler });

    StepPiazzamento = 0;
    AvanzaPiazzamento();
}

void AGameManager::AvanzaPiazzamento()
{
    if (StepPiazzamento >= SequenzaPiazzamento.Num())
    {
        StartGioco();
        return;
    }

    const auto& Step = SequenzaPiazzamento[StepPiazzamento];
    const FString Tipo = (Step.Tipo == ETipoUnita::Sniper)
        ? TEXT("Sniper") : TEXT("Brawler");

    if (Step.Owner == EOwner::AI)
    {
        SetStatus(FString::Printf(TEXT("AI piazza %s..."), *Tipo));
        PiazzaUnitaAI();
    }
    else
    {
        SetStatus(FString::Printf(
            TEXT("Piazza il tuo %s nelle righe 0-2. Clicca una cella."),
            *Tipo));
    }
}

void AGameManager::PiazzaUnitaAI()
{
    const auto& Step = SequenzaPiazzamento[StepPiazzamento];
    const int32 N = MappaRiferimento->DimensioneGriglia;
    const int32 CX = N / 2;

    FUnita* UnitaTarget = nullptr;
    for (FUnita& U : Unita)
        if (U.Owner == EOwner::AI && U.Tipo == Step.Tipo && !U.IsPiazzata())
        { UnitaTarget = &U; break; }
    if (!UnitaTarget) return;

    int32 BestX = -1, BestY = -1, BestScore = INT_MIN;

    for (int32 Y = 22; Y <= 24; ++Y)
    {
        for (int32 X = 0; X < N; ++X)
        {
            const FCellData* C = MappaRiferimento->GetCella(X, Y);
            if (!C || !C->IsCalpestabile()) continue;
            if (UnitaSuCella(X, Y)) continue;

            int32 Score = 0;
            const int32 DC = FMath::Abs(X - CX);
            Score += (Step.Tipo == ETipoUnita::Sniper) ? DC : -DC;
            Score += (24 - Y) * 2;

            if (Score > BestScore) { BestScore = Score; BestX = X; BestY = Y; }
        }
    }

    if (BestX >= 0)
    {
        PiazzaUnita(UnitaTarget->ID, BestX, BestY);
        StepPiazzamento++;
        AvanzaPiazzamento();
    }
}

// ============================================================
//  CLICK CELLA
// ============================================================
void AGameManager::OnCellaCliccata(int32 GridX, int32 GridY)
{
    // ── Piazzamento ───────────────────────────────────────────
    if (FaseCorrente == EFaseGioco::Piazzamento)
    {
        const auto& Step = SequenzaPiazzamento[StepPiazzamento];
        if (Step.Owner != EOwner::Human) return;

        if (!IsZonaHuman(GridY))
        { SetStatus(TEXT("Puoi piazzare solo nelle righe 0, 1 o 2!")); return; }

        const FCellData* C = MappaRiferimento->GetCella(GridX, GridY);
        if (!C || !C->IsCalpestabile())
        { SetStatus(TEXT("Cella non valida!")); return; }

        if (UnitaSuCella(GridX, GridY))
        { SetStatus(TEXT("Cella occupata!")); return; }

        for (FUnita& U : Unita)
        {
            if (U.Owner == EOwner::Human &&
                U.Tipo  == Step.Tipo     && !U.IsPiazzata())
            {
                PiazzaUnita(U.ID, GridX, GridY);
                StepPiazzamento++;
                AvanzaPiazzamento();
                return;
            }
        }
        return;
    }

    // ── Gioco in corso ────────────────────────────────────────
    if (FaseCorrente != EFaseGioco::GiocoInCorso) return;
    if (PlayerCorrente != EOwner::Human) return;

    FUnita* Cliccata = UnitaSuCella(GridX, GridY);

    if (UnitaSelezionata >= 0)
    {
        FUnita* Sel = TrovaUnita(UnitaSelezionata);

        // Design doc: secondo click sulla stessa unita' → deseleziona e nasconde range
        if (Cliccata && Cliccata->ID == UnitaSelezionata)
        {
            DeselezionaUnita();
            SetStatus(TEXT("Unita' deselezionata."));
            return;
        }

        // Click su nemico attaccabile → attacca
        if (Cliccata && Cliccata->Owner == EOwner::AI)
        {
            if (!Sel->bHaAttaccato)
            {
                // Verifica che sia nel range attacco corrente
                bool bInRange = false;
                for (const auto& P : RangeAttaccoCorrente)
                    if (P.Key == GridX && P.Value == GridY)
                    { bInRange = true; break; }

                if (bInRange)
                    EseguiAttacco(GridX, GridY);
                else
                    SetStatus(TEXT("Nemico fuori range di attacco!"));
            }
            else
                SetStatus(TEXT("Hai gia' attaccato con questa unita'!"));
            return;
        }

        // Click su cella vuota raggiungibile → muovi
        if (!Cliccata)
        {
            if (!Sel->bHaMosso)
            {
                if (FPathfinder::IsCellaRaggiungibile(
                        GridX, GridY, RangeMovimentoCorrente))
                    EseguiMovimento(GridX, GridY);
                else
                    SetStatus(TEXT("Cella non raggiungibile!"));
            }
            else
                SetStatus(TEXT("Hai gia' mosso questa unita'!"));
            return;
        }

        // Click su altra unita' propria → cambia selezione
        if (Cliccata->Owner == EOwner::Human && !Cliccata->AzioniFinite())
        {
            SelezionaUnita(Cliccata->ID);
            return;
        }

        // Click su cella irrilevante → deseleziona
        DeselezionaUnita();
        return;
    }

    // Nessuna unita' selezionata: seleziona se propria
    if (Cliccata && Cliccata->Owner == EOwner::Human && !Cliccata->AzioniFinite())
        SelezionaUnita(Cliccata->ID);
}

// ============================================================
//  SELEZIONE
// ============================================================
void AGameManager::SelezionaUnita(int32 ID)
{
    if (UnitaSelezionata >= 0)
        if (AUnitActor* A = TrovaActor(UnitaSelezionata))
            A->SetSelezionata(false);

    UnitaSelezionata = ID;

    if (AUnitActor* A = TrovaActor(ID))
        A->SetSelezionata(true);

    AggiornaRangeVisivo(ID);

    const FUnita* U = TrovaUnita(ID);
    if (U)
        SetStatus(FString::Printf(
            TEXT("[%s] HP:%d/%d | %s | %s | Clicca per muovere o attaccare."),
            *U->GetNome(), U->HP, U->HPMax,
            U->bHaMosso     ? TEXT("Mosso")     : TEXT("Puo muovere"),
            U->bHaAttaccato ? TEXT("Attaccato") : TEXT("Puo attaccare")));
}

void AGameManager::DeselezionaUnita()
{
    if (UnitaSelezionata >= 0)
    {
        if (AUnitActor* A = TrovaActor(UnitaSelezionata))
            A->SetSelezionata(false);
        CancellaRangeVisivo();
    }
    UnitaSelezionata = -1;
    SetStatus(TEXT("Clicca una tua unita' per selezionarla."));
}

// ============================================================
//  MOVIMENTO
// ============================================================
void AGameManager::EseguiMovimento(int32 GridX, int32 GridY)
{
    FUnita* U = TrovaUnita(UnitaSelezionata);
    if (!U || U->bHaMosso) return;

    const int32 OldX = U->GridX, OldY = U->GridY;

    // Trova percorso minimo
    TArray<TPair<int32,int32>> Percorso = FPathfinder::TrovaPercorso(
        OldX, OldY, GridX, GridY, *U, MappaRiferimento, Unita);

    if (Percorso.IsEmpty())
    {
        SetStatus(TEXT("Percorso non trovato!"));
        return;
    }

    // Aggiorna posizione logica
    U->GridX = GridX;
    U->GridY = GridY;
    U->bHaMosso = true;

    // Storico mosse: HP: S B4 -> D6
    const FString OwnerStr = (U->Owner == EOwner::Human) ? TEXT("HP") : TEXT("AI");
    LogMossa(FString::Printf(TEXT("%s: %s %s -> %s"),
        *OwnerStr, *U->GetLettera(),
        *CellaToString(OldX, OldY),
        *CellaToString(GridX, GridY)));

    // Animazione movimento
    if (AUnitActor* A = TrovaActor(U->ID))
    {
        const float CS   = MappaRiferimento->DimensioneCella;
        const float Half = (MappaRiferimento->DimensioneGriglia - 1) * CS * 0.5f;
        const FCellData* C = MappaRiferimento->GetCella(GridX, GridY);
        const float ZBase = C ? C->PosizioneMondo.Z + CS * 0.5f + 5.f : 0.f;
        A->AnimaMovimento(Percorso, CS, Half, ZBase);
    }

    // Controlla conquista torre
    int32 TorreIdx = -1;
    if (ControllaTorreProssima(GridX, GridY, TorreIdx))
    {
        UE_LOG(LogTemp, Log,
            TEXT("[GameManager] %s vicino alla torre %d!"),
            *U->GetNome(), TorreIdx);
    }

    // Aggiorna range visivo dopo il movimento
    AggiornaRangeVisivo(U->ID);

    SetStatus(FString::Printf(
        TEXT("%s si muove in %s. Puoi ancora attaccare."),
        *U->GetNome(), *CellaToString(GridX, GridY)));

    FineTurnoUnita(U->ID);
}

// ============================================================
//  ATTACCO
// ============================================================
void AGameManager::EseguiAttacco(int32 GridX, int32 GridY)
{
    FUnita* Att = TrovaUnita(UnitaSelezionata);
    if (!Att || Att->bHaAttaccato) return;

    FUnita* Ber = UnitaSuCella(GridX, GridY);
    if (!Ber || Ber->Owner == EOwner::Human || !Ber->bViva) return;

    // Verifica elevazione: bersaglio deve essere <= elevazione attaccante
    const FCellData* CellAtt = MappaRiferimento->GetCella(Att->GridX, Att->GridY);
    const FCellData* CellBer = MappaRiferimento->GetCella(GridX, GridY);
    if (CellAtt && CellBer && CellBer->Elevazione > CellAtt->Elevazione)
    {
        SetStatus(TEXT("Non puoi attaccare unita' su elevazione superiore!"));
        return;
    }

    const int32 Danno = RollDanno(*Att);
    Ber->HP -= Danno;
    Att->bHaAttaccato = true;

    // Storico mosse attacco
    const FString OwnerStr = (Att->Owner == EOwner::Human) ? TEXT("HP") : TEXT("AI");
    LogMossa(FString::Printf(TEXT("%s: %s %s %d"),
        *OwnerStr, *Att->GetLettera(),
        *CellaToString(GridX, GridY), Danno));

    FString Msg = FString::Printf(
        TEXT("%s attacca %s per %d danni (HP: %d/%d)"),
        *Att->GetNome(), *Ber->GetNome(),
        Danno, FMath::Max(0, Ber->HP), Ber->HPMax);

    if (AUnitActor* A = TrovaActor(Ber->ID))
        A->AggiornaHP(Ber->HP, Ber->HPMax);

    // ── Contrattacco Sniper ───────────────────────────────────
    // Se l'attaccante e' Sniper e:
    //   - il bersaglio e' Sniper, OPPURE
    //   - il bersaglio e' Brawler a distanza 1
    if (Att->Tipo == ETipoUnita::Sniper)
    {
        const int32 Dist = FMath::Abs(Att->GridX - GridX)
                         + FMath::Abs(Att->GridY - GridY);

        bool bContrattacco =
            (Ber->Tipo == ETipoUnita::Sniper) ||
            (Ber->Tipo == ETipoUnita::Brawler && Dist <= 1);

        if (bContrattacco && Ber->bViva)
        {
            const int32 DannoC = FMath::RandRange(1, 3);
            Att->HP -= DannoC;
            Msg += FString::Printf(
                TEXT(" | CONTRATTACCO: %s subisce %d danni!"),
                *Att->GetNome(), DannoC);

            if (AUnitActor* AA = TrovaActor(Att->ID))
                AA->AggiornaHP(Att->HP, Att->HPMax);
        }
    }

    // ── Eliminazione e Respawn ────────────────────────────────
    if (Ber->HP <= 0)
    {
        Msg += TEXT(" *** ELIMINATO! Respawn... ***");
        Ber->HP    = 0;
        Ber->bViva = false;

        if (AUnitActor* A = TrovaActor(Ber->ID))
            A->Destroy();

        UnitaActors.RemoveAll([Ber](AUnitActor* A)
        { return A && A->GetUnitaID() == Ber->ID; });

        // Respawn nella posizione iniziale
        RespawnUnita(*Ber);
    }

    if (Att->HP <= 0)
    {
        Att->HP    = 0;
        Att->bViva = false;
        Msg += TEXT(" | Attaccante eliminato! Respawn...");

        if (AUnitActor* A = TrovaActor(Att->ID))
            A->Destroy();

        UnitaActors.RemoveAll([Att](AUnitActor* A)
        { return A && A->GetUnitaID() == Att->ID; });

        RespawnUnita(*Att);
    }

    SetStatus(Msg);
    CancellaRangeVisivo();
    FineTurnoUnita(Att->ID);
}

// ============================================================
//  RESPAWN
// ============================================================
void AGameManager::RespawnUnita(FUnita& U)
{
    U.HP    = U.HPMax;
    U.bViva = true;
    U.GridX = U.SpawnX;
    U.GridY = U.SpawnY;
    U.ResetTurno();

    // Crea nuovo Actor visivo
    AUnitActor* Actor = SpawnActor(U);
    if (Actor && MappaRiferimento)
    {
        const float CS   = MappaRiferimento->DimensioneCella;
        const float Half = (MappaRiferimento->DimensioneGriglia - 1) * CS * 0.5f;
        Actor->SetPosizioneGriglia(U.SpawnX, U.SpawnY, CS, Half);

        FVector Pos = Actor->GetActorLocation();
        const FCellData* C = MappaRiferimento->GetCella(U.SpawnX, U.SpawnY);
        if (C) Pos.Z = C->PosizioneMondo.Z + CS * 0.5f + 5.f;
        Actor->SetActorLocation(Pos);
    }

    UE_LOG(LogTemp, Log, TEXT("[GameManager] %s respawn in %s"),
        *U.GetNome(), *CellaToString(U.SpawnX, U.SpawnY));
}

// ============================================================
//  FINE TURNO UNITA'
// ============================================================
void AGameManager::FineTurnoUnita(int32 ID)
{
    // Controlla se tutte le unita' del giocatore corrente hanno finito
    int32 Rimanenti = 0;
    for (const FUnita& U : Unita)
        if (U.Owner == PlayerCorrente && U.bViva && !U.AzioniFinite())
            Rimanenti++;

    if (Rimanenti == 0)
        FineTurnoGiocatore();
    else
        SetStatus(FString::Printf(
            TEXT("Ancora %d unita' da muovere."), Rimanenti));
}

// ============================================================
//  FINE TURNO GIOCATORE
// ============================================================
void AGameManager::FineTurnoGiocatore()
{
    DeselezionaUnita();

    // Aggiorna torri a fine turno
    AggiornaTorri();

    // Controlla fine partita per torri
    ControllaTurnoFinePartita();
    if (FaseCorrente == EFaseGioco::FinePartita) return;

    // Cambia giocatore
    PlayerCorrente = (PlayerCorrente == EOwner::Human)
        ? EOwner::AI : EOwner::Human;

    if (PlayerCorrente == EOwner::Human) TurnoCorrente++;

    for (FUnita& U : Unita)
        if (U.Owner == PlayerCorrente) U.ResetTurno();

    StampaStorico();

    if (PlayerCorrente == EOwner::AI)
    {
        SetStatus(TEXT("Turno AI..."));

        // Piccolo delay prima del turno AI
        FTimerHandle T;
        GetWorldTimerManager().SetTimer(T, [this]() { TurnoAI(); }, 0.5f, false);
    }
    else
    {
        SetStatus(FString::Printf(
            TEXT("=== TURNO %d - TUO TURNO ==="), TurnoCorrente));
    }
}

// ============================================================
//  CONTROLLO FINE PARTITA (torri)
// ============================================================
void AGameManager::ControllaTurnoFinePartita()
{
    const EOwner Possesso = ControllaPossessoTorri();

    if (Possesso == EOwner::Human)
    {
        TurniConsecutiviHuman++;
        TurniConsecutiviAI = 0;
    }
    else if (Possesso == EOwner::AI)
    {
        TurniConsecutiviAI++;
        TurniConsecutiviHuman = 0;
    }
    else
    {
        TurniConsecutiviHuman = 0;
        TurniConsecutiviAI   = 0;
    }

    // Fine partita: 2 torri su 3 per 2 turni consecutivi
    if (TurniConsecutiviHuman >= 2)
    {
        FaseCorrente = EFaseGioco::FinePartita;
        MessaggioVincitore = TEXT("HAI VINTO!");
        SetStatus(TEXT("*** HAI VINTO! Controlli 2 torri per 2 turni! ***"));
        return;
    }
    if (TurniConsecutiviAI >= 2)
    {
        FaseCorrente = EFaseGioco::FinePartita;
        MessaggioVincitore = TEXT("HAI PERSO! L'AI vince.");
        SetStatus(TEXT("*** HAI PERSO! L'AI controlla 2 torri per 2 turni! ***"));
    }
}

// ============================================================
//  TURNO AI
// ============================================================
// Design doc: "nel caso della AI, si evidenzia sulla griglia il movimento e/o il range di attacco"
// Questa funzione evidenzia brevemente il range di un'unita' AI prima che si muova
void AGameManager::MostraRangeAI(const FUnita& U)
{
    if (!MappaRiferimento) return;

    // Calcola range movimento
    TArray<FCellaRaggiungibile> Range =
        FPathfinder::CalcolaRaggiungibili(U, MappaRiferimento, Unita);

    // Colora celle blu scuro (range AI)
    for (const FCellaRaggiungibile& C : Range)
        MappaRiferimento->SetColoreRange(C.X, C.Y,
            FLinearColor(0.3f, 0.3f, 0.8f));  // Blu per range AI

    // Colora l'unita' AI in giallo
    MappaRiferimento->SetColoreRange(U.GridX, U.GridY,
        FLinearColor(1.0f, 0.7f, 0.0f));
}

void AGameManager::TurnoAI()
{
    for (FUnita& AIU : Unita)
    {
        if (AIU.Owner != EOwner::AI || !AIU.bViva) continue;

        // ── Trova obiettivo: torre libera o nemico ─────────────
        int32 TargetX = -1, TargetY = -1;
        bool  bTargetTorre = false;

        // Priorita' 1: torre libera/neutrale vicina
        int32 MinDistTorre = INT_MAX;
        for (const FTorre& T : Torri)
        {
            if (T.Stato == EStatoTorre::AI) continue; // gia' nostra

            const int32 D = FMath::Abs(AIU.GridX - T.GridX)
                          + FMath::Abs(AIU.GridY - T.GridY);
            if (D < MinDistTorre)
            {
                MinDistTorre = D;
                TargetX = T.GridX; TargetY = T.GridY;
                bTargetTorre = true;
            }
        }

        // Priorita' 2: nemico piu' vicino (se torre e' lontana)
        int32 MinDistNemico = INT_MAX;
        FUnita* Bersaglio = nullptr;

        for (FUnita& HU : Unita)
        {
            if (HU.Owner != EOwner::Human || !HU.bViva) continue;
            const int32 D = FMath::Abs(AIU.GridX - HU.GridX)
                          + FMath::Abs(AIU.GridY - HU.GridY);
            if (D < MinDistNemico)
            {
                MinDistNemico = D;
                Bersaglio = &HU;
            }
        }

        // Se il nemico e' piu' vicino della torre (o in range attacco)
        if (Bersaglio &&
            (MinDistNemico <= AIU.RangeAttacco ||
             (!bTargetTorre || MinDistNemico < MinDistTorre - 2)))
        {
            TargetX = Bersaglio->GridX;
            TargetY = Bersaglio->GridY;
            bTargetTorre = false;
        }

        // ── Attacca se in range ────────────────────────────────
        if (Bersaglio && !AIU.bHaAttaccato)
        {
            TArray<TPair<int32,int32>> RangeAtt =
                FPathfinder::CalcolaRangeAttacco(AIU, MappaRiferimento, Unita);

            for (const auto& P : RangeAtt)
            {
                if (P.Key == Bersaglio->GridX && P.Value == Bersaglio->GridY)
                {
                    // Design doc: evidenzia range attacco AI
                    MostraRangeAI(AIU);
                    // Attacca
                    UnitaSelezionata = AIU.ID;
                    EseguiAttacco(Bersaglio->GridX, Bersaglio->GridY);
                    UnitaSelezionata = -1;
                    break;
                }
            }
        }

        // ── Muovi verso l'obiettivo ────────────────────────────
        if (!AIU.bHaMosso && TargetX >= 0)
        {
            TArray<FCellaRaggiungibile> Raggiungibili =
                FPathfinder::CalcolaRaggiungibili(AIU, MappaRiferimento, Unita);

            // Design doc: evidenzia il range dell'AI durante la sua mossa
            MostraRangeAI(AIU);

            // Trova la cella raggiungibile piu' vicina all'obiettivo
            int32 BestX = -1, BestY = -1, BestDist = INT_MAX;

            for (const FCellaRaggiungibile& CR : Raggiungibili)
            {
                const int32 D = FMath::Abs(CR.X - TargetX)
                              + FMath::Abs(CR.Y - TargetY);
                if (D < BestDist)
                {
                    BestDist = D;
                    BestX = CR.X; BestY = CR.Y;
                }
            }

            if (BestX >= 0 && (BestX != AIU.GridX || BestY != AIU.GridY))
            {
                UnitaSelezionata = AIU.ID;
                EseguiMovimento(BestX, BestY);
                UnitaSelezionata = -1;
            }
            else
            {
                AIU.bHaMosso = true;
            }
        }

        if (!AIU.bHaAttaccato) AIU.bHaAttaccato = true;
        if (!AIU.bHaMosso)     AIU.bHaMosso     = true;
    }

    FineTurnoGiocatore();
}

// ============================================================
//  PIAZZA UNITA'
// ============================================================
void AGameManager::PiazzaUnita(int32 ID, int32 X, int32 Y)
{
    FUnita* U = TrovaUnita(ID);
    if (!U) return;

    U->GridX  = X; U->GridY  = Y;
    U->SpawnX = X; U->SpawnY = Y;

    AUnitActor* Actor = SpawnActor(*U);
    if (Actor && MappaRiferimento)
    {
        const float CS   = MappaRiferimento->DimensioneCella;
        const float Half = (MappaRiferimento->DimensioneGriglia - 1) * CS * 0.5f;
        Actor->SetPosizioneGriglia(X, Y, CS, Half);

        FVector Pos = Actor->GetActorLocation();
        const FCellData* C = MappaRiferimento->GetCella(X, Y);
        if (C) Pos.Z = C->PosizioneMondo.Z + CS * 0.5f + 5.f;
        Actor->SetActorLocation(Pos);
    }

    UE_LOG(LogTemp, Log, TEXT("[GameManager] %s piazzato in %s"),
        *U->GetNome(), *CellaToString(X, Y));
}

AUnitActor* AGameManager::SpawnActor(const FUnita& U)
{
    if (!GetWorld()) return nullptr;

    FActorSpawnParameters P;
    P.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AUnitActor* A = GetWorld()->SpawnActor<AUnitActor>(
        AUnitActor::StaticClass(),
        FVector::ZeroVector, FRotator::ZeroRotator, P);

    if (A)
    {
        A->Init(U, MappaRiferimento ? MappaRiferimento->DimensioneCella : 100.f);
        UnitaActors.Add(A);
    }
    return A;
}

// ============================================================
//  START GIOCO
// ============================================================
void AGameManager::StartGioco()
{
    FaseCorrente   = EFaseGioco::GiocoInCorso;
    PlayerCorrente = SequenzaPiazzamento[0].Owner;
    TurnoCorrente  = 1;

    for (FUnita& U : Unita)
        if (U.Owner == PlayerCorrente) U.ResetTurno();

    SetStatus(FString::Printf(
        TEXT("=== TURNO 1 - %s INIZIA ==="),
        (PlayerCorrente == EOwner::Human) ? TEXT("TU") : TEXT("AI")));

    if (PlayerCorrente == EOwner::AI)
    {
        FTimerHandle T;
        GetWorldTimerManager().SetTimer(T, [this]() { TurnoAI(); }, 0.5f, false);
    }
}

// ============================================================
//  RANGE VISIVO
// ============================================================
void AGameManager::AggiornaRangeVisivo(int32 ID)
{
    CancellaRangeVisivo();
    const FUnita* U = TrovaUnita(ID);
    if (!U) return;

    if (!U->bHaMosso)
        RangeMovimentoCorrente = FPathfinder::CalcolaRaggiungibili(
            *U, MappaRiferimento, Unita);

    if (!U->bHaAttaccato)
        RangeAttaccoCorrente = FPathfinder::CalcolaRangeAttacco(
            *U, MappaRiferimento, Unita);

    // Colora celle sulla griglia
    if (MappaRiferimento)
    {
        // Blu = celle raggiungibili
        for (const FCellaRaggiungibile& CR : RangeMovimentoCorrente)
            MappaRiferimento->SetColoreRange(CR.X, CR.Y,
                AGridMapGenerator::GetColoreMovimento());

        // Rosso = nemici attaccabili
        for (const auto& P : RangeAttaccoCorrente)
            MappaRiferimento->SetColoreRange(P.Key, P.Value,
                AGridMapGenerator::GetColoreAttacco());

        // Giallo = cella unita' selezionata
        MappaRiferimento->SetColoreRange(U->GridX, U->GridY,
            AGridMapGenerator::GetColoreSelezionato());
    }

    UE_LOG(LogTemp, Log,
        TEXT("[GameManager] Range movimento: %d celle, Range attacco: %d nemici"),
        RangeMovimentoCorrente.Num(), RangeAttaccoCorrente.Num());
}

void AGameManager::CancellaRangeVisivo()
{
    // Ripristina colori griglia
    if (MappaRiferimento)
        MappaRiferimento->ResetTuttiColori();

    RangeMovimentoCorrente.Empty();
    RangeAttaccoCorrente.Empty();
}

// ============================================================
//  STORICO MOSSE
// ============================================================
void AGameManager::LogMossa(const FString& Testo)
{
    StoricoMosse.Add(Testo);
    UE_LOG(LogTemp, Log, TEXT("[Storico] %s"), *Testo);
}

void AGameManager::StampaStorico() const
{
    UE_LOG(LogTemp, Log, TEXT("=== STORICO MOSSE ==="));
    for (const FString& M : StoricoMosse)
        UE_LOG(LogTemp, Log, TEXT("  %s"), *M);
    UE_LOG(LogTemp, Log, TEXT("====================="));
}

// ============================================================
//  HELPER
// ============================================================
FUnita* AGameManager::TrovaUnita(int32 ID)
{
    for (FUnita& U : Unita) if (U.ID == ID) return &U;
    return nullptr;
}

FUnita* AGameManager::UnitaSuCella(int32 X, int32 Y)
{
    for (FUnita& U : Unita)
        if (U.bViva && U.GridX == X && U.GridY == Y) return &U;
    return nullptr;
}

AUnitActor* AGameManager::TrovaActor(int32 ID)
{
    for (AUnitActor* A : UnitaActors)
        if (A && A->GetUnitaID() == ID) return A;
    return nullptr;
}

int32 AGameManager::RollDanno(const FUnita& U) const
{
    return FMath::RandRange(U.DannoMin, U.DannoMax);
}

void AGameManager::SetStatus(const FString& Msg)
{
    MessaggioStatus = Msg;
    UE_LOG(LogTemp, Log, TEXT("[GameManager] %s"), *Msg);
}

// ============================================================
//  GetTorreInfo - per HUD
// ============================================================
FString AGameManager::GetTorreInfo(int32 i) const
{
    if (!Torri.IsValidIndex(i)) return TEXT("???");
    const FTorre& T = Torri[i];
    const FString Coord = CellaToString(T.GridX, T.GridY);
    const FString Stato =
        T.Stato == EStatoTorre::Human   ? TEXT("HUMAN")   :
        T.Stato == EStatoTorre::AI      ? TEXT("AI")      :
        T.Stato == EStatoTorre::Contesa ? TEXT("CONTESA") :
                                           TEXT("NEUTRALE");
    return FString::Printf(TEXT("%s: %s"), *Coord, *Stato);
}

// ============================================================
//  TerminaTurno — pulsante UI
// ============================================================
void AGameManager::TerminaTurno()
{
    if (FaseCorrente != EFaseGioco::GiocoInCorso) return;
    if (PlayerCorrente != EOwner::Human) return;

    UE_LOG(LogTemp, Log, TEXT("[GameManager] Turno terminato manualmente."));
    CancellaRangeVisivo();
    UnitaSelezionata = -1;

    // Passa all'AI
    FTimerHandle T;
    GetWorldTimerManager().SetTimer(T, [this]() { TurnoAI(); }, 0.3f, false);
}
