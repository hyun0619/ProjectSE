#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectSECharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UMotionWarpingComponent;
class UCharacterTrajectoryComponent;
/**
 * GASP(Game Animation Sample) 기반 탑다운 캐릭터의 C++ 베이스
 *
 * - 카메라/움직임 기본 설정과 GASP 의존 컴포넌트(MotionWarping, Trajectory)를 관리
 * - 입력, 모션매칭 상태, 트래버설은 자식 BP(CBP_SandboxCharacter)가 담당
 * - 향후 게임 로직(HP, 공격, 상호작용 등)은 여기에 추가
 */
UCLASS(abstract)
class AProjectSECharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AProjectSECharacter();

	// --------------------------------------------------------------
	// 접근자
	// --------------------------------------------------------------
	FORCEINLINE UCameraComponent*           GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	FORCEINLINE USpringArmComponent*        GetCameraBoom()             const { return CameraBoom; }
	FORCEINLINE UMotionWarpingComponent*    GetMotionWarping()          const { return MotionWarping; }
	FORCEINLINE UCharacterTrajectoryComponent* GetTrajectory()          const { return Trajectory; }

protected:
	virtual void BeginPlay() override;

private:
	// --------------------------------------------------------------
	// 카메라 (탑다운)
	// --------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	// --------------------------------------------------------------
	// GASP 의존 컴포넌트
	// --------------------------------------------------------------
	/** 트래버설(볼트/맨틀 등)의 워프 타겟 처리에 사용 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GASP", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	/** 모션 매칭의 미래 궤적 예측 (5.6 빌트인) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GASP", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCharacterTrajectoryComponent> Trajectory;
};

