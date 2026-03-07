// Fill out your copyright notice in the Description page of Project Settings.


#include "HandAnalyzer.h"

FTileData UShantenEvaluator::GetBestDiscard(const TArray<FTileData>& Hand)
{
    TMap<FTileData, int32> DiscardResults = FShantenCalculator::CalculateDiscardResult(Hand);

    FTileData BestDiscard;
    int32 BestShanten = INT32_MAX;

    for (const auto& Pair : DiscardResults)
    {
        if (Pair.Value < BestShanten)
        {
            BestShanten = Pair.Value;
            BestDiscard = Pair.Key;
        }
    }

    return BestDiscard;
}
