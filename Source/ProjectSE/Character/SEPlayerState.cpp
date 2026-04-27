#include "SEPlayerState.h"
#include "Message/SEGameMessages.h"
#include "Core/SEGameInstance.h"
#include "Message/SEMessageHelper.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Kismet/GameplayStatics.h"


ASEPlayerState::ASEPlayerState()
{
}

void ASEPlayerState::BeginPlay()
{
	Super::BeginPlay();

	UGameplayMessageSubsystem& MessageSubsystem =UGameplayMessageSubsystem::Get(this);

	MatchStateListenerHandle = MessageSubsystem.RegisterListener<FMatchStateMessage>(
		TAG_Message_Match_StateChanged,
		this,
		&ASEPlayerState::OnMatchStateChanged);

	UE_LOG(LogTemp, Log, TEXT("[ExtractionPS] BeginPlay — GMS 매치 상태 채널 구독 완료"));
}

// --------------------------------------------------------------
// 임시 인벤토리 버퍼
// --------------------------------------------------------------
void ASEPlayerState::AddLootItem(const FItemData& Item)
{
	// 같은 아이템이 이미 버퍼에 있으면 수량만 증가
	FItemData* Existing = LootBuffer.FindByPredicate(
		[&Item](const FItemData& Buffered)
		{
			return Buffered.ItemId == Item.ItemId;
		});

	int32 FinalSlotIndex = Item.SlotIndex;

	if (Existing)
	{
		Existing->Quantity += Item.Quantity;
		FinalSlotIndex = Existing->SlotIndex;
	}
	else
	{
		// 슬롯 인덱스가 -1이면 자동으로 다음 빈 인덱스 할당
		FItemData NewItem = Item;
		if (NewItem.SlotIndex < 0)
		{
			NewItem.SlotIndex = LootBuffer.Num();
		}
		FinalSlotIndex = NewItem.SlotIndex;
		LootBuffer.Add(NewItem);
	}

	// GMS로 아이템 추가 알림 -> UI 인벤토리 위젯이 수신
	USEMessageHelper::BroadcastInventoryItemAdded(
		this, Item.ItemId, Item.Quantity, FinalSlotIndex);

	UE_LOG(LogTemp, Log, TEXT("[ExtractionPS] AddLootItem — %s x%d (슬롯 %d)"),
		*Item.ItemId.ToString(), Item.Quantity, FinalSlotIndex);
}

bool ASEPlayerState::RemoveLootItem(const FPrimaryAssetId& ItemId, int32 Quantity)
{
	for (int32 i = 0; i < LootBuffer.Num(); ++i)
	{
		if (LootBuffer[i].ItemId == ItemId)
		{
			const int32 SlotIndex = LootBuffer[i].SlotIndex;
			LootBuffer[i].Quantity -= Quantity;

			if (LootBuffer[i].Quantity <= 0)
			{
				LootBuffer.RemoveAt(i);
			}

			// GMS로 아이템 제거 알림
			USEMessageHelper::BroadcastInventoryItemRemoved(
				this, ItemId, Quantity, SlotIndex);

			UE_LOG(LogTemp, Log, TEXT("[ExtractionPS] RemoveLootItem — %s x%d"),
				*ItemId.ToString(), Quantity);
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[ExtractionPS] RemoveLootItem — %s 을 찾을 수 없음"),
		*ItemId.ToString());
	return false;
}

// --------------------------------------------------------------
// 임시 화폐
// --------------------------------------------------------------
void ASEPlayerState::AddLootCurrency(int32 Amount)
{
	LootCurrency += Amount;
	UE_LOG(LogTemp, Log, TEXT("[ExtractionPS] AddLootCurrency — +%d (총 %d)"),
		Amount, LootCurrency);
}

// --------------------------------------------------------------
// 영속 데이터 커밋
// --------------------------------------------------------------
void ASEPlayerState::CommitToGameInstance()
{
	USEGameInstance* GI = Cast<USEGameInstance>(
		UGameplayStatics::GetGameInstance(this));

	if (!GI)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[ExtractionPS] CommitToGameInstance — GameInstance 캐스트 실패"));
		return;
	}

	// GameInstance의 커밋 함수에 임시 데이터 전달
	GI->CommitExtractionData(LootBuffer, LootCurrency);

	// 임시 버퍼 초기화
	LootBuffer.Empty();
	LootCurrency = 0;

	UE_LOG(LogTemp, Log, TEXT("[ExtractionPS] CommitToGameInstance — 커밋 완료, 버퍼 초기화"));
}

// --------------------------------------------------------------
// GMS 수신 핸들러
// --------------------------------------------------------------
void ASEPlayerState::OnMatchStateChanged(
	FGameplayTag Channel, const FMatchStateMessage& Message)
{
	// 탈출 성공 시 자동으로 영속 데이터에 커밋
	if (Message.NewState == EMatchState::Succeeded)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[ExtractionPS] GMS 수신 — 매치 성공, 자동 커밋 실행"));
		CommitToGameInstance();
	}
}
