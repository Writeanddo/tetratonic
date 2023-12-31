// Fill out your copyright notice in the Description page of Project Settings.


#include "EntitySpawner.h"

#include "TrackGameMode.h"

// Sets default values
AEntitySpawner::AEntitySpawner()
{
 	// Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

// Called when the game starts or when spawned
void AEntitySpawner::BeginPlay()
{
	Super::BeginPlay();

	EntitySpawns.Sort([](const FEntitySpawnParameters& Entity1, const FEntitySpawnParameters& Entity2)
	{
		return Entity1.TargetBeat > Entity2.TargetBeat;
	});
}

// Called every frame
void AEntitySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEntitySpawner::SpawnEntities(int32 CurrentBeat)
{
	int32 NumSpawns = EntitySpawns.Num();
	while (NumSpawns > 0 && EntitySpawns[NumSpawns - 1].TargetBeat <= CurrentBeat + SpawnBeatOffset)
	{
		const auto& [EntityType, EntityDirection, TargetPosition, TargetBeat, NumBeats] = EntitySpawns.Pop();
		NumSpawns--;

		SpawnEntity(
			(EntityType == EEntityType::PickupEntity) ? PickupClass : AdversaryClass,
			EntityDirection,
			TargetPosition,
			TargetBeat,
			NumBeats,
			EntitySpeed
			);
	}
}

TArray<FEntitySpawnParameters> AEntitySpawner::GetEntitiesToSpawn(int32 CurrentBeat)
{
	TArray<FEntitySpawnParameters> EntitiesToSpawn = TArray<FEntitySpawnParameters>();
	TArray<FEntitySpawnParameters> TempEntitySpawns = TArray<FEntitySpawnParameters>();
	for (auto& Entity : EntitySpawns)
	{
		TempEntitySpawns.Add(Entity);
	}
	TempEntitySpawns.Sort();
	
	int32 SpawnIndex = 0;
	while (SpawnIndex < EntitySpawns.Num() && EntitySpawns[SpawnIndex].TargetBeat <= CurrentBeat + SpawnBeatOffset)
	{
		EntitiesToSpawn.Add(EntitySpawns[SpawnIndex]);
		SpawnIndex++;
	}

	return EntitiesToSpawn;
}

void AEntitySpawner::SortEntities()
{
	EntitySpawns.Sort();
}


