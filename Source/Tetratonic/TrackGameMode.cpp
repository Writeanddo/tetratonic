// Fill out your copyright notice in the Description page of Project Settings.


#include "TrackGameMode.h"

#include "EntitySpawner.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Quartz/AudioMixerClockHandle.h"

void ATrackGameMode::StartPlay()
{
	UE_LOG(LogTemp, Display, TEXT("Game mode initializing Quartz clock"));
	const UWorld* World = GetWorld();
	UQuartzSubsystem* QuartzSubsystem = UQuartzSubsystem::Get(World);

	FQuartzClockSettings ClockSettings = FQuartzClockSettings();
	ClockSettings.TimeSignature.NumBeats = NumBeats;
	ClockSettings.TimeSignature.BeatType = BeatType;

	FQuartzQuantizationBoundary QuantizationBoundary = FQuartzQuantizationBoundary();
	QuantizationBoundary.Quantization = EQuartzCommandQuantization::Beat;
	QuantizationBoundary.CountingReferencePoint = EQuarztQuantizationReference::BarRelative;
	
	QuartzClock = QuartzSubsystem->CreateNewClock(this, FName("ConductorClock"), ClockSettings);
	QuartzClock->SetBeatsPerMinute(World, QuantizationBoundary, FOnQuartzCommandEventBP(), QuartzClock, BeatsPerMinute);

	TArray<AActor*> EntityActors = TArray<AActor*>();
	UGameplayStatics::GetAllActorsOfClass(World, AExtendableEntity::StaticClass(), EntityActors);

	for (const auto& Entity : EntityActors)
	{
		Entity->Destroy();
	}
	
	Super::StartPlay();
}

void ATrackGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	if (StartDelay > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_StartDelay, this, &ATrackGameMode::PlayAudioTrack, StartDelay);
	}
	else
	{
		PlayAudioTrack();
	}
}

void ATrackGameMode::PlayAudioTrack()
{
	if (!AudioTrack)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayAudioTrack invoked on game mode, but no audio track was set."));
		return;
	}
	if (!QuartzClock)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayAudioTrack invoked on game mode, but Quartz Clock is not initialized."));
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("Got command to play audio track: %s"), *AudioTrack->GetName());
	
	const UWorld* World = GetWorld();

	AudioComponent = UGameplayStatics::CreateSound2D(World, AudioTrack);
	AudioComponent->bOverridePriority = true;
	AudioComponent->Priority = 10;

	FQuartzQuantizationBoundary TrackBoundary = FQuartzQuantizationBoundary();
	TrackBoundary.Quantization = EQuartzCommandQuantization::Bar;
	TrackBoundary.CountingReferencePoint = EQuarztQuantizationReference::CurrentTimeRelative;

	FOnQuartzCommandEventBP CommandEvent = FOnQuartzCommandEventBP();
	CommandEvent.BindUFunction(this, "OnAudioComponentQuantized");
	
	AActor* FoundEntitySpawner = UGameplayStatics::GetActorOfClass(World, AEntitySpawner::StaticClass());
	if (FoundEntitySpawner)
	{
		const AEntitySpawner* EntitySpawnerActor = Cast<AEntitySpawner>(FoundEntitySpawner);
		this->StartBeat = EntitySpawnerActor->StartBeat;
	}
	
	StartTime = (60 / BeatsPerMinute) * (StartBeat - 1);
	UE_LOG(LogTemp, Display, TEXT("Starting audio track at offset of: %f"), StartTime);
	
	AudioComponent->PlayQuantized(World, QuartzClock, TrackBoundary, CommandEvent, StartTime, FadeInDuration, FadeVolumeLevel, FaderCurve);

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_StartDelay);
}

void ATrackGameMode::OnAudioComponentQuantized(EQuartzCommandDelegateSubType CommandType, FName)
{
	if (CommandType == EQuartzCommandDelegateSubType::CommandOnQueued)
	{
		QuartzClock->StartClock(GetWorld(), QuartzClock);
	}
}

void ATrackGameMode::SetPaused(bool bPaused)
{
	const UWorld* World = GetWorld();
	UGameplayStatics::SetGamePaused(World, bPaused);
	
	if (QuartzClock)
	{
		if (bPaused)
		{
			QuartzClock->PauseClock(World, QuartzClock);
		}
		else
		{
			QuartzClock->StartClock(World, QuartzClock);
		}
	}
	if (AudioComponent)
	{
		AudioComponent->SetPaused(bPaused);
	}
}

void ATrackGameMode::GetCorrectedTimestamp(int32& Bars, int32& Beat, float& BeatFraction, float& Seconds) const
{
	const FQuartzTransportTimeStamp CurrentTimestamp = QuartzClock->GetCurrentTimestamp(GetWorld());

	const int32 AdjustedBeat = CurrentTimestamp.Beat + StartBeat - 2;
	Bars = CurrentTimestamp.Bars + (AdjustedBeat / 4);
	Beat = AdjustedBeat % 4 + 1;
	BeatFraction = CurrentTimestamp.BeatFraction;
	Seconds = CurrentTimestamp.Seconds + StartTime;
}


