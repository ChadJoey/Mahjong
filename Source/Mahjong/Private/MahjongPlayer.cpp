// Fill out your copyright notice in the Description page of Project Settings.


#include "MahjongPlayer.h"
#include "MahjongGameStateBase.h"
#include <MahjongGameMode.h>

AMahjongPlayer::AMahjongPlayer()
{
	Difficulty = EAIDifficulty::Hard;
	ThinkingTime = 1.0;
}

FTileData AMahjongPlayer::DecideDiscard()
{
	switch (Difficulty)
	{
	case EAIDifficulty::Easy:
		return DecideDiscardEasy();
		break;
	case EAIDifficulty::Medium:
		return DecideDiscardMedium();
		break;
	case EAIDifficulty::Hard:
		//return DecideDiscardHard();
		break;
	default:
		break;
	}
	return FTileData();
}

bool AMahjongPlayer::ShouldCallChi(const FTileData& DiscardedTile)
{
	// Implement chi logic based on difficulty
	if (Difficulty == EAIDifficulty::Easy)
		return false; // Easy AI doesn't call chi

	// Check if chi would improve shanten
	// TODO: Implement
	return false;
}

bool AMahjongPlayer::ShouldCallPon(const FTileData& DiscardedTile)
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState) return false;

	// Count how many of this tile we have
	TArray<FTileData> Hand = MyState->GetHand();
	int32 Count = 0;
	for (const FTileData& Tile : Hand)
	{
		if (Tile == DiscardedTile)
			Count++;
	}

	// Can pon if we have 2+ of this tile
	if (Count < 2)
		return false;

	// Medium+ AI: check if pon improves hand
	if (Difficulty >= EAIDifficulty::Medium)
	{
		// Calculate shanten before and after pon
		// TODO: Implement proper evaluation
	}
	return false;
	//return Difficulty >= EAIDifficulty::Medium;
}

bool AMahjongPlayer::ShouldCallKan(const FTileData& DiscardedTile)
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState) return false;

	// Count how many of this tile we have
	TArray<FTileData> Hand = MyState->GetHand();
	int32 Count = 0;
	for (const FTileData& Tile : Hand)
	{
		if (Tile == DiscardedTile)
			Count++;
	}

	// Can pon if we have 2+ of this tile
	if (Count < 3)
		return false;

	// Medium+ AI: check if pon improves hand
	if (Difficulty >= EAIDifficulty::Medium)
	{
		// Calculate shanten before and after pon
		// TODO: Implement proper evaluation
	}
	return false;
	//return Difficulty >= EAIDifficulty::Medium;
}

bool AMahjongPlayer::ShouldDeclareRiichi()
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState) return false;

	TArray<FTileData> Hand = MyState->GetHand();
	TArray<uint8> Tiles34 = FShantenCalculator::ToTile34Array(Hand);
	int32 Shanten = FShantenCalculator::Calculate(Tiles34);
	//can only declare riichi if we are in tenpai (shanten = 0)
	if (Shanten != 0)
	{
		return false;
	}

	// Medium AI always declares when possible
	if (Difficulty == EAIDifficulty::Medium)
		return true;

	// Hard AI considers point situation and tile safety
	// TODO: Implement more sophisticated logic
	return true;
}

bool AMahjongPlayer::ShouldDeclareRon(const FTileData& DiscardedTile)
{
	// Check if adding this tile completes the hand
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState) return false;

	TArray<FTileData> TestHand = MyState->GetHand();
	TestHand.Add(DiscardedTile);

	TArray<uint8> Tiles34 = FShantenCalculator::ToTile34Array(TestHand);
	int32 Shanten = FShantenCalculator::Calculate(Tiles34);

	// Shanten = -1 means winning hand
	return (Shanten == -1);
}

bool AMahjongPlayer::ShouldDeclareTsumo()
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState) return false;

	TArray<FTileData> Hand = MyState->GetHand();
	TArray<uint8> Tiles34 = FShantenCalculator::ToTile34Array(Hand);
	int32 Shanten = FShantenCalculator::Calculate(Tiles34);

	return (Shanten == -1);
}

void AMahjongPlayer::BeginPlay()
{
	Super::BeginPlay();
	switch (Difficulty)
	{
	case EAIDifficulty::Easy:
		break;
	case EAIDifficulty::Medium:
		HandEvaluator = NewObject<UShantenEvaluator>(this);
		break;
	case EAIDifficulty::Hard:
		HandEvaluator = NewObject<UShantenEvaluator>(this);
		break;
	default:
		break;
	}

}

void AMahjongPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FMonteCarloInput AMahjongPlayer::BuildMonteCarloInput() const
{
	FMonteCarloInput Input;
	Input.MyPlayerIndex = (int32)MahjongPlayerState->GetSeat(); // seat index
	Input.NumPlayers = 4;

	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState) return Input;

	Input.MyHand34 = FShantenCalculator::ToTile34Array(MyState->GetHand());

	Input.SeenTiles34.Init(0,34);


	if (AMahjongGameStateBase* GS = GetWorld()->GetGameState<AMahjongGameStateBase>())
	{
		TArray<FTileData> Visible = GS->GetVisibleTiles();
		TArray<uint8> VisibleArr = FShantenCalculator::ToTile34Array(Visible);
		for (int32 i = 0; i < 34; ++i)
		{
			Input.SeenTiles34[i] += VisibleArr[i];
		}
	}


	for (APlayerState* PS : GetWorld()->GetGameState()->PlayerArray)
	{
		AMahjongPlayerState* MPS = Cast<AMahjongPlayerState>(PS);
		if (!MPS) continue;
		Input.OpponentMeldCounts.Add(MPS->GetRevealedMelds().Num());
	}

	if (AMahjongGameMode* GM = GetWorld()->GetAuthGameMode<AMahjongGameMode>())
	{
		Input.WallRemaining = GM->GetWAllTilesRemaining();
		
	}
	else
	{
		Input.WallRemaining = 70;
	}

	return Input;
}

AMahjongPlayerState* AMahjongPlayer::GetMahjongPlayerState() const
{
	return MahjongPlayerState;
}

FTileData AMahjongPlayer::DecideDiscardEasy()
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState || MyState->GetHandSize() == 0)
	{
		return FTileData();
	}

	TArray<FTileData> Hand = MyState->GetHand();
	int32 RandomIndex = FMath::RandRange(0, Hand.Num() - 1);
	return Hand[RandomIndex];
}

FTileData AMahjongPlayer::DecideDiscardMedium()
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState) return FTileData{};

	if (!HandEvaluator)
	{
		HandEvaluator = NewObject<UShantenEvaluator>(this);
	}

	TArray<FTileData> Hand = MyState->GetHand();
	if (Hand.Num() == 0) return FTileData{};

	return HandEvaluator->GetBestDiscard(Hand);
}

FTileData AMahjongPlayer::DecideDiscardHard()
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState || !HandEvaluator) return FTileData{};
	
	TArray<FTileData> Hand = MyState->GetHand();
	if (Hand.Num() == 0) return FTileData{};

	FMonteCarloInput Input = BuildMonteCarloInput();

	if (Input.MyHand34.Num() != 34) return HandEvaluator->GetBestDiscard(Hand);

	TArray<FMonteCarloResult> Results = FMahjongMonteCarloSimulator::EvaluateDiscards(Input, 50);

	if (Results.Num() == 0) return HandEvaluator->GetBestDiscard(Hand);

	const FMonteCarloResult& Best = Results[0];

	UE_LOG(LogTemp, Log,
		TEXT("[Hard AI] Discarding %s | WinRate=%.1f%% | AvgTurns=%.1f | Shanten=%d"),
		*Best.Discard.ToString(),
		Best.Winrate * 100.f,
		Best.AvgTurnsToWin,
		Best.AvgFinalShanten);

	return Best.Discard;

}


void AMahjongPlayer::DecideDiscardAsync(TFunction<void(FTileData)> OnDecided)
{
	// Easy / Medium: synchronous, callback fires immediately
	if (Difficulty != EAIDifficulty::Hard)
	{
		OnDecided(DecideDiscard());
		return;
	}

	// Hard: build MC input here on the game thread, then run off it
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState || MyState->GetHandSize() == 0 || !HandEvaluator)
	{
		OnDecided(FTileData{});
		return;
	}

	TArray<FTileData> Hand = MyState->GetHand();
	FMonteCarloInput Input = BuildMonteCarloInput();

	if (Input.MyHand34.Num() != 34)
	{
		OnDecided(HandEvaluator->GetBestDiscard(Hand));
		return;
	}

	FMahjongMonteCarloSimulator::EvaluateDiscardsAsync(
		MoveTemp(Input), Simulations,
		[this, OnDecided = MoveTemp(OnDecided)](TArray<FMonteCarloResult> Results)
		{
			AMahjongPlayerState* State = GetMahjongPlayerState();
			if (Results.IsEmpty())
			{
				bHasLastMCResult = false;
				OnDecided(HandEvaluator && State
					? HandEvaluator->GetBestDiscard(State->GetHand())
					: FTileData{});
				return;
			}

			// Store for UI before firing the callback
			LastMCResult = Results[0];
			bHasLastMCResult = true;

			UE_LOG(LogTemp, Log,
				TEXT("[Hard AI] Discard %s | WR=%.1f%% | AvgTurns=%.1f | Shanten=%d"),
				*Results[0].Discard.ToString(),
				Results[0].Winrate * 100.f,
				Results[0].AvgTurnsToWin,
				Results[0].AvgFinalShanten);

			OnDecided(Results[0].Discard);
		});
}

float AMahjongPlayer::EvaluateHandMonteCarlo(const TArray<FTileData>& Hand, int32 NumSimulations)
{
	return 0.0f;
}
