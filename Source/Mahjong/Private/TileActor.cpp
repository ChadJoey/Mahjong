// Fill out your copyright notice in the Description page of Project Settings.


#include "TileActor.h"

// Sets default values
ATileActor::ATileActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	TileFront = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileFront"));
	RootComponent = TileFront;

	// Adjust Z to match your asset's thickness.
	//TileFront->SetRelativeLocation(FVector(0.f, 0.f, 2.f));

}

void ATileActor::InitTile(const FTileData& InTileData,UStaticMesh* InFrontMesh,UMaterialInterface* InFrontMaterial)
{
	TileData = InTileData;

	if (InFrontMesh)
		TileFront->SetStaticMesh(InFrontMesh);

	if (InFrontMaterial)
		TileFront->SetMaterial(0, InFrontMaterial);
}

void ATileActor::SetFaceUp(bool bFaceUp)
{
	FRotator Rot = GetActorRotation();
	Rot.Roll = bFaceUp ? .0f : 180.f;
	SetActorRotation(Rot);
}

void ATileActor::MoveTo(FTransform TargetTransform, float Duration)
{
	MoveStart = GetActorTransform();
	MoveTarget = TargetTransform;
	MoveDuration = FMath::Max(Duration, 0.01f);
	MoveElapsed = 0.f;
	bIsMoving = true;
}

// Called when the game starts or when spawned
void ATileActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATileActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsMoving) return;

	MoveElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(MoveElapsed / MoveDuration, 0.f, 1.f);
	// Smooth-step easing
	const float T = Alpha * Alpha * (3.f - 2.f * Alpha);

	SetActorTransform(FTransform(
		FQuat::Slerp(MoveStart.GetRotation(), MoveTarget.GetRotation(), T),
		FMath::Lerp(MoveStart.GetLocation(), MoveTarget.GetLocation(), T),
		FMath::Lerp(MoveStart.GetScale3D(), MoveTarget.GetScale3D(), T)));

	if (Alpha >= 1.f)
	{
		bIsMoving = false;
		OnMoveComplete.Broadcast();
	}
}

