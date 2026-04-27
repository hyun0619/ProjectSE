#include "SEMessageHelper.h"
#include "Message/SEGameMessages.h"
#include "GameFramework/GameplayMessageSubsystem.h"


// --------------------------------------------------------------
// 매치 상태
// --------------------------------------------------------------
void USEMessageHelper::BroadcastMatchState(
UObject* Context, EMatchState OldState, EMatchState NewState)
{
	FMatchStateMessage Msg;
	Msg.OldState = OldState;
	Msg.NewState = NewState;

	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(Context);
	Subsystem.BroadcastMessage(TAG_Message_Match_StateChanged, Msg);
}

// --------------------------------------------------------------
// 타이머
// --------------------------------------------------------------
void USEMessageHelper::BroadcastTimerUpdate(
	UObject* Context, float RemainingSeconds, float TotalSeconds)
{
	FTimerMessage Msg;
	Msg.RemainingSeconds = RemainingSeconds;
	Msg.TotalSeconds = TotalSeconds;

	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(Context);
	Subsystem.BroadcastMessage(TAG_Message_Timer_Updated, Msg);
}

// --------------------------------------------------------------
// 인벤토리
// --------------------------------------------------------------
void USEMessageHelper::BroadcastInventoryItemAdded(
	UObject* Context, FPrimaryAssetId ItemId, int32 Quantity, int32 SlotIndex)
{
	FInventoryMessage Msg;
	Msg.ItemId = ItemId;
	Msg.Quantity = Quantity;
	Msg.SlotIndex = SlotIndex;

	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(Context);
	Subsystem.BroadcastMessage(TAG_Message_Player_Inventory_ItemAdded, Msg);
}

void USEMessageHelper::BroadcastInventoryItemRemoved(
	UObject* Context, FPrimaryAssetId ItemId, int32 Quantity, int32 SlotIndex)
{
	FInventoryMessage Msg;
	Msg.ItemId = ItemId;
	Msg.Quantity = Quantity;
	Msg.SlotIndex = SlotIndex;

	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(Context);
	Subsystem.BroadcastMessage(TAG_Message_Player_Inventory_ItemRemoved, Msg);
}

// --------------------------------------------------------------
// 소음
// --------------------------------------------------------------
void USEMessageHelper::BroadcastNoise(
	UObject* Context, FVector Location, float Intensity,
	AActor* Instigator, FGameplayTag NoiseTag)
{
	FNoiseMessage Msg;
	Msg.Location = Location;
	Msg.Intensity = Intensity;
	Msg.Instigator = Instigator;
	Msg.NoiseTag = NoiseTag;

	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(Context);
	Subsystem.BroadcastMessage(TAG_Message_Noise_Generated, Msg);
}

// --------------------------------------------------------------
// 탈출 프로세스
// --------------------------------------------------------------
void USEMessageHelper::BroadcastExtractionStarted(
	UObject* Context, FVector Location, float Duration, AActor* Player)
{
	FExtractionMessage Msg;
	Msg.ExtractionLocation = Location;
	Msg.ExtractionDuration = Duration;
	Msg.Player = Player;

	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(Context);
	Subsystem.BroadcastMessage(TAG_Message_Extraction_Started, Msg);
}

void USEMessageHelper::BroadcastExtractionCompleted(
	UObject* Context, FVector Location, float Duration, AActor* Player)
{
	FExtractionMessage Msg;
	Msg.ExtractionLocation = Location;
	Msg.ExtractionDuration = Duration;
	Msg.Player = Player;

	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(Context);
	Subsystem.BroadcastMessage(TAG_Message_Extraction_Completed, Msg);
}

void USEMessageHelper::BroadcastExtractionFailed(
	UObject* Context, FVector Location, float Duration, AActor* Player)
{
	FExtractionMessage Msg;
	Msg.ExtractionLocation = Location;
	Msg.ExtractionDuration = Duration;
	Msg.Player = Player;

	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(Context);
	Subsystem.BroadcastMessage(TAG_Message_Extraction_Failed, Msg);
}

// --------------------------------------------------------------
// 배터리
// --------------------------------------------------------------
void USEMessageHelper::BroadcastBatteryState(
	UObject* Context, EBatteryState NewState, float CurrentValue, float MaxValue)
{
	FBatteryStateMessage Msg;
	Msg.NewState = NewState;
	Msg.CurrentValue = CurrentValue;
	Msg.MaxValue = MaxValue;

	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(Context);
	Subsystem.BroadcastMessage(TAG_Message_Player_Battery_StateChanged, Msg);
}

void USEMessageHelper::BroadcastBatteryValue(
	UObject* Context, float CurrentValue, float MaxValue, float Delta)
{
	FBatteryValueMessage Msg;
	Msg.CurrentValue = CurrentValue;
	Msg.MaxValue = MaxValue;
	Msg.Delta = Delta;

	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(Context);
	Subsystem.BroadcastMessage(TAG_Message_Player_Battery_ValueChanged, Msg);
}

// --------------------------------------------------------------
// 체력
// --------------------------------------------------------------
void USEMessageHelper::BroadcastHealthChanged(
	UObject* Context, float CurrentHealth, float MaxHealth,
	float Delta, AActor* Instigator)
{
	FHealthChangedMessage Msg;
	Msg.CurrentHealth = CurrentHealth;
	Msg.MaxHealth = MaxHealth;
	Msg.Delta = Delta;
	Msg.Instigator = Instigator;

	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(Context);
	Subsystem.BroadcastMessage(TAG_Message_Player_Health_Changed, Msg);
}
