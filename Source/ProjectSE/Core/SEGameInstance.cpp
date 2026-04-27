#include "SEGameInstance.h"
#include "Core/SESaveGame.h"
#include "Kismet/GameplayStatics.h"

const FString USEGameInstance::DEFAULT_SAVE_SLOT = TEXT("ExtractionSave_0");

USEGameInstance::USEGameInstance()
	: Currency(0)
	, TotalExtractions(0)
{
}

// --------------------------------------------------------------
// 세이브, 로드
// --------------------------------------------------------------

bool USEGameInstance::SaveToSlot(const FString& SlotName, int32 UserIndex)
{
	USESaveGame* SaveData = NewObject<USESaveGame>();
	if (!SaveData) return false;

	PopulateSaveData(SaveData);

	const bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveData, SlotName, UserIndex);
	UE_LOG(LogTemp, Log, TEXT("[ExtractionGI] SaveToSlot '%s' — %s"),
		*SlotName, bSuccess ? TEXT("성공") : TEXT("실패"));
	return bSuccess;
}

bool USEGameInstance::LoadFromSlot(const FString& SlotName, int32 UserIndex)
{
	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
	USESaveGame* SaveData = Cast<USESaveGame>(Loaded);
	if (!SaveData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ExtractionGI] LoadFromSlot '%s' — 파일 없음 또는 캐스트 실패"), *SlotName);
		return false;
	}
	
	if (SaveData->SaveVersion < USESaveGame::LATEST_SAVE_VERSION)
	{
		UE_LOG(LogTemp, Log, TEXT("[ExtractionGI] 세이브 버전 %d → %d 마이그레이션 필요"),
			SaveData->SaveVersion, USESaveGame::LATEST_SAVE_VERSION);
	}

	ApplySaveData(SaveData);
	UE_LOG(LogTemp, Log, TEXT("[ExtractionGI] LoadFromSlot '%s' — 성공 (아이템 %d개, 화폐 %d)"),
		*SlotName, PersistentInventory.Num(), Currency);
	return true;
}

bool USEGameInstance::DoesSaveExist(const FString& SlotName, int32 UserIndex) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

void USEGameInstance::ApplySaveData(const USESaveGame* SaveData)
{
	PersistentInventory = SaveData->PersistentInventory;
	Currency = SaveData->Currency;
	UnlockedFeatures = SaveData->UnlockedFeatures;
	TotalExtractions = SaveData->TotalExtractions;
}

void USEGameInstance::PopulateSaveData(USESaveGame* SaveData) const
{
	SaveData->SaveVersion = USESaveGame::LATEST_SAVE_VERSION;
	SaveData->PersistentInventory = PersistentInventory;
	SaveData->Currency = Currency;
	SaveData->UnlockedFeatures = UnlockedFeatures;
	SaveData->TotalExtractions = TotalExtractions;
}

// --------------------------------------------------------------
// 레벨 전환
// --------------------------------------------------------------

void USEGameInstance::TravelToDangerZone()
{
	TravelToLevel(DangerZoneLevel);
}

void USEGameInstance::TravelToHub()
{
	TravelToLevel(HubLevel);
}

void USEGameInstance::TravelToLevel(const TSoftObjectPtr<UWorld>& LevelRef)
{
	if (LevelRef.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[ExtractionGI] TravelToLevel — 레벨 레퍼런스가 비어있음"));
		return;
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, LevelRef);
}

// --------------------------------------------------------------
// 데이터 커밋
// --------------------------------------------------------------

void USEGameInstance::CommitExtractionData(
	const TArray<FItemData>& LootedItems, int32 LootedCurrency)
{
	for (const FItemData& Looted : LootedItems)
	{
		FItemData* Existing = PersistentInventory.FindByPredicate(
			[&Looted](const FItemData& Inv)
			{
				return Inv.ItemId == Looted.ItemId;
			});

		if (Existing)
		{
			Existing->Quantity += Looted.Quantity;
		}
		else
		{
			PersistentInventory.Add(Looted);
		}
	}
	
	Currency += LootedCurrency;
	TotalExtractions++;

	UE_LOG(LogTemp, Log, TEXT("[ExtractionGI] CommitExtractionData — 아이템 %d건 커밋, 화폐 +%d (총 %d), 클리어 #%d"),
		LootedItems.Num(), LootedCurrency, Currency, TotalExtractions);
	
	SaveToSlot(DEFAULT_SAVE_SLOT);
}
