#include "SEGameMode.h"
#include "Message/SEGameMessages.h"
#include "Message/SEMessageHelper.h"


ASEGameMode::ASEGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASEGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	RemainingTime = MatchTimeLimit; // 제한 시간 초기화

	UE_LOG(LogTemp, Log, TEXT("[ExtractionGM] BeginPlay — 제한시간 %.0f초, 매치 상태: WaitingToStart"),
		MatchTimeLimit);
}

void ASEGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	switch (CurrentMatchState)
	{
	case EMatchState::InProgress:
		TickTimer(DeltaSeconds);
		break;

	case EMatchState::Extracting:
		TickExtraction(DeltaSeconds);
		break;

	default:
		break;
	}
}

// --------------------------------------------------------------
// 매치 상태 관리
// --------------------------------------------------------------
EMatchState ASEGameMode::GetMatchState() const
{
	return CurrentMatchState;
}

void ASEGameMode::SetMatchState(EMatchState NewState)
{
	if (CurrentMatchState == NewState) return;

	const EMatchState OldState = CurrentMatchState;
	CurrentMatchState = NewState;

	// GMS로 상태 전이를 브로드캐스트
	USEMessageHelper::BroadcastMatchState(this, OldState, NewState);

	UE_LOG(LogTemp, Log, TEXT("[ExtractionGM] 매치 상태 전이: %d → %d"),
		static_cast<int32>(OldState), static_cast<int32>(NewState));
}

void ASEGameMode::StartMatch()
{
	if (CurrentMatchState != EMatchState::WaitingToStart)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ExtractionGM] StartMatch 호출되었으나 현재 상태가 WaitingToStart가 아님 (%d)"),
			static_cast<int32>(CurrentMatchState));
		return;
	}

	RemainingTime = MatchTimeLimit;
	TimerBroadcastAccumulator = 0.f;
	SetMatchState(EMatchState::InProgress);
}

void ASEGameMode::RequestExtraction(AActor* Player, FVector ExtractionLocation)
{
	if (CurrentMatchState != EMatchState::InProgress)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ExtractionGM] RequestExtraction — 진행 중 상태가 아님"));
		return;
	}

	CachedExtractionPlayer = Player;
	CachedExtractionLocation = ExtractionLocation;
	ExtractionRemainingTime = ExtractionDuration;

	SetMatchState(EMatchState::Extracting);

	// 탈출 프로세스 시작을 GMS로 알림
	USEMessageHelper::BroadcastExtractionStarted(
		this, ExtractionLocation, ExtractionDuration, Player);
}

void ASEGameMode::CompleteExtraction()
{
	if (CurrentMatchState != EMatchState::Extracting) return;

	SetMatchState(EMatchState::Succeeded);

	// 탈출 완료를 GMS로 알림
	USEMessageHelper::BroadcastExtractionCompleted(
		this, CachedExtractionLocation, ExtractionDuration,
		CachedExtractionPlayer.Get());
}

void ASEGameMode::FailExtraction()
{
	SetMatchState(EMatchState::Failed);

	// 탈출 실패를 GMS로 알림
	USEMessageHelper::BroadcastExtractionFailed(
		this, CachedExtractionLocation, ExtractionDuration,
		CachedExtractionPlayer.Get());
}

// --------------------------------------------------------------
// 타이머 틱
// --------------------------------------------------------------

void ASEGameMode::TickTimer(float DeltaSeconds)
{
	RemainingTime -= DeltaSeconds;

	// 타이머 브로드캐스트
	TimerBroadcastAccumulator += DeltaSeconds;
	if (TimerBroadcastAccumulator >= TimerBroadcastInterval)
	{
		TimerBroadcastAccumulator -= TimerBroadcastInterval;
		USEMessageHelper::BroadcastTimerUpdate(
			this, FMath::Max(0.f, RemainingTime), MatchTimeLimit);
	}

	// 시간 초과 판정
	if (RemainingTime <= 0.f)
	{
		RemainingTime = 0.f;
		OnTimeExpired();
	}
}

void ASEGameMode::TickExtraction(float DeltaSeconds)
{
	// 탈출 중에도 전체 제한 시간은 계속 흐름
	TickTimer(DeltaSeconds);

	ExtractionRemainingTime -= DeltaSeconds;
	if (ExtractionRemainingTime <= 0.f)
	{
		ExtractionRemainingTime = 0.f;
		CompleteExtraction();
	}
}

void ASEGameMode::OnTimeExpired()
{
	UE_LOG(LogTemp, Log, TEXT("[ExtractionGM] 제한 시간 초과 — 탈출 실패"));
	FailExtraction();
}