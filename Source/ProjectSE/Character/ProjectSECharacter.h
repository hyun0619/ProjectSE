#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectSECharacter.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
/**
 *  WASD 이동 + 스페이스 점프 + 탑다운 카메라 사용하는 기본 캐릭터
 */
UCLASS(abstract)
class AProjectSECharacter : public ACharacter
{
	GENERATED_BODY()

private:
	// --------------------------------------------------------------
	// 컴포넌트
	// --------------------------------------------------------------
	/** 탑다운 카메라 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** 카메라를 캐릭터 위에 고정하는 스프링 암 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

public:
	// --------------------------------------------------------------
	// Enhanced Input 에셋 - 에디터 BP에서 할당
	// --------------------------------------------------------------
	/** 캐릭터가 활성화할 IMC */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	/** WASD 이동 입력 액션 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;
	
	/** 점프, 벽 점프 입력 액션 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;
	
	/** 마우스 좌클릭 입력 액션 - 지금은 바인딩만! 추후 공격 기능 연결 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;
	
	
	// --------------------------------------------------------------
	// 생명 주기
	// --------------------------------------------------------------
	AProjectSECharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	/** 컨트롤러 교체 시 IMC 자동 재등록 */
	virtual void NotifyControllerChanged() override;
	
	/** Enhanced Input 컴포넌트에 액션 바인딩 등록 */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	// --------------------------------------------------------------
	// 접근자
	// --------------------------------------------------------------
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const;
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const;
	
protected:
	// --------------------------------------------------------------
	// 입력 핸들러
	// --------------------------------------------------------------
	/** WASD 이동 처리 */
	void HandleMove(const FInputActionValue& Value);
	
	/** 스페이스바 Press 처리 - 벽 타기 중이면 벽 점프, 아니면 일반 점프 */
	void HandleJump(const FInputActionValue& Value);
	
	/** 스페이스바 Released - 가변 높이 점프를 위해 StopJumping 호출 */
	void HandleStopJumping(const FInputActionValue& Value);
	
	/** 좌클릭 Press 처리 - 추후 공격 로직 추가 */
	void HandleAttack(const FInputActionValue& Value);
};

