#include "ProjectSECharacter.h"

#include "Camera/CameraComponent.h"
#include "Character/Component/SEAimComponent.h"
#include "Character/Component/SEPistolComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/InheritableComponentHandler.h"
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
	// 1. PistolComponent를 실제로 생성하고 메모리에 할당합니다.
	// 피스톨 컴포넌트 인스턴스 생성 및 할당 (이름은 고유해야 함)
    PistolComponent = CreateDefaultSubobject<USEPistolComponent>(TEXT("PistolComponent"));

#if WITH_EDITOR
	// TEMP DIAGNOSTIC (remove once the stale component record is cleared).
	if (UBlueprintGeneratedClass* ProbeBPGC = Cast<UBlueprintGeneratedClass>(GetClass()))
	{
		if (ProbeBPGC->GetName().Contains(TEXT("Sandbox")) || !PistolComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PistolProbe] class=%s pistol=%s aim=%s overrides=%d"),
				*ProbeBPGC->GetName(),
				PistolComponent ? TEXT("VALID") : TEXT("NULL"),
				AimComponent ? TEXT("VALID") : TEXT("NULL"),
				ProbeBPGC->ComponentClassOverrides.Num());

			for (const FBPComponentClassOverride& ProbeOvr : ProbeBPGC->ComponentClassOverrides)
			{
				UE_LOG(LogTemp, Warning, TEXT("[PistolProbe]   override '%s' -> %s"),
					*ProbeOvr.ComponentName.ToString(),
					ProbeOvr.ComponentClass ? *ProbeOvr.ComponentClass->GetName() : TEXT("NULL"));
			}

			if (UInheritableComponentHandler* ProbeICH = ProbeBPGC->GetInheritableComponentHandler(false))
			{
				int32 ProbeNum = 0;
				for (auto ProbeIt = ProbeICH->CreateRecordIterator(); ProbeIt; ++ProbeIt)
				{
					++ProbeNum;
					UE_LOG(LogTemp, Warning, TEXT("[PistolProbe]   ICH[%d] scsVar=%s class=%s tmpl=%s"), ProbeNum,
						*ProbeIt->ComponentKey.GetSCSVariableName().ToString(),
						ProbeIt->ComponentClass ? *ProbeIt->ComponentClass->GetName() : TEXT("NULL"),
						ProbeIt->ComponentTemplate ? *ProbeIt->ComponentTemplate->GetName() : TEXT("NULL"));
				}
				UE_LOG(LogTemp, Warning, TEXT("[PistolProbe] ICH records=%d"), ProbeNum);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[PistolProbe] ICH=null"));
			}
		}
	}
#endif
}

void AProjectSECharacter::BeginPlay()
{
	Super::BeginPlay();
}