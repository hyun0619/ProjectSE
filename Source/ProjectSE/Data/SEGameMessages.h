#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "Data/SEGameTypes.h"
#include "SEGameMessages.generated.h"


/**
 * 게임 내 발생하는 이벤트를 전달하기 위한 공동 메시지 구조체 모음
 */


// --------------------------------------------------------------
// GMS 채널 태그 선언
// --------------------------------------------------------------

/** 위험구역 매치 상태 변경 채널 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Match_StateChanged);

/** 배터리 상태 변경 채널 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Player_Battery_StateChanged);
/** 배터리 수치 변경 채널 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Player_Battery_ValueChanged);

/** 플레이어 체력 변경 채널 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Player_Health_Changed);

/** 인벤토리 아이템 추가 채널 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Player_Inventory_ItemAdded);
/** 인벤토리 아이템 제거 채널 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Player_Inventory_ItemRemoved);

/** 소음 발생 채널 — AI 감지 시스템과 연동 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Noise_Generated);

/** 제한 시간 업데이트 채널 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Timer_Updated);

/** 탈출 프로세스 시작 채널 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Extraction_Started);
/** 탈출 프로세스 완료 채널 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Extraction_Completed);
/** 탈출 프로세스 실패 채널 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Extraction_Failed);



// --------------------------------------------------------------
// 매치 상태 변경 메시지
// 채널: Message.Match.StateChanged
// 발신: AExtractionGameMode
// 수신: UI, PlayerState, 사운드 시스템 등
// --------------------------------------------------------------
USTRUCT(BlueprintType)
struct FMatchStateMessage
{
	GENERATED_BODY()

	/** 이전 상태 — 전이 방향 판단용 */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	EMatchState OldState = EMatchState::WaitingToStart;
	/** 새로운 상태 */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	EMatchState NewState = EMatchState::WaitingToStart;
};

// --------------------------------------------------------------
// 배터리 상태 변경 메시지
// 채널: Message.Player.Battery.StateChanged
// 발신: 배터리 관리 컴포넌트 (2단계 GAS AttributeSet 이관 후 변경 예정)
// 수신: UI HUD, 사운드(경고음)
// --------------------------------------------------------------
USTRUCT(BlueprintType)
struct FBatteryStateMessage
{
	GENERATED_BODY()

	/** 새로운 배터리 상태 */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	EBatteryState NewState = EBatteryState::Full;
	/** 현재 배터리 수치 */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float CurrentValue = 100.f;
	/** 최대 배터리 수치 */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float MaxValue = 100.f;
};

// --------------------------------------------------------------
// 배터리 수치 변경 메시지
// 채널: Message.Player.Battery.ValueChanged
// 발신: 배터리 관리 컴포넌트
// 수신: UI HUD (게이지 바)
// --------------------------------------------------------------
USTRUCT(BlueprintType)
struct FBatteryValueMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float CurrentValue = 100.f;
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float MaxValue = 100.f;
	/** 변경량 (양수=충전, 음수=소모) */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float Delta = 0.f;
};

// --------------------------------------------------------------
// 체력 변경 메시지
// 채널: Message.Player.Health.Changed
// 발신: 2단계 GAS AttributeSet
// 수신: UI HUD, 사망 판정 로직
// --------------------------------------------------------------
USTRUCT(BlueprintType)
struct FHealthChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float CurrentHealth = 100.f;
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float MaxHealth = 100.f;
	/** 변경량 (양수=회복, 음수=피해) */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float Delta = 0.f;
	/** 피해를 준 액터 (nullptr이면 환경 피해) */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	TObjectPtr<AActor> Instigator = nullptr;
};

// --------------------------------------------------------------
// 인벤토리 변경 메시지
// 채널: Message.Player.Inventory.ItemAdded / ItemRemoved
// 발신: PlayerState 임시 버퍼 관리 로직
// 수신: UI 인벤토리 위젯
// --------------------------------------------------------------
USTRUCT(BlueprintType)
struct FInventoryMessage
{
	GENERATED_BODY()

	/** 변경된 아이템의 에셋 ID */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	FPrimaryAssetId ItemId;
	/** 변경 수량 */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	int32 Quantity = 1;
	/** 인벤토리 슬롯 인덱스 */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	int32 SlotIndex = -1;
};

// --------------------------------------------------------------
// 소음 발생 메시지
// 채널: Message.Noise.Generated
// 발신: 무기 발사, 이동, 상호작용 등 소음 원인
// 수신: AI 감지 시스템
// --------------------------------------------------------------
USTRUCT(BlueprintType)
struct FNoiseMessage
{
	GENERATED_BODY()

	/** 소음이 발생한 월드 좌표 */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	FVector Location = FVector::ZeroVector;
	/** 소음 강도 (0.0 ~ 1.0, AI 감지 반경에 영향) */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float Intensity = 0.f;
	/** 소음을 발생시킨 액터 */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	TObjectPtr<AActor> Instigator = nullptr;
	/** 소음 종류를 구분하는 태그 (예: Noise.Footstep, Noise.Gunshot) */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	FGameplayTag NoiseTag;
};

// --------------------------------------------------------------
// 타이머 업데이트 메시지
// 채널: Message.Timer.Updated
// 발신: AExtractionGameMode
// 수신: UI HUD 타이머 위젯
// --------------------------------------------------------------
USTRUCT(BlueprintType)
struct FTimerMessage
{
	GENERATED_BODY()

	/** 남은 시간 (초) */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float RemainingSeconds = 0.f;
	/** 전체 제한 시간 (초) */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float TotalSeconds = 0.f;
};

// --------------------------------------------------------------
// 탈출 프로세스 메시지
// 채널: Message.Extraction.Started / Completed / Failed
// 발신: AExtractionGameMode 또는 탈출 트리거
// 수신: UI, 사운드, 이펙트 시스템
// --------------------------------------------------------------
USTRUCT(BlueprintType)
struct FExtractionMessage
{
	GENERATED_BODY()

	/** 탈출 지점의 월드 좌표 */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	FVector ExtractionLocation = FVector::ZeroVector;

	/** 탈출에 소요되는 시간 (초) */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	float ExtractionDuration = 0.f;

	/** 탈출을 시도하는 플레이어 */
	UPROPERTY(BlueprintReadWrite, Category = "Message")
	TObjectPtr<AActor> Player = nullptr;
};
