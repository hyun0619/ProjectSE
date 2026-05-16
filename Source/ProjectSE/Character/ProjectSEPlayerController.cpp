#include "ProjectSEPlayerController.h"

#include "Character/Component/SEAimComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "ProjectSE.h"

AProjectSEPlayerController::AProjectSEPlayerController()
{
	bShowMouseCursor   = true;
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

void AProjectSEPlayerController::EnterWallRunInputMode()
{
	auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Sub) return;
	
	if (ACharacter* Char = Cast<ACharacter>(GetPawn()))
	{
		if (auto* Aim = Char->FindComponentByClass<USEAimComponent>())
		{
			Aim->SetAiming(false);
			Aim->ClearAimYawTarget();
		}
	}

	if (DefaultMappingContext)  Sub->RemoveMappingContext(DefaultMappingContext);
	if (WallRunMappingContext)  Sub->AddMappingContext(WallRunMappingContext, 0);

}

void AProjectSEPlayerController::ExitWallRunInputMode()
{
	auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Sub) return;

	if (WallRunMappingContext)  Sub->RemoveMappingContext(WallRunMappingContext);
	if (DefaultMappingContext)  Sub->AddMappingContext(DefaultMappingContext, 0);

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

void AProjectSEPlayerController::OnStrafeHoldStarted(const FInputActionValue&)
{
	ACharacter* Char = Cast<ACharacter>(GetPawn());
	if (!Char)
	{
		return;
	}

	if (USEAimComponent* Aim = Char->FindComponentByClass<USEAimComponent>())
	{
		Aim->SetAiming(true);
	}

	// 조준 중에는 CMC의 회전 제어를 끄고 SEAimComponent가 직접 회전
	if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
	{
		Move->bOrientRotationToMovement     = false;
		Move->bUseControllerDesiredRotation = false;
	}
}

void AProjectSEPlayerController::OnStrafeHoldCompleted(const FInputActionValue&)
{
	ACharacter* Char = Cast<ACharacter>(GetPawn());
	if (!Char)
	{
		return;
	}

	if (USEAimComponent* Aim = Char->FindComponentByClass<USEAimComponent>())
	{
		Aim->SetAiming(false);
		Aim->ClearAimYawTarget();
	}

	// 일반 이동 복귀 - 이동 방향으로 캐릭터가 도는 GASP 기본 모드
	if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
	{
		Move->bOrientRotationToMovement     = true;
		Move->bUseControllerDesiredRotation = false;
	}
}

void AProjectSEPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdatePawnAimRotation();
}

void AProjectSEPlayerController::UpdatePawnAimRotation()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}

	USEAimComponent* Aim = MyPawn->FindComponentByClass<USEAimComponent>();
	if (!Aim || !Aim->IsAiming())
	{
		return;
	}

	FHitResult Hit;
	if (!GetHitResultUnderCursor(CursorTraceChannel, /*bTraceComplex*/ true, Hit))
	{
		Aim->ClearAimYawTarget();
		return;
	}

	const FVector PawnLocation   = MyPawn->GetActorLocation();
	const FVector CursorLocation = Hit.Location;
	const FVector Delta          = CursorLocation - PawnLocation;
	if (Delta.SizeSquared2D() < KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(PawnLocation, CursorLocation);
	const FRotator TargetRot(0.f, LookAt.Yaw, 0.f);

	// GASP ABP가 읽을 ControlRotation 갱신
	SetControlRotation(TargetRot);

	// SEAimComponent에도 전달 (디버그/상태용)
	Aim->SetAimYawTarget(LookAt.Yaw);
}