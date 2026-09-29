#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectSECharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UMotionWarpingComponent;
class UCharacterTrajectoryComponent;
class USEAimComponent;
class USEPistolComponent;

/**
 * GASP(Game Animation Sample) 기반 탑다운 캐릭터의 C++ 베이스
 *
 * - 카메라/움직임 기본 설정과 GASP 의존 컴포넌트(MotionWarping, Trajectory) 관리
 * - 조준 로직은 SEAimComponent에 위임
 * - 입력, 모션매칭 상태, 트래버설은 자식 BP(CBP_SandboxCharacter)가 담당
 */
UCLASS(abstract)
class AProjectSECharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AProjectSECharacter();

	// --------------------------------------------------------------
	// 컴포넌트 접근자 (Getter)
	// --------------------------------------------------------------
	FORCEINLINE UCameraComponent*              GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	FORCEINLINE USpringArmComponent*           GetCameraBoom()             const { return CameraBoom; }
	FORCEINLINE UMotionWarpingComponent*       GetMotionWarping()          const { return MotionWarping; }
	FORCEINLINE UCharacterTrajectoryComponent* GetTrajectory()             const { return Trajectory; }
	FORCEINLINE USEAimComponent*               GetAimComponent()           const { return AimComponent; }
	
	// 블루프린트나 다른 C++ 클래스에서 안전하게 컴포넌트를 가져다 쓸 수 있도록 함수 제공
	FORCEINLINE USEPistolComponent*            GetPistolComponent()        const { return PistolComponent; }

protected:
	virtual void BeginPlay() override;

	// [삭제됨] 여기에 있던 중복된 PistolComponent 선언은 지웠습니다.

private:
	// --------------------------------------------------------------
	// 카메라 (탑다운)
	// --------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;

	// --------------------------------------------------------------
	// GASP 의존 컴포넌트
	// --------------------------------------------------------------
	/** 트래버설(볼트/맨틀 등)의 워프 타겟 처리에 사용 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	/** 모션 매칭의 미래 궤적 예측 (5.6 빌트인) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GASP", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterTrajectoryComponent> Trajectory;

	// --------------------------------------------------------------
	// 게임플레이 컴포넌트
	// --------------------------------------------------------------
	/** 조준(StrafeHold) 회전 처리 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aim", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USEAimComponent> AimComponent;
	
	/** 권총 장착/발사 처리 */
	// UE5 권장 방식인 TObjectPtr를 사용한 선언 하나만 남겨둡니다.
	// AllowPrivateAccess="true" 덕분에 private 영역에 있어도 블루프린트(디테일 패널)에서 볼 수 있습니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USEPistolComponent> PistolComponent;
};