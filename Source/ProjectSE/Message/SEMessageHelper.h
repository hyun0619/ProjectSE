#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/SEGameTypes.h"
#include "SEMessageHelper.generated.h"

/**
 * GMS 브로드캐스트 헬퍼 - 보일러플레이트 코드 제거
 *
 * [역할]
 * UGameplayMessageSubsystem::Get(Context).BroadcastMessage(...)
 * 호출에 필요한 보일러플레이트를 한 줄짜리 static 함수로 래핑한다.
 *
 * [사용 예시]
 * // 기존 (보일러플레이트):
 * UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(this);
 * FMatchStateMessage Msg;
 * Msg.OldState = OldState;
 * Msg.NewState = NewState;
 * Subsystem.BroadcastMessage(TAG_Message_Match_StateChanged, Msg);
 *
 * // 헬퍼 사용:
 * UExtractionMessageHelper::BroadcastMatchState(this, OldState, NewState);
 *
 * [특징]
 * - Blueprint에서도 호출 가능 (UBlueprintFunctionLibrary 상속)
 * - 채널 태그와 구조체 생성 로직 통합 관리
 */
UCLASS()
class PROJECTSE_API USEMessageHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	// --------------------------------------------------------------
	// 매치 상태
	// 채널: Message.Match.StateChanged
	// --------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "GMS|Match", meta = (WorldContext = "Context"))
	static void BroadcastMatchState(
		UObject* Context,
		EMatchState OldState,
		EMatchState NewState);

	
	// --------------------------------------------------------------
	// 타이머
	// 채널: Message.Timer.Updated
	// --------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "GMS|Timer", meta = (WorldContext = "Context"))
	static void BroadcastTimerUpdate(
		UObject* Context,
		float RemainingSeconds,
		float TotalSeconds);

	
	// --------------------------------------------------------------
	// 인벤토리
	// --------------------------------------------------------------
	/** 아이템 추가 브로드캐스트 — 채널: Message.Player.Inventory.ItemAdded */
	UFUNCTION(BlueprintCallable, Category = "GMS|Inventory", meta = (WorldContext = "Context"))
	static void BroadcastInventoryItemAdded(
		UObject* Context,
		FPrimaryAssetId ItemId,
		int32 Quantity,
		int32 SlotIndex);

	/** 아이템 제거 브로드캐스트 — 채널: Message.Player.Inventory.ItemRemoved */
	UFUNCTION(BlueprintCallable, Category = "GMS|Inventory", meta = (WorldContext = "Context"))
	static void BroadcastInventoryItemRemoved(
		UObject* Context,
		FPrimaryAssetId ItemId,
		int32 Quantity,
		int32 SlotIndex);

	
	// --------------------------------------------------------------
	// 소음
	// 채널: Message.Noise.Generated
	// --------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "GMS|Noise", meta = (WorldContext = "Context"))
	static void BroadcastNoise(
		UObject* Context,
		FVector Location,
		float Intensity,
		AActor* Instigator,
		FGameplayTag NoiseTag);

	
	// --------------------------------------------------------------
	// 탈출 프로세스
	// --------------------------------------------------------------
	/** 탈출 시작 브로드캐스트 — 채널: Message.Extraction.Started */
	UFUNCTION(BlueprintCallable, Category = "GMS|Extraction", meta = (WorldContext = "Context"))
	static void BroadcastExtractionStarted(
		UObject* Context,
		FVector Location,
		float Duration,
		AActor* Player);

	/** 탈출 완료 브로드캐스트 — 채널: Message.Extraction.Completed */
	UFUNCTION(BlueprintCallable, Category = "GMS|Extraction", meta = (WorldContext = "Context"))
	static void BroadcastExtractionCompleted(
		UObject* Context,
		FVector Location,
		float Duration,
		AActor* Player);

	/** 탈출 실패 브로드캐스트 — 채널: Message.Extraction.Failed */
	UFUNCTION(BlueprintCallable, Category = "GMS|Extraction", meta = (WorldContext = "Context"))
	static void BroadcastExtractionFailed(
		UObject* Context,
		FVector Location,
		float Duration,
		AActor* Player);

	// --------------------------------------------------------------
	// 배터리 (2단계 GAS 이관 전 임시)
	// --------------------------------------------------------------
	/** 배터리 상태 변경 브로드캐스트 */
	UFUNCTION(BlueprintCallable, Category = "GMS|Battery", meta = (WorldContext = "Context"))
	static void BroadcastBatteryState(
		UObject* Context,
		EBatteryState NewState,
		float CurrentValue,
		float MaxValue);

	/** 배터리 수치 변경 브로드캐스트 */
	UFUNCTION(BlueprintCallable, Category = "GMS|Battery", meta = (WorldContext = "Context"))
	static void BroadcastBatteryValue(
		UObject* Context,
		float CurrentValue,
		float MaxValue,
		float Delta);

	
	// --------------------------------------------------------------
	// 체력 (2단계 GAS 이관 전 임시)
	// --------------------------------------------------------------
	/** 체력 변경 브로드캐스트 */
	UFUNCTION(BlueprintCallable, Category = "GMS|Health",
		meta = (WorldContext = "Context"))
	static void BroadcastHealthChanged(
		UObject* Context,
		float CurrentHealth,
		float MaxHealth,
		float Delta,
		AActor* Instigator);
};
