#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SEAimComponent.generated.h"

class UAnimInstance;

/**
 * 조준(StrafeHold) 회전을 담당하는 컴포넌트.
 *
 * - 외부(PlayerController 등)가 SetAiming(true/false)와 SetAimYawTarget(Yaw)를 호출
 * - TickComponent(TG_PostPhysics)에서 소유 액터를 목표 Yaw로 보간 회전
 * - ABP의 `bIsAiming` Bool 변수에도 자동으로 푸시
 *
 * 사용:
 *   1) AProjectSECharacter처럼 ACharacter에 이 컴포넌트를 붙임
 *   2) ABP에 `bIsAiming`(Bool) 변수 생성 → Get_OffsetRootRotationMode에서 사용
 *   3) PlayerController가 Owner의 SEAimComponent를 가져와 제어
 */
UCLASS(ClassGroup=(ProjectSE), meta=(BlueprintSpawnableComponent), DisplayName="SE Aim Component")
class USEAimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USEAimComponent();

	// --------------------------------------------------------------
	// 조준 상태
	// --------------------------------------------------------------
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Aim", meta = (BlueprintThreadSafe))
	bool IsAiming() const { return bIsAiming; }

	/** 조준 상태 토글. ABP의 bIsAiming 변수에도 동일하게 전달 */
	UFUNCTION(BlueprintCallable, Category = "Aim")
	void SetAiming(bool bNewAiming);

	/** PlayerController가 매 틱 갱신해주는 목표 Yaw (월드 공간) */
	void SetAimYawTarget(float NewYaw) { AimYawTarget = NewYaw; bAimTargetValid = true; }
	void ClearAimYawTarget()           { bAimTargetValid = false; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// --------------------------------------------------------------
	// 튜닝값 (에디터 노출)
	// --------------------------------------------------------------
	/** 조준 시 캐릭터가 커서로 향하는 보간 속도(deg/sec). 0이면 즉시. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim", meta = (ClampMin = "0.0"))
	float AimRotationInterpSpeed = 12.f;

	/** ABP에서 푸시 대상 Bool 변수 이름 (기본 "bIsAiming") */
	UPROPERTY(EditAnywhere, Category = "Aim|Advanced")
	FName AnimBPIsAimingPropertyName = TEXT("bIsAiming");

	// --------------------------------------------------------------
	// 디버그 시각화 (런타임 토글)
	// --------------------------------------------------------------
	/** 조준 중 액터 정면(빨강) / 목표 방향(녹색) 화살표와 화면 메시지 표시 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Debug")
	bool bDrawAimDebug = false;

	/** 디버그 화살표 길이 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Debug", meta = (ClampMin = "10.0"))
	float AimDebugArrowLength = 200.f;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aim", meta = (AllowPrivateAccess = "true"))
	bool bIsAiming = false;

	float AimYawTarget    = 0.f;
	bool  bAimTargetValid = false;
};