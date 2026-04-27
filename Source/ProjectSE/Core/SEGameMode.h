#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Data/SEGameTypes.h"
#include "SEGameMode.generated.h"

/**
 * 위험구역 전용 GameMode
 * AGameModeBase는 레벨이 로드될 때 생성, 레벨이 언로드될 때 파괴
 */
UCLASS()
class PROJECTSE_API ASEGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ASEGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// --------------------------------------------------------------
	// 매치 상태 관리
	// --------------------------------------------------------------
	/** 현재 매치 상태 조회 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Match")
	EMatchState GetMatchState() const;

	UFUNCTION(BlueprintCallable, Category = "Match")
	void SetMatchState(EMatchState NewState);

	/** 매치 시작 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void StartMatch();

	/** 탈출 요청 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void RequestExtraction(AActor* Player, FVector ExtractionLocation);

	/** 탈출 완료 처리 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void CompleteExtraction();

	/** 탈출 실패 처리 */
	UFUNCTION(BlueprintCallable, Category = "Match")
	void FailExtraction();

	
	// --------------------------------------------------------------
	// 타이머
	// --------------------------------------------------------------
	/** 남은 시간 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Timer")
	float GetRemainingTime() const { return RemainingTime; }

protected:
	/** 위험구역 제한 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match|Timer",
		meta = (DisplayName = "제한 시간 (초)", ClampMin = "30.0"))
	float MatchTimeLimit = 600.f;

	/** 탈출 프로세스 소요 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match|Extraction",
		meta = (DisplayName = "탈출 소요 시간 (초)", ClampMin = "1.0"))
	float ExtractionDuration = 10.f;

	/** GMS 타이머 브로드캐스트 주기 — 매 프레임 전송은 낭비이므로 주기를 둠 */
	UPROPERTY(EditDefaultsOnly, Category = "Match|Timer",
		meta = (DisplayName = "타이머 브로드캐스트 간격 (초)", ClampMin = "0.1"))
	float TimerBroadcastInterval = 1.f;

private:
	/** 현재 매치 상태 */
	EMatchState CurrentMatchState = EMatchState::WaitingToStart;

	/** 남은 제한 시간 */
	float RemainingTime = 0.f;

	/** 탈출 프로세스 남은 시간 */
	float ExtractionRemainingTime = 0.f;

	/** 타이머 브로드캐스트 누적 시간 */
	float TimerBroadcastAccumulator = 0.f;

	/** 탈출 요청 시 저장되는 위치 정보 */
	FVector CachedExtractionLocation = FVector::ZeroVector;

	/** 탈출 요청 플레이어 */
	TWeakObjectPtr<AActor> CachedExtractionPlayer;

	/** 타이머 틱 — InProgress 상태에서 매 프레임 호출 */
	void TickTimer(float DeltaSeconds);

	/** 탈출 프로세스 틱 — Extracting 상태에서 매 프레임 호출 */
	void TickExtraction(float DeltaSeconds);

	/** 시간 초과 처리 */
	void OnTimeExpired();
};
