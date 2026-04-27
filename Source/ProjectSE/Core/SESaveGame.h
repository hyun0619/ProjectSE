#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Data/SEGameTypes.h"
#include "SESaveGame.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTSE_API USESaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	USESaveGame();
	
	/** 현재 세이브 데이터의 포멧 버전 */
	UPROPERTY(SaveGame, VisibleAnywhere, Category = "SaveData")
	int32 SaveVersion;
	/** 현재 저장 포맷의 최신 버전 */
	static constexpr int32 LATEST_SAVE_VERSION = 1;

	/** 보유 화폐량 */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "SaveData")
	int32 Currency;
	
	/** 위험구역 총 클리어 횟수 */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "SaveData")
	int32 TotalExtractions;

	/** 창고 인벤토리 - 허브에서 보유 중인 아이템 목록 */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "SaveData")
	TArray<FItemData> PersistentInventory;
	
	/** 해금된 콘텐츠 태그 목록 - 예: "Unlock.Map.Warehouse" */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "SaveData")
	TSet<FName> UnlockedFeatures;
	
};
