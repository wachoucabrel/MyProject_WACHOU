// ============================================================
// GameManager.h  -  MyProject_WACHOU
// ============================================================
// Controller centrale della logica di gioco.
//
//   FASI GESTITE:
//     Configurazione → schermata parametri mappa
//     LancioMoneta   → 50/50 random determina chi inizia
//     Piazzamento    → alternato Y=0-2 (Human) / Y=22-24 (AI)
//     GiocoInCorso   → turni alternati
//     FinePartita    → 2 torri x 2 turni consecutivi
//
//   FUNZIONI PUBBLICHE:
//     IniziaGioco()          → da chiamare dal pulsante UI
//     OnCellaCliccata()      → routing input giocatore
//     TerminaTurno()         → fine turno anticipata (UI)
//     GetTutteUnita()        → per HUD
//     GetTorreInfo()         → per HUD
//     GetVincitore()         → per schermata fine partita
//     MostraRangeAI()        → evidenzia movimento AI
//
//   STRUCT:
//     FTorre → posizione, stato (NEUTRALE/HUMAN/AI/CONTESA),
//              turni consecutivi controllo
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnitData.h"
#include "Pathfinder.h"
#include "GameManager.generated.h"

class AGridMapGenerator;
class AUnitActor;

// ── Stato torre ───────────────────────────────────────────────
UENUM()
enum class EStatoTorre : uint8
{
    Neutrale,
    Human,
    AI,
    Contesa
};

USTRUCT()
struct FTorre
{
    GENERATED_BODY()
    UPROPERTY() int32       GridX       = 0;
    UPROPERTY() int32       GridY       = 0;
    UPROPERTY() EStatoTorre Stato       = EStatoTorre::Neutrale;
    UPROPERTY() int32       TurniHuman  = 0; // turni consecutivi controllo Human
    UPROPERTY() int32       TurniAI     = 0; // turni consecutivi controllo AI
};

UCLASS()
class MYPROJECT_WACHOU_API AGameManager : public AActor
{
    GENERATED_BODY()

public:
    AGameManager();

    // ── Parametri editor ──────────────────────────────────────
    UPROPERTY(EditAnywhere, Category = "Wachou")
    AGridMapGenerator* MappaRiferimento = nullptr;

    // ── API pubblica ──────────────────────────────────────────
    void IniziaGioco();
    void OnCellaCliccata(int32 GridX, int32 GridY);

    EFaseGioco GetFase()           const { return FaseCorrente; }
    EOwner     GetPlayerCorrente() const { return PlayerCorrente; }
    int32      GetTurnoCorrente()  const { return TurnoCorrente; }
    FString    GetStatus()         const { return MessaggioStatus; }

    // Unita'e torri per HUD
    const TArray<FUnita>& GetTutteUnita()  const { return Unita; }
    int32      GetNumTorri()      const { return Torri.Num(); }
    FString    GetTorreInfo(int32 i) const;
    FString    GetVincitore()     const { return MessaggioVincitore; }

    // Termina turno anticipatamente (pulsante UI)
    void TerminaTurno();
    void MostraRangeAI(const FUnita& U);


    // Range visivo (per il renderer)
    const TArray<FCellaRaggiungibile>& GetRangeMovimento() const
    { return RangeMovimentoCorrente; }
    const TArray<TPair<int32,int32>>& GetRangeAttacco() const
    { return RangeAttaccoCorrente; }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    // ── Dati di gioco ─────────────────────────────────────────
    UPROPERTY() TArray<FUnita>      Unita;
    UPROPERTY() TArray<AUnitActor*> UnitaActors;
    UPROPERTY() TArray<FTorre>      Torri;

    EFaseGioco FaseCorrente    = EFaseGioco::GenerazioneMappa;
    EOwner     PlayerCorrente  = EOwner::Human;
    int32      TurnoCorrente   = 1;
    int32      UnitaSelezionata= -1;
    int32      NextUnitaID     = 0;
    FString    MessaggioStatus;
    FString    MessaggioVincitore;

    // Range visivo corrente
    TArray<FCellaRaggiungibile>  RangeMovimentoCorrente;
    TArray<TPair<int32,int32>>   RangeAttaccoCorrente;

    // Storico mosse
    TArray<FString> StoricoMosse;

    // Turni consecutivi controllo torri
    int32 TurniConsecutiviHuman = 0;
    int32 TurniConsecutiviAI    = 0;

    // ── Step piazzamento ──────────────────────────────────────
    struct FStepPiazzamento { EOwner Owner; ETipoUnita Tipo; };
    TArray<FStepPiazzamento> SequenzaPiazzamento;
    int32 StepPiazzamento = 0;

    // ── Fasi ──────────────────────────────────────────────────
    void StartLancioMoneta();
    void StartPiazzamento(EOwner Primo);
    void AvanzaPiazzamento();
    void PiazzaUnitaAI();
    void StartGioco();

    // ── Turno ─────────────────────────────────────────────────
    void SelezionaUnita(int32 ID);
    void DeselezionaUnita();
    void EseguiMovimento(int32 GridX, int32 GridY);
    void EseguiAttacco(int32 GridX, int32 GridY);
    void FineTurnoUnita(int32 ID);
    void FineTurnoGiocatore();
    void TurnoAI();

    // ── Torri ─────────────────────────────────────────────────
    void InizializzaTorri();
    void AggiornaTorri();           // chiamata a fine di ogni turno
    bool ControllaTorreProssima(int32 X, int32 Y, int32& OutTorreIdx) const;
    EOwner ControllaPossessoTorri() const; // chi ha 2+ torri

    // ── Vittoria ──────────────────────────────────────────────
    void ControllaTurnoFinePartita();

    // ── Respawn ───────────────────────────────────────────────
    void RespawnUnita(FUnita& U);

    // ── Storico mosse ─────────────────────────────────────────
    void LogMossa(const FString& Testo);
    void StampaStorico() const;

    // ── Helper ────────────────────────────────────────────────
    bool        IsZonaHuman(int32 Y) const { return Y >= 0  && Y <= 2;  }
    bool        IsZonaAI   (int32 Y) const { return Y >= 22 && Y <= 24; }
    FUnita*     TrovaUnita(int32 ID);
    FUnita*     UnitaSuCella(int32 X, int32 Y);
    AUnitActor* TrovaActor(int32 ID);
    int32       RollDanno(const FUnita& U) const;
    void        PiazzaUnita(int32 ID, int32 X, int32 Y);
    AUnitActor* SpawnActor(const FUnita& U);
    void        SetStatus(const FString& Msg);
    void        AggiornaRangeVisivo(int32 UnitaID);
    void        CancellaRangeVisivo();

    // Distanza Chebyshev (per torri)
    int32 DistanzaChebyshev(int32 X1, int32 Y1, int32 X2, int32 Y2) const
    {
        return FMath::Max(FMath::Abs(X1-X2), FMath::Abs(Y1-Y2));
    }
};
