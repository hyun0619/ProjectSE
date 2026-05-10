#include "ProjectSECharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "MotionWarpingComponent.h"
#include "CharacterTrajectoryComponent.h"

AProjectSECharacter::AProjectSECharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// --------------------------------------------------------------
	// 캡슐
	// --------------------------------------------------------------
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	// --------------------------------------------------------------
	// 회전: 컨트롤러 회전을 캐릭터에 그대로 적용하지 않음 (탑다운)
	// --------------------------------------------------------------
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	// --------------------------------------------------------------
	// CharacterMovement 기본값
	//   GASP는 모션 매칭이 직접 회전을 제어하는 부분이 있어
	//   bOrientRotationToMovement는 BP(CMC defaults)에서 최종 결정합니다.
	//   여기서는 합리적 기본값만 둠.
	// --------------------------------------------------------------
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate              = FRotator(0.f, 500.f, 0.f);
	Move->bUseControllerDesiredRotation = false;

	// --------------------------------------------------------------
	// 카메라 (탑다운)
	// --------------------------------------------------------------
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-50.f, 45.f, 0.f)); // BP_SE와 동일 각도
	CameraBoom->bDoCollisionTest = false;

	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	// --------------------------------------------------------------
	// GASP 의존 컴포넌트
	//   CBP_SandboxCharacter가 이미 이 컴포넌트들을 가지고 있다면
	//   리페어런트 후 BP의 컴포넌트는 삭제하고 C++의 것을 상속받게 됩니다.
	// --------------------------------------------------------------
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));

	Trajectory = CreateDefaultSubobject<UCharacterTrajectoryComponent>(TEXT("Trajectory"));
}

void AProjectSECharacter::BeginPlay()
{
	Super::BeginPlay();
}