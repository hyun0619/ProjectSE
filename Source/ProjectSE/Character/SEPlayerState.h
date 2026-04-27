#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Data/SEGameTypes.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayTagContainer.h" 
#include "Message/SEGameMessages.h"
#include "SEPlayerState.generated.h"

/**
 * 위험구역 내 플레이어의 휘발성 데이터 관리
 * APlayerState - 레벨이 로드될 때 생성, 레벨이 끝나면 파괴
 * 탈출 성공 시에만 이 임시 데이터가 GameInstance의 영속 데이터로 커밋
 * 탈출 실패 시 PlayerState가 파괴되면서 자연스럽게 소멸
 */
UCLASS()
class PROJECTSE_API ASEPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	ASEPlayerState();

	virtual void BeginPlay() override;

	// --------------------------------------------------------------
	// 임시 인벤토리 버퍼
	// --------------------------------------------------------------
	/** 위험구역에서 아이템을 획득할 때 호출 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddLootItem(const FItemData& Item);
	
	/** 임시 버퍼에서 아이템을 제거할 때 호출 (드롭, 사용 등) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveLootItem(const FPrimaryAssetId& ItemId, int32 Quantity = 1);

	/** 현재 임시 버퍼에 들어있는 아이템 목록 조회 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	const TArray<FItemData>& GetLootBuffer() const { return LootBuffer; }

	
	// --------------------------------------------------------------
	// 임시 화폐
	// --------------------------------------------------------------
	/** 위험구역에서 화폐를 획득 */
	UFUNCTION(BlueprintCallable, Category = "Economy")
	void AddLootCurrency(int32 Amount);

	/** 현재 임시 화폐 조회 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Economy")
	int32 GetLootCurrency() const { return LootCurrency; }

	
	// --------------------------------------------------------------
	// 영속 데이터 커밋
	// --------------------------------------------------------------
	/** 탈출 성공 시 호출 — 모든 데이터를 GameInstance의 영속 데이터로 커밋 */
	UFUNCTION(BlueprintCallable, Category = "DataCommit")
	void CommitToGameInstance();

protected:
	/** 위험구역에서 획득한 아이템 임시 저장소 — 탈출 실패 시 소멸 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FItemData> LootBuffer;

	/** 위험구역에서 획득한 화폐 임시 저장소 */
	UPROPERTY(BlueprintReadOnly, Category = "Economy")
	int32 LootCurrency = 0;

private:
	/** GMS 매치 상태 수신 핸들 — 탈출 성공 시 자동 커밋을 위함 */
	FGameplayMessageListenerHandle MatchStateListenerHandle;

	/** GMS로 수신한 매치 상태 변경 처리 */
	void OnMatchStateChanged(FGameplayTag Channel, const FMatchStateMessage& Message);
};
