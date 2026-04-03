// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TileTypes.h"
#include "MahjongPlayer.h"
#include "MahjongPlayerState.h"
#include "MahjongGameMode.generated.h"



// ── Phase enum ────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    WaitingToStart  UMETA(DisplayName = "Waiting to Start"),
    Dealing         UMETA(DisplayName = "Dealing"),
    PlayerDraw      UMETA(DisplayName = "Player Draw"),
    PlayerAction    UMETA(DisplayName = "Player Action"),
    ResponseWindow  UMETA(DisplayName = "Response Window"),
    ProcessingCall  UMETA(DisplayName = "Processing Call"),
    RoundEnd        UMETA(DisplayName = "Round End"),
    GameEnd         UMETA(DisplayName = "Game End"),
};

// ── Call type ─────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class ECallType : uint8
{
    None UMETA(DisplayName = "None"),
    Chi  UMETA(DisplayName = "Chi"),
    Pon  UMETA(DisplayName = "Pon"),
    Kan  UMETA(DisplayName = "Kan"),
    Ron  UMETA(DisplayName = "Ron"),
};

// ── Pending call (submitted during the response window) ───────────────────────
USTRUCT(BlueprintType)
struct FPendingCall
{
    GENERATED_BODY()

    UPROPERTY() ECallType Type = ECallType::None;
    UPROPERTY() int32     PlayerIndex = -1;

    int32 GetPriority() const
    {
        switch (Type)
        {
        case ECallType::Ron: return 4;
        case ECallType::Kan: return 3;
        case ECallType::Pon: return 2;
        case ECallType::Chi: return 1;
        default:             return 0;
        }
    }
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPhaseChanged, EGamePhase, OldPhase, EGamePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTileDiscarded, int32, PlayerIndex, FTileData, Tile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRoundEnded, int32, WinnerIndex, bool, bTsumo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAwaitingStep, FString, ActionText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTileDrawn, int32, PlayerIndex, FTileData, Tile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRoundResult, const FString&, ResultText, float, DisplayDuration);

UCLASS()
class MAHJONG_API AMahjongGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMahjongGameMode();

    // ── Entry points (call from BeginPlay or a "New Game" button) ────────────
    UFUNCTION(BlueprintCallable, Category = "Mahjong|Game")
    void StartGame();

    // Human player calls this once they've picked a discard tile
    UFUNCTION(BlueprintCallable, Category = "Mahjong|Game")
    void OnPlayerDiscard(int32 PlayerIndex, const FTileData& DiscardedTile);

    // Called during the response window (by UI for human, automatically for AI)
    UFUNCTION(BlueprintCallable, Category = "Mahjong|Game")
    void SubmitCall(int32 PlayerIndex, ECallType CallType);

    UFUNCTION(BlueprintCallable, Category = "Mahjong|Game")
    void SubmitPass(int32 PlayerIndex);

    UFUNCTION(BlueprintCallable)
    int32 GetWAllTilesRemaining() const { return Wall.Num() - WallDrawIndex - DeadWallSize; }

    UFUNCTION()
    void OnDealingComplete();


    // ── Events — bind in Blueprint or C++ ────────────────────────────────────
    UPROPERTY(BlueprintAssignable, Category = "Mahjong|Events")
    FOnPhaseChanged  OnPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category = "Mahjong|Events")
    FOnTileDiscarded OnTileDiscarded;

    UPROPERTY(BlueprintAssignable, Category = "Mahjong|Events")
    FOnRoundEnded    OnRoundEnded;

    UPROPERTY(BlueprintAssignable, Category = "Mahjong|Events")
    FOnTileDrawn OnTileDrawn;

    // ── Read-only state (query from UI/Blueprint) ─────────────────────────────
    UPROPERTY(BlueprintReadOnly, Category = "Mahjong|State")
    EGamePhase CurrentPhase = EGamePhase::WaitingToStart;

    UPROPERTY(BlueprintReadOnly, Category = "Mahjong|State")
    int32 CurrentPlayerIndex = 0;

    // ── Tunable timing ────────────────────────────────────────────────────────
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mahjong|Timing")
    float AIThinkingTime = 1.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mahjong|Timing")
    float ResponseWindowDuration = 3.0f;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong|Debug")
    bool bStepMode = true;

    UPROPERTY(BlueprintAssignable) 
    FOnRoundResult OnRoundResult;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game|Round")
    float RoundRestartDelay = 10.0f;


    // Called by the UI button to advance one turn
    UFUNCTION(BlueprintCallable, Category = "Mahjong|Debug")
    void StepNextTurn();

    UFUNCTION(BlueprintCallable, Category = "Game") void ToggleStepMode();
    UFUNCTION(BlueprintCallable, Category="Game") bool GetStepMode() const { return bStepMode; }


    // Read by the widget to display what just happened
    UPROPERTY(BlueprintReadOnly, Category = "Mahjong|Debug")
    FString LastActionText;

    UPROPERTY(BlueprintAssignable, Category = "Mahjong|Debug")
    FOnAwaitingStep OnAwaitingStep;

    AMahjongPlayerState* GetPlayerStateAt(int32 Index) const;

    FTileData LastDrawnTile;

protected:
    virtual void BeginPlay() override;

private:
    // ── Phase machine ─────────────────────────────────────────────────────────
    void TransitionToPhase(EGamePhase NewPhase);
    void StartRound();
    void BeginPlayerTurn(int32 PlayerIndex);
    void BeginResponseWindow(const FTileData& DiscardedTile, int32 DiscardingPlayerIndex);
    void ResolveResponseWindow();
    void ApplyCall(const FPendingCall& Call, const FTileData& ClaimedTile);
    void EndRound(int32 WinningPlayerIndex, bool bIsTsumo);

   

    bool bWaitingForDealAnimation = false;
    // Tracks whether we're waiting for dealing animation to finish


    // ── AI scheduling ─────────────────────────────────────────────────────────
    void ScheduleAIDiscard(int32 PlayerIndex);
    void ScheduleAIDiscardOld(int32 PlayerIndex);
    void ScheduleAICallDecision(int32 PlayerIndex);

    // ── Helpers ───────────────────────────────────────────────────────────────
    int32               GetNextPlayerIndex(int32 From) const;
    AMahjongPlayer* GetPlayer(int32 Index) const;
    bool                IsHumanPlayer(int32 Index) const;
    void                CheckAllPlayersResponded();

    // ── Wall ─────────────────────────────────────────────────────────────────
    void BuildAndShuffleWall();
    FTileData DrawFromWall();
    bool IsWallEmpty() const;

    // ── Runtime state ─────────────────────────────────────────────────────────
    UPROPERTY() TArray<AMahjongPlayer*> Players;
    UPROPERTY() TArray<FTileData> Wall;

    int32 WallDrawIndex = 0;
    int32 LastDiscardingPlayerIndex = -1;
    FTileData LastDiscardedTile;

    TArray<bool> PlayerHasResponded; // per-player flag during response window
    TArray<FPendingCall> PendingCalls;

    // One handle for the current player's AI discard, separate array for
    // concurrent call-decision timers (one per opposing AI player).
    FTimerHandle AIDiscardTimerHandle;
    TArray<FTimerHandle> CallDecisionTimerHandles;
    FTimerHandle ResponseWindowTimerHandle;
    FTimerHandle RoundRestartTimerHandle;
    
    static constexpr int32 NumPlayers = 4;
    static constexpr int32 StartingHandSize = 13;
    static constexpr int32 DeadWallSize = 14;

    bool bAsyncDiscardPending = false;
    int32 StepPendingNextPlayer = -1;
};
