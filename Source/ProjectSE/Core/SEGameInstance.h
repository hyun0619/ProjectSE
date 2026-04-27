#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Data/SEGameTypes.h"
#include "SEGameInstance.generated.h"

class USESaveGame;
/**
 * 게임 인스턴스 - 영속 데이터 관리, 저장 & 로드, 레벨 전환 등
 */
UCLASS()
class PROJECTSE_API USEGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
	public:
	USEGameInstance();

	// --------------------------------------------------------------
	// 영속 데이터 — 레벨 전환에도 보존
	// --------------------------------------------------------------
	/** 허브에서 보유 중인 아이템 목록 - 탈출 성공 시 커밋 */
	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Inventory")
	TArray<FItemData> PersistentInventory;

	/** 보유 화폐 */
	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Economy")
	int32 Currency;

	/** 해금된 콘텐츠 */
	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Progress")
	TSet<FName> UnlockedFeatures;

	/** 위험구역 총 클리어 횟수 */
	UPROPERTY(BlueprintReadWrite, Category = "Persistence|Progress")
	int32 TotalExtractions;

	
	// --------------------------------------------------------------
	// 세이브, 로드
	// --------------------------------------------------------------
	/** 현재 영속 데이터를 지정 슬롯에 저장 */
	UFUNCTION(BlueprintCallable, Category = "SaveLoad")
	bool SaveToSlot(const FString& SlotName, int32 UserIndex = 0);

	/** 지정 슬롯에서 영속 데이터를 로드 */
	UFUNCTION(BlueprintCallable, Category = "SaveLoad")
	bool LoadFromSlot(const FString& SlotName, int32 UserIndex = 0);

	/** 세이브 파일 존재 여부 확인 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SaveLoad")
	bool DoesSaveExist(const FString& SlotName, int32 UserIndex = 0) const;

	
	// --------------------------------------------------------------
	// 레벨 전환
	// --------------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, Category = "LevelTransition", meta = (DisplayName = "허브 레벨"))
	TSoftObjectPtr<UWorld> HubLevel;

	UPROPERTY(EditDefaultsOnly, Category = "LevelTransition", meta = (DisplayName = "위험구역 레벨"))
	TSoftObjectPtr<UWorld> DangerZoneLevel;

	/** 허브 -> 위험구역 레벨 전환 */
	UFUNCTION(BlueprintCallable, Category = "LevelTransition")
	void TravelToDangerZone();

	/** 위험구역 -> 허브 레벨 전환 */
	UFUNCTION(BlueprintCallable, Category = "LevelTransition")
	void TravelToHub();

	
	// --------------------------------------------------------------
	// 데이터 커밋 - 탈출 성공 시
	// --------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "DataCommit")
	void CommitExtractionData(const TArray<FItemData>& LootedItems, int32 LootedCurrency);

	
	// --------------------------------------------------------------
	// 기본 세이브 슬롯
	// --------------------------------------------------------------
	/** 기본 세이브 슬롯 이름 */
	static const FString DEFAULT_SAVE_SLOT;

private:
	/** SaveGame 객체에서 영속 데이터를 복사해오는 내부 함수 */
	void ApplySaveData(const USESaveGame* SaveData);

	/** 현재 영속 데이터를 SaveGame 객체에 기록하는 내부 함수 */
	void PopulateSaveData(USESaveGame* SaveData) const;

	/** 레벨 전환 공통 로직 */
	void TravelToLevel(const TSoftObjectPtr<UWorld>& LevelRef);
};
