#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SEPistolComponent.generated.h"

/**
 * 권총 장착/해제와 발사를 담당하는 컴포넌트.
 *
 * 설계 원칙 (기존 SEAimComponent와 동일한 구조):
 *  - 상태(bIsEquipped / bIsFiring / bIsWallRunning)는 이 컴포넌트가 단독 소유한다.
 *  - ABP는 매 프레임 이 컴포넌트의 BlueprintPure + BlueprintThreadSafe getter를
 *    읽어서 자기 변수로 복사한다 (AnimBP의 스레드 세이프 업데이트 경로에서 안전).
 *  - 입력은 CBP_SandboxCharacter의 EnhancedInputAction 노드가 담당하고,
 *    여기의 BlueprintCallable 함수만 호출한다.
 */
UCLASS(ClassGroup=(ProjectSE), meta=(BlueprintSpawnableComponent), DisplayName="SE Pistol Component")
class USEPistolComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USEPistolComponent();

    // ------------------------------------------------------------------
    // 상태 조회 (ABP에서 호출) - 순수 함수 + 스레드 세이프
    // ------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon", meta = (BlueprintThreadSafe))
    bool IsEquipped() const { return bIsEquipped; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon", meta = (BlueprintThreadSafe))
    bool IsFiring() const { return bIsFiring; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon", meta = (BlueprintThreadSafe))
    bool IsWallRunning() const { return bIsWallRunning; }

    // ------------------------------------------------------------------
    // 입력 진입점 (BP에서 호출)
    // ------------------------------------------------------------------
    /** F 키: 장착/해제 토글. 월런 중이면 완전히 무시된다. */
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void ToggleEquip();

    /** 좌클릭: 권총이 장착되어 있고 월런 중이 아니면 1발 발사. */
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void FireOnce();

    /**
     * 월런 상태 통지.
     * CBP_SandboxCharacter의 HandleWallRunStarted / HandleWallRunEnded에서 호출한다.
     */
    UFUNCTION(BlueprintCallable, Category = "Weapon")
    void SetWallRunning(bool bNewWallRunning);

protected:
    virtual void BeginPlay() override;

    /**
     * 발사 상태(bIsFiring) 유지 시간(초).
     * MM_Pistol_Fire 길이(0.667s)보다 아주 살짝 길게 둬서
     * ABP의 Fire 스테이트가 애니메이션 끝까지 재생하고 빠져나갈 수 있게 한다.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.05"))
    float FireAnimDuration = 0.70f;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    bool bIsEquipped = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    bool bIsFiring = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
    bool bIsWallRunning = false;

    /** 발사 상태 자동 해제용 타이머 */
    FTimerHandle FireTimerHandle;
};