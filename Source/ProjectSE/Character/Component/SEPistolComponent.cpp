#include "Character/Component/SEPistolComponent.h"

#include "GameFramework/Character.h"
#include "TimerManager.h"

USEPistolComponent::USEPistolComponent()
{
	// 상태 플래그만 관리하므로 Tick은 필요 없다.
	// (발사 종료는 타이머가 처리한다)
	PrimaryComponentTick.bCanEverTick = false;
}

void USEPistolComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USEPistolComponent::ToggleEquip()
{
	// [요구사항 1] 월런 중에는 장착/해제 자체를 무시한다.
	if (bIsWallRunning)
	{
		return;
	}

	bIsEquipped = !bIsEquipped;

	// 해제 시 발사 상태를 즉시 정리한다.
	// (그러지 않으면 ABP가 Fire 포즈에 머무른 채로 블렌드아웃될 수 있다)
	if (!bIsEquipped)
	{
		bIsFiring = false;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FireTimerHandle);
		}
	}
}

void USEPistolComponent::FireOnce()
{
	// [요구사항 2 + 1] 장착 상태여야 하고, 월런 중이 아니어야 하고,
	// 이미 발사 중(=쿨다운 구간)이 아니어야 1발 나간다.
	if (!bIsEquipped || bIsWallRunning || bIsFiring)
	{
		return;
	}

	bIsFiring = true;

	UWorld* World = GetWorld();
	if (!World)
	{
		bIsFiring = false;
		return;
	}

	// 애니메이션 길이만큼만 bIsFiring을 유지한 뒤 자동으로 내린다.
	// CreateWeakLambda를 쓰면 컴포넌트가 먼저 파괴돼도 안전하다.
	World->GetTimerManager().SetTimer(
		FireTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			bIsFiring = false;
		}),
		FireAnimDuration,
		/*bLoop=*/false);
}

void USEPistolComponent::SetWallRunning(bool bNewWallRunning)
{
	if (bIsWallRunning == bNewWallRunning)
	{
		return;
	}

	bIsWallRunning = bNewWallRunning;

	if (bIsWallRunning)
	{
		// 월런 진입 → 진행 중이던 발사를 취소한다.
		bIsFiring = false;

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FireTimerHandle);
		}
	}
}