#include "SEGameMessages.h"

// 매치 상태 전이
UE_DEFINE_GAMEPLAY_TAG(TAG_Message_Match_StateChanged, "Message.Match.StateChanged");

// 배터리 상태 변경 & 배터리 수치 변경
UE_DEFINE_GAMEPLAY_TAG(TAG_Message_Player_Battery_StateChanged, "Message.Player.Battery.StateChanged");
UE_DEFINE_GAMEPLAY_TAG(TAG_Message_Player_Battery_ValueChanged,	"Message.Player.Battery.ValueChanged");

// 체력 변경
UE_DEFINE_GAMEPLAY_TAG(TAG_Message_Player_Health_Changed, "Message.Player.Health.Changed");

// 아이템 획득 & 제거
UE_DEFINE_GAMEPLAY_TAG(TAG_Message_Player_Inventory_ItemAdded, "Message.Player.Inventory.ItemAdded");
UE_DEFINE_GAMEPLAY_TAG(TAG_Message_Player_Inventory_ItemRemoved, "Message.Player.Inventory.ItemRemoved");

// 소음 발생 - AI 감지용
UE_DEFINE_GAMEPLAY_TAG(TAG_Message_Noise_Generated,	"Message.Noise.Generated");

// 제한 시간 업데이트
UE_DEFINE_GAMEPLAY_TAG(TAG_Message_Timer_Updated, "Message.Timer.Updated");

// 탈출 프로세스 시작 & 성공 & 실패
UE_DEFINE_GAMEPLAY_TAG(TAG_Message_Extraction_Started, "Message.Extraction.Started");
UE_DEFINE_GAMEPLAY_TAG(TAG_Message_Extraction_Completed, "Message.Extraction.Completed");
UE_DEFINE_GAMEPLAY_TAG(TAG_Message_Extraction_Failed, "Message.Extraction.Failed");