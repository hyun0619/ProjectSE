#include "ProjectSEPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "ProjectSE.h"

AProjectSEPlayerController::AProjectSEPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void AProjectSEPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AProjectSEPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (StrafeHoldAction)
		{
			EIC->BindAction(StrafeHoldAction, ETriggerEvent::Started,
				this, &AProjectSEPlayerController::OnStrafeHoldStarted);
			EIC->BindAction(StrafeHoldAction, ETriggerEvent::Completed,
				this, &AProjectSEPlayerController::OnStrafeHoldCompleted);
			EIC->BindAction(StrafeHoldAction, ETriggerEvent::Canceled,
				this, &AProjectSEPlayerController::OnStrafeHoldCompleted);
		}
	}
	else
	{
		UE_LOG(LogProjectSE, Error,
			TEXT("'%s' Failed to find an Enhanced Input Component."),
			*GetNameSafe(this));
	}
}

void AProjectSEPlayerController::OnStrafeHoldStarted(const FInputActionValue& /*Value*/)
{
	bIsAiming = true;
}

void AProjectSEPlayerController::OnStrafeHoldCompleted(const FInputActionValue& /*Value*/)
{
	bIsAiming = false;
}

void AProjectSEPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bIsAiming)
	{
		UpdatePawnAimRotation(DeltaTime);
	}
}

void AProjectSEPlayerController::UpdatePawnAimRotation(float DeltaTime)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	FHitResult Hit;
	if (!GetHitResultUnderCursor(CursorTraceChannel, /*bTraceComplex*/ true, Hit))
	{
		return;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector CursorLocation = Hit.Location;

	const FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(PawnLocation, CursorLocation);
	const FRotator TargetRotation(0.f, LookAt.Yaw, 0.f);

	if (AimRotationInterpSpeed > 0.f)
	{
		const FRotator Current = GetControlRotation();
		const FRotator NewRot = UKismetMathLibrary::RInterpTo(
			Current, TargetRotation, DeltaTime, AimRotationInterpSpeed);
		SetControlRotation(NewRot);  // ← Pawn 대신 Controller
	}
	else
	{
		SetControlRotation(TargetRotation);  // ← Pawn 대신 Controller
	}
}