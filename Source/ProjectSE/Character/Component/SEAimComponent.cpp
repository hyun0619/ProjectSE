#include "Character/Component/SEAimComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

USEAimComponent::USEAimComponent()
{
	// GASP가 회전을 처리한 뒤 우리가 덮어쓰기 위해 TG_PostPhysics에서 틱
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup    = TG_PostPhysics;
}

void USEAimComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USEAimComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsAiming || !bAimTargetValid)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// 액터를 목표 Yaw로 보간 회전 (GASP가 회전한 뒤 우리가 덮어씀)
	const FRotator Current = Owner->GetActorRotation();
	const FRotator Target(0.f, AimYawTarget, 0.f);
	const FRotator NewRot  = (AimRotationInterpSpeed > 0.f)
		? FMath::RInterpTo(Current, Target, DeltaTime, AimRotationInterpSpeed)
		: Target;
	Owner->SetActorRotation(NewRot);

	// 디버그 시각화
	if (bDrawAimDebug)
	{
		if (UWorld* World = GetWorld())
		{
			const FVector Start      = Owner->GetActorLocation();
			const FVector ForwardEnd = Start + Owner->GetActorForwardVector() * AimDebugArrowLength;
			const FVector TargetEnd  = Start + FRotator(0.f, AimYawTarget, 0.f).Vector() * AimDebugArrowLength;

			DrawDebugDirectionalArrow(World, Start, ForwardEnd, 120.f, FColor::Red,   false, 0.f, 0, 5.f);
			DrawDebugDirectionalArrow(World, Start, TargetEnd,  120.f, FColor::Green, false, 0.f, 0, 5.f);
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Yellow,
				FString::Printf(TEXT("Aim Target: %.1f, Current: %.1f"),
					AimYawTarget, NewRot.Yaw));
		}
	}
}

void USEAimComponent::SetAiming(bool bNewAiming)
{
	bIsAiming = bNewAiming;

	// ABP의 bIsAiming(Bool) 변수에도 동일하게 전달
	// (ABP의 Get_OffsetRootRotationMode 등에서 사용)
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = OwnerCharacter->GetMesh();
	if (!MeshComp)
	{
		return;
	}

	UAnimInstance* AnimBP = MeshComp->GetAnimInstance();
	if (!AnimBP)
	{
		return;
	}

	if (FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(
		AnimBP->GetClass(), AnimBPIsAimingPropertyName))
	{
		BoolProp->SetPropertyValue_InContainer(AnimBP, bNewAiming);
	}
}