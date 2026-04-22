# Step 2 - Sistema di Gioco Completo
## MyProject_WACHOU

### File inclusi in questo zip

| File | Descrizione |
|------|-------------|
| UnitData.h | Enum, statistiche unita', struct FUnita, formato coordinate A0-Y24 |
| Pathfinder.h/.cpp | Dijkstra con costi elevazione, range attacco, percorso minimo |
| UnitActor.h/.cpp | Actor visivo con movimento animato lungo il percorso |
| GameManager.h/.cpp | Logica di gioco completa (vedi sotto) |
| WachouPlayerController.h/.cpp | Click mouse → coordinata griglia |
| WachouGameMode.h/.cpp | Registra PlayerController |

---

### Funzionalita' implementate

✅ Statistiche Sniper e Brawler esatte dal design doc
✅ Lancio moneta casuale
✅ Piazzamento alternato (zone Y=0-2 Human, Y=22-24 AI)
✅ Movimento con costi reali: piano=1, salita=2, discesa=1
✅ Pathfinding Dijkstra (percorso minimo, no teletrasporto)
✅ Ostacoli: acqua, torri e unita' bloccano il percorso
✅ Attacco con regola elevazione (solo uguale o inferiore)
✅ Ranged (Sniper) attraversa acqua, Melee (Brawler) no
✅ Contrattacco Sniper (danno 1-3)
✅ Respawn unita' eliminate nella posizione iniziale
✅ Storico mosse formato: HP: S B4 -> D6
✅ Coordinate celle formato A0..Y24
✅ Sistema torri: conquista a distanza Chebyshev <= 2
✅ Fine partita: 2 torri su 3 per 2 turni consecutivi
✅ AI con priorita' torri
✅ Range movimento calcolato (visibile nel log)
✅ Turni alternati con log chiaro
✅ Stato unita' stampato nel log

---

### Installazione

1. Copia TUTTI i file nella cartella:
   `Source/MyProject_WACHOU/`

2. In Unreal premi `Ctrl+Alt+F11` per compilare

3. Trascina `GameManager` nella scena dal Content Browser

4. Seleziona GameManager → pannello Details → campo
   `Mappa Riferimento` → seleziona il GridMapGenerator
   gia' presente nella scena

5. Vai su `Edit → Project Settings → Maps & Modes`
   → Default GameMode → seleziona `WachouGameMode`

6. Premi ▶ Play

---

### Come si gioca

**Piazzamento:**
- Clicca una cella nelle righe 0-2 per piazzare le tue unita'
- L'AI piazza automaticamente nelle righe 22-24
- Prima Sniper, poi Brawler (alternati)

**Turno:**
- Clicca su una tua unita' (ciano) per selezionarla
- Clicca su una cella libera per muoverti
- Clicca su un nemico (rosso) per attaccare
- Puoi fare entrambe le azioni nello stesso turno

**Output Log:**
- Apri `Window → Output Log`
- Filtra per `LogTemp` per vedere lo stato del gioco
- Il formato delle mosse e': `HP: S B4 -> D6`

---

### Note tecniche

Il range di movimento e attacco viene calcolato ogni volta
che selezioni un'unita' e stampato nel log.
La visualizzazione colorata sulla griglia (celle blu/rosse)
sara' aggiunta nello Step 3 con il renderer dedicato.
