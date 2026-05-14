#include "ProjectSECharacter.h"

#include "Camera/CameraComponent.h"
#include "Character/Component/SEAimComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "MotionWarpingComponent.h"
#include "CharacterTrajectoryComponent.h"

AProjectSECharacter::AProjectSECharacter()
{
	// Character 자체는 더 이상 Tick 안 함 (SEAimComponent가 자체 Tick)
	PrimaryActorTick.bCanEverTick = false;

	// 캡슐
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// 회전: 컨트롤러 회전을 캐릭터에 그대로 적용하지 않음 (탑다운)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	// CharacterMovement 기본값
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement     = true;
	Move->RotationRate                  = FRotator(0.f, 500.f, 0.f);
	Move->bUseControllerDesiredRotation = false;

	// 카메라 (탑다운)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-50.f, 45.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	// GASP 의존 컴포넌트
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	Trajectory    = CreateDefaultSubobject<UCharacterTrajectoryComponent>(TEXT("Trajectory"));

	// 게임플레이 컴포넌트
	AimComponent = CreateDefaultSubobject<USEAimComponent>(TEXT("AimComponent"));
}

void AProjectSECharacter::BeginPlay()
{
	Super::BeginPlay();
}