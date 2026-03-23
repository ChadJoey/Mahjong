// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TileTypes.h"
#include "ShantenTestActor.generated.h"

UCLASS()
class MAHJONG_API AShantenTestActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AShantenTestActor();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanten Test")
	TArray<FTileData> TestHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanten Test")
	FTileData DrawnTile;

	UFUNCTION(BluePrintCallable, Category = "Shanten Test")
	void RunShantenTest();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
