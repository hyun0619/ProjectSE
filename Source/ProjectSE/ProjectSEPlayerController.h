#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ProjectSEPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  Top-down player controller with aim-on-hold orientation.
 *  When the aim action is held, the character faces the mouse cursor,
 *  enabling strafing movement (forward/back/sideways relative to cursor direction).
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

	/** Rotation speed toward cursor (deg/sec). 0 = instant. */
	UPROPERTY(EditDefaultsOnly, Category = "Aim", meta = (ClampMin = "0.0"))
	float AimRotationInterpSpeed = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Aim")
	TEnumAsByte<ECollisionChannel> CursorTraceChannel = ECC_Visibility;

	UPROPERTY(BlueprintReadOnly, Category = "Aim")
	bool bIsAiming = false;

	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

	void OnStrafeHoldStarted(const FInputActionValue& Value);
	void OnStrafeHoldCompleted(const FInputActionValue& Value);

	/** Rotate the controlled pawn (not the controller) to face the cursor */
	void UpdatePawnAimRotation(float DeltaTime);
};