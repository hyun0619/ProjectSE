#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ProjectSEPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 *  탑다운 플레이어 컨트롤러.
 *  StrafeHold(우클릭) 입력 시 폰의 SEAimComponent에 목표 Yaw를 매 틱 전달.
 *  실제 회전 적용은 SEAimComponent의 Tick(TG_PostPhysics)에서 수행.
 */
UCLASS(abstract)
class AProjectSEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AProjectSEPlayerController();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* StrafeHoldAction;

	/** 커서 트레이스에 사용할 채널 */
	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	TEnumAsByte<ECollisionChannel> CursorTraceChannel = ECC_Visibility;

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

	void OnStrafeHoldStarted(const FInputActionValue& Value);
	void OnStrafeHoldCompleted(const FInputActionValue& Value);

	/** 매 틱 커서 위치를 트레이스해 SEAimComponent에 목표 Yaw 전달 */
	void UpdatePawnAimRotation();
};