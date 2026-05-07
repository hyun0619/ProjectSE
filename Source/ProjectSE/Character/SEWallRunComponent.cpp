#include "SEWallRunComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"


USEWallRunComponent::USEWallRunComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // 매 프레임 TickComp 호출되어야 벽 감지 동작
}

void USEWallRunComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!ensureMsgf(OwnerCharacter, TEXT("[WallRunComponent] ACharacter에만 부착 가능. 소유자: %s"),
		*GetOwner()->GetName()))
	{
		SetComponentTickEnabled(false);
		return;
	}
	
	MovementComp = OwnerCharacter->GetCharacterMovement();
	
	// 벽타기 종료 시 원래 값으로 복원하기 위해 저장
	DefaultGravityScale = MovementComp->GravityScale;
	DefaultAirControl = MovementComp->AirControl;
}

void USEWallRunComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!OwnerCharacter || !MovementComp) return;
	
	if (bIsWallRunning)
	{
		UpdateWallRun(DeltaTime); // 이미 벽타기 중 -> 매 틱 상태 유지, 종료 판단
	}
	else
	{
		// 벽타기 중 아닐 때만 새벽 탐색 시도
		EWallRunSide DetectedSide;
		FVector DetectedNormal;
		
		if (CanStartWallRun() && FindRunnableWall(DetectedSide, DetectedNormal))
		{
			BeginWallRun(DetectedSide, DetectedNormal);
		}
	}
}

void USEWallRunComponent::OnJumpInput()
{
}

bool USEWallRunComponent::IsWallRunning() const
{
	return bIsWallRunning;
}

EWallRunSide USEWallRunComponent::GetWallRunSide() const
{
	return WallRunSide;
}

FVector USEWallRunComponent::GetWallNormal() const
{
	return WallNormal;
}

bool USEWallRunComponent::FindRunnableWall(EWallRunSide& OutSide, FVector& OutWallNormal) const
{
	if (!OwnerCharacter) return false;
	
	const FVector Origin = OwnerCharacter->GetActorLocation();
	const FVector RightVector = OwnerCharacter->GetActorRightVector();
	
	// 오른쪽, 왼쪽 각각 TraceLength만큼 LineTrace
	const FVector RightEnd = Origin + RightVector * WallDetectTraceLength;
	const FVector LeftEnd = Origin - RightVector * WallDetectTraceLength;
	
	FHitResult RightHit, LeftHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	
	const bool bHitRight = GetWorld()->LineTraceSingleByChannel(
		RightHit, Origin, RightEnd, ECC_Visibility, QueryParams);
	const bool bHitLeft = GetWorld()->LineTraceSingleByChannel(
		LeftHit, Origin, LeftEnd, ECC_Visibility, QueryParams);
	
	// 에디터 전용 디버그 시각화 - 에디터 환경 실행 시 포함, 패키징 시 완전 삭제 지시어
#if WITH_EDITOR
	DrawDebugLine(GetWorld(), Origin, RightEnd, bHitRight ? FColor::Green : FColor::Red, false, -1.f, 0, 2.f);
	DrawDebugLine(GetWorld(), Origin, LeftEnd, bHitLeft ? FColor::Green : FColor::Red, false, -1.f, 0, 2.f);
#endif
	
	// 오른쪽 우선 탐지 - 둘 다 Hit이면 오른쪽 선택
	if (bHitRight)
	{
		OutSide = EWallRunSide::Right;
		OutWallNormal = RightHit.ImpactNormal;
		return true;
	}
	if (bHitLeft)
	{
		OutSide = EWallRunSide::Left;
		OutWallNormal = LeftHit.ImpactNormal;
		return true;
	}
	return false;
}

bool USEWallRunComponent::CanStartWallRun() const
{
	if (!MovementComp) return false;
	
	// 조건 1 - 공중에 있기	
	if (MovementComp->IsMovingOnGround()) return false;
	
	// 조건 2 -  충분한 수평 속력 존재
	const FVector HorizontalVelocity(MovementComp->Velocity.X, MovementComp->Velocity.Y, 0.f);
	if (HorizontalVelocity.SizeSquared() < MinSpeedToStartWallRun*MinSpeedToStartWallRun) return false;
	
	// 조건 3 - 지면에서 MinWallRunHeight 이상 높이 떠 있기, 캡슐 하단~지면까지의 거리 측정
	const FVector TraceStart = OwnerCharacter->GetActorLocation();
	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, MinWallRunHeight);
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	
	const bool bGroundNearby = GetWorld()->LineTraceSingleByChannel(
		HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	
	return bGroundNearby; // 지면과 가까우면 false
}

void USEWallRunComponent::BeginWallRun(EWallRunSide Side, const FVector& InWallNormal)
{
	bIsWallRunning = true;
	WallRunSide = Side;
	WallNormal = InWallNormal;
	WallRunTimer = 0.f;
	
	// 낙하 중이었다면 Z 속도 0으로 초기화
	FVector Vel = MovementComp->Velocity;
	Vel.Z = 0.f;
	MovementComp->Velocity = Vel;
	
	// 중력 감소 - 벽을 타는 동안 거의 떨어지지 않도록
	MovementComp->GravityScale = WallRunGravityScale;
	// AirControl을 최대로 - 벽 타기 중에도 좌우 조작 가능
	MovementComp->AirControl = 1.f;
	
	// 외부 시스템에 알림
	OnWallRunStateChanged.Broadcast(true);
	OnWallRunSideChanged.Broadcast(Side);
}

void USEWallRunComponent::EndWallRun()
{
	if (!bIsWallRunning) return;
	
	bIsWallRunning = false;
	WallRunSide = EWallRunSide::None;
	WallNormal = FVector::ZeroVector;
	
	// 원래 중력, AirControl 복원
	MovementComp->GravityScale = DefaultGravityScale;
	MovementComp->AirControl = DefaultAirControl;
	
	OnWallRunStateChanged.Broadcast(false);
	OnWallRunSideChanged.Broadcast(EWallRunSide::None);
}

void USEWallRunComponent::UpdateWallRun(float DeltaTime)
{
	WallRunTimer += DeltaTime;
}

FVector USEWallRunComponent::ComputeWallRunDirection(const FVector& InWallNormal) const
{
	if (!OwnerCharacter) return FVector::ZeroVector;
	
	// 벽을 따라가는 수평 벡터 = 벽 법선과 세계 Up 벡터의 CrossProduct
	FVector Direction = FVector::CrossProduct(InWallNormal, FVector::UpVector);
	
	if (WallRunSide == EWallRunSide::Right)
	{
		Direction = -Direction; // 오른쪽 벽은 CrossProduct 결과가 전방 반대 방향이므로 반전
	}
	
	if (FVector::DotProduct(Direction, OwnerCharacter->GetActorForwardVector()) < 0.f)
	{
		Direction = -Direction;
	}
	
	return Direction.GetSafeNormal();
}
