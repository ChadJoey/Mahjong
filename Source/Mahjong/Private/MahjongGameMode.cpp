// Fill out your copyright notice in the Description page of Project Settings.


#include "MahjongGameMode.h"
#include "MahjongGameStateBase.h"
#include <EngineUtils.h>

AMahjongGameMode::AMahjongGameMode()
{
}


void AMahjongGameMode::StartGame()
{
    Players.Empty();
    for (AMahjongPlayer* Player : TActorRange<AMahjongPlayer>(GetWorld()))
    {
        Players.Add(Player);
    }
    if (Players.Num() != NumPlayers)
    {
        UE_LOG(LogTemp, Error, TEXT("AMahjongGameMode: expected %d players, found %d"), NumPlayers, Players.Num());
        return;
    }
    CurrentPlayerIndex = 0;
    StartRound();
}

void AMahjongGameMode::OnPlayerDiscard(int32 PlayerIndex, const FTileData& DiscardedTile)
{
    if (CurrentPhase != EGamePhase::PlayerAction)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnPlayerDiscard called outside PlayerAction phase"));
        return;
    }
    if (PlayerIndex != CurrentPlayerIndex)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnPlayerDiscard: wrong player (%d vs %d)"), PlayerIndex, CurrentPlayerIndex);
        return;
    }

    if (AMahjongPlayerState* PS = GetPlayerStateAt(PlayerIndex))
    {
        PS->RemoveTileFromHand(DiscardedTile);
    }

    OnTileDiscarded.Broadcast(PlayerIndex, DiscardedTile);
    BeginResponseWindow(DiscardedTile, PlayerIndex);

}

void AMahjongGameMode::SubmitCall(int32 PlayerIndex, ECallType CallType)
{
    if (CurrentPhase != EGamePhase::ResponseWindow) return;
    FPendingCall Call;
    Call.PlayerIndex = PlayerIndex;
    Call.Type = CallType;
    PendingCalls.Add(Call);
    PlayerHasResponded[PlayerIndex] = true;


    if (CallType == ECallType::Ron)
    {
        GetWorldTimerManager().ClearTimer(ResponseWindowTimerHandle);
        ResolveResponseWindow();
        return;
    }

    CheckAllPlayersResponded();
}

void AMahjongGameMode::SubmitPass(int32 PlayerIndex)
{
    if (CurrentPhase != EGamePhase::ResponseWindow) return;
    PlayerHasResponded[PlayerIndex] = true;
    CheckAllPlayersResponded();
}


void AMahjongGameMode::BeginPlay()
{
    Super::BeginPlay();
    // Initialize shanten calculator tables (do this ONCE at game start)
    if (!FShantenCalculator::IsInitialized())
    {
        FShantenCalculator::InitializeTables();
    }
}

void AMahjongGameMode::TransitionToPhase(EGamePhase NewPhase)
{
    const EGamePhase Old = CurrentPhase;
    CurrentPhase = NewPhase;
    UE_LOG(LogTemp, Log, TEXT("Phase: %d -> %d"), (int32)Old, (int32)NewPhase);
    OnPhaseChanged.Broadcast(Old, NewPhase);
}

void AMahjongGameMode::StartRound()
{
    TransitionToPhase(EGamePhase::Dealing);
    for (AMahjongPlayer* P : Players)
    {
        if (AMahjongPlayerState* PS = P->GetMahjongPlayerState())
        {
            //TODO expose clearHand on mahjongPlayerSTate
            PS->GetHand().Empty();
            PS->bIsRiichi = false;
        }
    }

    BuildAndShuffleWall();

    for (int32 t = 0; t < StartingHandSize; ++t)
    {
        for (int32 p = 0; p < NumPlayers; ++p)
        {
            if (AMahjongPlayerState* PS = GetPlayerStateAt(p))
            {
                PS->AddTileToHand(DrawFromWall());
            }
        }
    }
    for (AMahjongPlayer* P : Players)
    {
        if (AMahjongPlayerState* PS = P->GetMahjongPlayerState())
        {
            PS->SortHand();
        }
    }
    
    BeginPlayerTurn(CurrentPlayerIndex);
}

void AMahjongGameMode::BeginPlayerTurn(int32 PlayerIndex)
{
    if (IsWallEmpty())
    {
        EndRound(-1, false);
        return;
    }

    CurrentPlayerIndex = PlayerIndex;
    TransitionToPhase(EGamePhase::PlayerDraw);

    FTileData Drawn = DrawFromWall();
    if (AMahjongPlayerState* PS = GetPlayerStateAt(PlayerIndex))
    {
        PS->AddTileToHand(Drawn);
        //may not wanna sort hand to show clearly which tile had been drawn
        //PS->SortHand();
    }

    TransitionToPhase(EGamePhase::PlayerAction);

    if (IsHumanPlayer(PlayerIndex))
    {
        return;
    }

    ScheduleAIDiscard(PlayerIndex);
}

void AMahjongGameMode::BeginResponseWindow(const FTileData& DiscardedTile, int32 DiscardingPlayerIndex)
{
    TransitionToPhase(EGamePhase::ResponseWindow);

    LastDiscardedTile = DiscardedTile;
    LastDiscardingPlayerIndex = DiscardingPlayerIndex;

    PlayerHasResponded.Init(false, NumPlayers);
    PlayerHasResponded[DiscardingPlayerIndex] = true;

    PendingCalls.Empty();
    CallDecisionTimerHandles.Empty();

    for (int32 i = 0; i < NumPlayers; ++i)
    {
        if (i == DiscardingPlayerIndex) continue;

        if (!IsHumanPlayer(i)) ScheduleAICallDecision(i);
    }

    FTimerDelegate FallBackDel;
    FallBackDel.BindUObject(this, &AMahjongGameMode::ResolveResponseWindow);
    GetWorldTimerManager().SetTimer(ResponseWindowTimerHandle, FallBackDel, ResponseWindowDuration, false);
}

void AMahjongGameMode::ResolveResponseWindow()
{
    if (CurrentPhase != EGamePhase::ResponseWindow) return;
    TransitionToPhase(EGamePhase::ProcessingCall);

    FPendingCall Best;
    for (const FPendingCall& Call : PendingCalls)
    {
        if (Call.GetPriority() > Best.GetPriority())
        {
            Best = Call;
        }
    }
    
    if (Best.Type == ECallType::None)
    {
        BeginPlayerTurn(GetNextPlayerIndex(LastDiscardingPlayerIndex));
        return;
    }
    if (Best.Type == ECallType::Ron)
    {
        EndRound(Best.PlayerIndex, false);
        return;
    }

    ApplyCall(Best, LastDiscardedTile);
}

void AMahjongGameMode::ApplyCall(const FPendingCall& Call, const FTileData& ClaimedTile)
{
    AMahjongPlayerState* PS = GetPlayerStateAt(Call.PlayerIndex);
    if (!PS) return;

    //Kan consumes 3 hand tiles + the claimed tile, Pon/Chi consume 2 + the claimed tile
    const int32 HandTilesNeeded = (Call.Type == ECallType::Kan) ? 3 : 2;

    FMeldData Meld;
    Meld.Tiles.Add(ClaimedTile);

    //remove matching tiles from hand to complete meld (requires to be stored somewhere so they cant be discarded but can be used for shanten)
    //TODO: Chi needs tile selection in some cases
    int32 Removed = 0;
    TArray<FTileData> Hand = PS->GetHand();
    for (int32 i = 0; i < Hand.Num() && Removed < HandTilesNeeded; ++i)
    {
        if (Hand[i] == ClaimedTile)
        {
            PS->RemoveTileFromHand(Hand[i]);
            Meld.Tiles.Add(Hand[i]);
            ++Removed;
        }
    }


    PS->AddRevealedMeld(Meld);

    if (Call.Type == ECallType::Kan && !IsWallEmpty())
    {
        //TODO: implement Dead wall / Draw From dead Wall
        PS->AddTileToHand(DrawFromWall());
        //TODO: implement Dora
    }

    CurrentPlayerIndex = Call.PlayerIndex;
    TransitionToPhase(EGamePhase::PlayerAction);

    if (!IsHumanPlayer(Call.PlayerIndex))
    {
        (
            ScheduleAIDiscard(Call.PlayerIndex));
    }
}

void AMahjongGameMode::EndRound(int32 WinningPlayerIndex, bool bIsTsumo)
{
    GetWorldTimerManager().ClearTimer(AIDiscardTimerHandle);
    GetWorldTimerManager().ClearTimer(ResponseWindowTimerHandle);
    for (FTimerHandle& H : CallDecisionTimerHandles)
    {
        GetWorldTimerManager().ClearTimer(H);
    }

    TransitionToPhase(EGamePhase::RoundEnd);

    if (WinningPlayerIndex >= 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Player %d wins be %s"), WinningPlayerIndex, bIsTsumo ? TEXT("Tsumo") : TEXT("Ron"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Ryuukyoku (no more draws)"))
    }

    OnRoundEnded.Broadcast(WinningPlayerIndex, bIsTsumo);

    //TODO: score calc, dealer rotation, check for game end
    //when done transition to GameEnd()

}

void AMahjongGameMode::ScheduleAIDiscard(int32 PlayerIndex)
{
    FTimerDelegate Del;
    Del.BindLambda([this, PlayerIndex]()
        {
            AMahjongPlayer* P = GetPlayer(PlayerIndex);
            if (!P || CurrentPhase != EGamePhase::PlayerAction) return;

            if (P->ShouldDeclareTsumo())
            {
                EndRound(PlayerIndex, true);
                return;
            }

            if (AMahjongPlayerState* PS = GetPlayerStateAt(PlayerIndex))
            {
                if (!PS->bIsRiichi && P->ShouldDeclareRiichi())
                {
                    PS->bIsRiichi = true;
                }
            }


            OnPlayerDiscard(PlayerIndex, P->DecideDiscard());
        });

    GetWorldTimerManager().SetTimer(AIDiscardTimerHandle, Del, AIThinkingTime, false);
}

void AMahjongGameMode::ScheduleAICallDecision(int32 PlayerIndex)
{
    // Each AI player gets its own timer handle so they don't cancel each other.
    // Stagger decisions slightly so they don't all fire in the same frame.
    FTimerHandle& Handle = CallDecisionTimerHandles.AddDefaulted_GetRef();
    const float Delay = 0.2f + PlayerIndex * 0.1f;

    FTimerDelegate Del;
    Del.BindLambda([this, PlayerIndex]() {

        if (CurrentPhase != EGamePhase::ResponseWindow) return;
        
        AMahjongPlayer* P = GetPlayer(PlayerIndex);
        if (!P) {SubmitPass(PlayerIndex); return;}

        if (P->ShouldDeclareRon(LastDiscardedTile)) SubmitCall(PlayerIndex, ECallType::Ron);
        else if (P->ShouldCallKan(LastDiscardedTile)) SubmitCall(PlayerIndex, ECallType::Kan);
        else if (P->ShouldCallPon(LastDiscardedTile)) SubmitCall(PlayerIndex, ECallType::Pon);
        else if (P->ShouldCallChi (LastDiscardedTile)) SubmitCall(PlayerIndex, ECallType::Chi);
        else SubmitPass(PlayerIndex);
        });

    GetWorldTimerManager().SetTimer(Handle, Del, Delay, false);

}

void AMahjongGameMode::BuildAndShuffleWall()
{
    Wall.Empty();
    WallDrawIndex = 0;

    // 136 tiles: 4 copies × (9 man + 9 pin + 9 sou + 7 honors)
    for (int32 copy = 0; copy < 4; ++copy)
    {
        for (int32 v = 1; v <= 9 ; ++v)
        {
            Wall.Add(FTileData{ ETileSuit::Man, (uint8)v });
            Wall.Add(FTileData{ ETileSuit::Pin, (uint8)v });
            Wall.Add(FTileData{ ETileSuit::Sou, (uint8)v });
        }
        for (int32 v = 1; v <= 7; ++v)
        {
            Wall.Add(FTileData{ ETileSuit::Honor, (uint8)v });
        }
    }

    for (int32 i = Wall.Num(); i > 0; --i)
    {
        Wall.Swap(i, FMath::RandRange(0, 1));
    }
}

FTileData AMahjongGameMode::DrawFromWall()
{
    if (IsWallEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("DrawFromWall: wall exhausted"));
        return FTileData{};
    }
    return Wall[WallDrawIndex++];
}


bool AMahjongGameMode::IsWallEmpty() const
{
    return WallDrawIndex >= Wall.Num() - DeadWallSize;
}

int32 AMahjongGameMode::GetNextPlayerIndex(int32 From) const
{
    return (From + 1) % NumPlayers;
}

AMahjongPlayer* AMahjongGameMode::GetPlayer(int32 Index) const
{
    return Players.IsValidIndex(Index) ? Players[Index] : nullptr;
}

AMahjongPlayerState* AMahjongGameMode::GetPlayerStateAt(int32 Index) const
{
    AMahjongPlayer* P = GetPlayer(Index);
    return P ? P->GetMahjongPlayerState() : nullptr;
}

bool AMahjongGameMode::IsHumanPlayer(int32 Index) const
{
    return Index == 0; //seat 0 is the player
}

void AMahjongGameMode::CheckAllPlayersResponded()
{
    for (bool b : PlayerHasResponded)
    {
        if (!b) return;
    }

    GetWorldTimerManager().ClearTimer(ResponseWindowTimerHandle);
    ResolveResponseWindow();
}
