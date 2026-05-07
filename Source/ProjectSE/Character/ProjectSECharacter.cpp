#include "ProjectSECharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Character/SEWallRunComponent.h"


AProjectSECharacter::AProjectSECharacter()
{
	// 캡슐 크기 설정
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// 컨트롤러 회전을 캐릭터에 직접 적용 x
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 캐릭터 이동 설정
	GetCharacterMovement()->bOrientRotationToMovement = true; // 이동방향 자동 회전
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);

	// 스프링 암
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // 카메라 회전을 월드 기준으로 고정
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f)); // 탑다운 앙각
	CameraBoom->bDoCollisionTest = false; // 카메라가 벽을 통과하지 않도록 할 경우 true로

	// 탑다운 카메라
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	// 벽 타기 컴포넌트
	WallRunComp = CreateDefaultSubobject<USEWallRunComponent>(TEXT("WallRunComponent"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AProjectSECharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AProjectSECharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AProjectSECharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Enhanced Input 서브시스템 매핑 컨택스트 추가
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();

			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AProjectSECharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// Enhanced Input 컴포넌트로 캐스팅
	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	
	if (MoveAction) // 이동
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AProjectSECharacter::HandleMove);
	}
	
	if (JumpAction) // 점프
	{
		EIC->BindAction(JumpAction, ETriggerEvent::Started,   this, &AProjectSECharacter::HandleJump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &AProjectSECharacter::HandleStopJumping);
	}
	
	if (AttackAction) // 공격 - 추후 구현
	{
		EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AProjectSECharacter::HandleAttack);
	}
}

class UCameraComponent* AProjectSECharacter::GetTopDownCameraComponent() const
{
	return TopDownCameraComponent;
}

class USpringArmComponent* AProjectSECharacter::GetCameraBoom() const
{
	return CameraBoom;
}

void AProjectSECharacter::HandleMove(const FInputActionValue& Value)
{
	// IA_Move는 Axis2D(Vector2D) 타입 - X = 좌우(A/D), Y = 전후(W/S)
	const FVector2D Input = Value.Get<FVector2D>();
	if (Input.IsNearlyZero()) return;

	const float    CameraYaw   = CameraBoom->GetComponentRotation().Yaw;
	const FRotator YawRotation(0.f, CameraYaw, 0.f);
	const FVector  ForwardDir  = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector  RightDir    = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDir, Input.Y);  // W = 앞(+Y), S = 뒤(-Y)
	AddMovementInput(RightDir,   Input.X);  // D = 우(+X), A = 좌(-X)
}

void AProjectSECharacter::HandleJump(const FInputActionValue& Value)
{
	// 벽 타기 중 -> 벽 점프 우선 실행
	if (WallRunComp && WallRunComp->IsWallRunning())
	{
		WallRunComp->OnJumpInput();
	}
	else
	{
		Jump();
	}
}

void AProjectSECharacter::HandleStopJumping(const FInputActionValue& Value)
{
	StopJumping();
}

void AProjectSECharacter::HandleAttack(const FInputActionValue& Value)
{
	// TODO : 공격 or 상호작용 로직 추가
}
