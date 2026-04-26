#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SEGameTypes.generated.h"


/**
 * 게임 규칙 및 데이터 규격 타입별로 정의
 */

// --------------------------------------------------------------
// 위험구역 매치 진행 상태 열거형 - GameMode가 관리
// --------------------------------------------------------------
UENUM(BlueprintType)
enum class EMatchState : uint8
{
	/** 플레이어가 위험구역에 진입 대기 중 */
	WaitingToStart  UMETA(DisplayName = "대기 중"),
	/** 제한 시간이 흐르고 있는 활성 상태 */
	InProgress      UMETA(DisplayName = "진행 중"),
	/** 탈출 지점에서 탈출 프로세스가 진행 중 */
	Extracting      UMETA(DisplayName = "탈출 중"),
	/** 제한 시간 초과 or 사망으로 실패 */
	Failed          UMETA(DisplayName = "실패"),
	/** 탈출 성공 - 획득 아이템이 영속 데이터로 커밋 */
	Succeeded       UMETA(DisplayName = "성공")
};

// --------------------------------------------------------------
// 배터리 상태 열거형
// --------------------------------------------------------------
UENUM(BlueprintType)
enum class EBatteryState : uint8
{
	/** 배터리 충분 */
	Full     UMETA(DisplayName = "충전 완료"),
	/** 배터리 보통 */
	Normal   UMETA(DisplayName = "보통"),
	/** 배터리 부족 경고 */
	Low      UMETA(DisplayName = "부족"),
	/** 배터리 완전 소진 */
	Depleted UMETA(DisplayName = "방전")
};


// --------------------------------------------------------------
// 아이템 데이터 구조체 - 인벤토리 시스템 최소 단위
// --------------------------------------------------------------
USTRUCT(BlueprintType)
struct FItemData
{
	GENERATED_BODY()

	/** 에셋 매니저가 식별하는 고유 아이템 ID (예: "Item:Rifle_AK47") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FPrimaryAssetId ItemId;
	/** 보유 수량 - 스택 가능 아이템용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Quantity = 1;
	/** 인벤토리 내 슬롯 인덱스 | -1이면 자동 배치 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 SlotIndex = -1;

	bool operator==(const FItemData& Other) const
	{
		return ItemId == Other.ItemId && SlotIndex == Other.SlotIndex;
	}
};