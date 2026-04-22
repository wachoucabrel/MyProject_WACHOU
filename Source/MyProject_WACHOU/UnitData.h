// ============================================================
// UnitData.h  -  MyProject_WACHOU
// ============================================================
// Definisce tutti i tipi fondamentali del gioco WACHOU:
//
//   ENUM:
//     EOwner       → proprietario unita' (Human/AI/Nessuno)
//     ETipoUnita   → Sniper o Brawler
//     ETipoAttacco → Distanza (attraversa acqua) o CorpoACorpo
//     EFaseGioco   → Configurazione, GenerazioneMappa, LancioMoneta,
//                    Piazzamento, GiocoInCorso, FinePartita
//
//   STRUCT:
//     FStatisticheUnita → statistiche fisse (read-only) da design doc
//                         Sniper: mov=4, range=10, danno=4-8, HP=20
//                         Brawler: mov=6, range=1, danno=1-6, HP=40
//     FUnita            → stato runtime (HP, posizione, flag turno)
//
//   FUNZIONI:
//     CellaToString(X,Y) → converte coordinate in formato "A0".."Y24"
//                          come richiesto dal design doc
// ============================================================
#pragma once

#include "CoreMinimal.h"
#include "UnitData.generated.h"

// ── Proprietario ──────────────────────────────────────────────
UENUM()
enum class EOwner : uint8
{
    Nessuno,
    Human,
    AI
};

// ── Tipo unita' ───────────────────────────────────────────────
UENUM()
enum class ETipoUnita : uint8
{
    Sniper,
    Brawler
};

// ── Tipo attacco ──────────────────────────────────────────────
UENUM()
enum class ETipoAttacco : uint8
{
    Distanza,    // Sniper: puo' attraversare acqua
    CorpoACorpo  // Brawler: non puo' attraversare acqua
};

// ── Fase di gioco ─────────────────────────────────────────────
UENUM()
enum class EFaseGioco : uint8
{
    Configurazione,
    GenerazioneMappa,
    LancioMoneta,
    Piazzamento,
    GiocoInCorso,
    FinePartita
};

// ── Statistiche fisse (tabella read-only) ─────────────────────
struct FStatisticheUnita
{
    ETipoUnita   Tipo         = ETipoUnita::Sniper;
    ETipoAttacco TipoAttacco  = ETipoAttacco::Distanza;
    int32        HPMax        = 0;
    int32        Movimento    = 0;
    int32        RangeAttacco = 0;
    int32        DannoMin     = 0;
    int32        DannoMax     = 0;

    static FStatisticheUnita MakeSniper()
    {
        FStatisticheUnita S;
        S.Tipo         = ETipoUnita::Sniper;
        S.TipoAttacco  = ETipoAttacco::Distanza;
        S.HPMax        = 20;
        S.Movimento    = 4;
        S.RangeAttacco = 10;
        S.DannoMin     = 4;
        S.DannoMax     = 8;
        return S;
    }

    static FStatisticheUnita MakeBrawler()
    {
        FStatisticheUnita S;
        S.Tipo         = ETipoUnita::Brawler;
        S.TipoAttacco  = ETipoAttacco::CorpoACorpo;
        S.HPMax        = 40;
        S.Movimento    = 6;
        S.RangeAttacco = 1;
        S.DannoMin     = 1;
        S.DannoMax     = 6;
        return S;
    }

    FString GetNome() const
    {
        return (Tipo == ETipoUnita::Sniper) ? TEXT("Sniper") : TEXT("Brawler");
    }

    // Identificativo lettera per storico mosse (S=Sniper, B=Brawler)
    FString GetLettera() const
    {
        return (Tipo == ETipoUnita::Sniper) ? TEXT("S") : TEXT("B");
    }
};

// ── Stato runtime unita' ─────────────────────────────────────
USTRUCT()
struct MYPROJECT_WACHOU_API FUnita
{
    GENERATED_BODY()

    // Identita'
    UPROPERTY() int32        ID          = -1;
    UPROPERTY() EOwner       Owner       = EOwner::Nessuno;
    UPROPERTY() ETipoUnita   Tipo        = ETipoUnita::Sniper;
    UPROPERTY() ETipoAttacco TipoAttacco = ETipoAttacco::Distanza;

    // Statistiche fisse
    UPROPERTY() int32 HPMax        = 0;
    UPROPERTY() int32 Movimento    = 0;
    UPROPERTY() int32 RangeAttacco = 0;
    UPROPERTY() int32 DannoMin     = 0;
    UPROPERTY() int32 DannoMax     = 0;

    // Stato runtime
    UPROPERTY() int32 HP     = 0;
    UPROPERTY() int32 GridX  = -1;
    UPROPERTY() int32 GridY  = -1;
    UPROPERTY() bool  bViva  = true;

    // Posizione iniziale (per respawn)
    UPROPERTY() int32 SpawnX = -1;
    UPROPERTY() int32 SpawnY = -1;

    // Stato turno
    UPROPERTY() bool bHaMosso     = false;
    UPROPERTY() bool bHaAttaccato = false;

    // ── Factory ───────────────────────────────────────────────
    static FUnita Crea(int32 InID, EOwner InOwner, ETipoUnita InTipo)
    {
        FUnita U;
        U.ID    = InID;
        U.Owner = InOwner;
        U.Tipo  = InTipo;

        const FStatisticheUnita S = (InTipo == ETipoUnita::Sniper)
            ? FStatisticheUnita::MakeSniper()
            : FStatisticheUnita::MakeBrawler();

        U.TipoAttacco  = S.TipoAttacco;
        U.HPMax        = S.HPMax;
        U.HP           = S.HPMax;
        U.Movimento    = S.Movimento;
        U.RangeAttacco = S.RangeAttacco;
        U.DannoMin     = S.DannoMin;
        U.DannoMax     = S.DannoMax;
        return U;
    }

    // ── Query ─────────────────────────────────────────────────
    FORCEINLINE bool IsPiazzata()   const { return GridX >= 0 && GridY >= 0; }
    FORCEINLINE bool AzioniFinite() const { return bHaMosso && bHaAttaccato; }
    FORCEINLINE bool PuoAgire()     const { return bViva && !AzioniFinite(); }

    void ResetTurno() { bHaMosso = false; bHaAttaccato = false; }

    // Identificativo per storico mosse
    FString GetLettera() const
    {
        return (Tipo == ETipoUnita::Sniper) ? TEXT("S") : TEXT("B");
    }

    // Colore: ciano=Human, rosso=AI
    FLinearColor GetColore() const
    {
        return (Owner == EOwner::Human)
            ? FLinearColor(0.f, 0.8f, 1.f)
            : FLinearColor(1.f, 0.15f, 0.15f);
    }

    FString GetNome() const
    {
        const FString O = (Owner == EOwner::Human) ? TEXT("HP") : TEXT("AI");
        return FString::Printf(TEXT("%s_%s_%d"), *O, *GetLettera(), ID);
    }
};

// ── Coordinata cella in formato A0..Y24 ───────────────────────
// Asse X = lettere A..Y (0..24)
// Asse Y = numeri 0..24
inline FString CellaToString(int32 X, int32 Y)
{
    if (X < 0 || X > 24 || Y < 0 || Y > 24)
        return TEXT("??");
    const TCHAR Lettera = static_cast<TCHAR>(TEXT('A') + X);
    return FString::Printf(TEXT("%c%d"), Lettera, Y);
}
