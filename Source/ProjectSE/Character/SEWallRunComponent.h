#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SEWallRunComponent.generated.h"

// --------------------------------------------------------------
//  BP: Content/DynamicWallRun/Blueprints/Enums/Direction_Enum
// 벽이 캐릭터 기준 어느 쪽에 있는지 나타내는 열거형
// --------------------------------------------------------------
UENUM(blueprintType)
enum class EWallRunSide : uint8
{
	None UMETA(DisplayName = "None"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right"),
};

// --------------------------------------------------------------
//  BP: DynamicWallrun_BPI (Blueprint Interface) 대응 델리게이트
// 외부(AnimBP, 카메라 등)에서 벽타기 상태 변화를 구독할 수 있도록 선언
// --------------------------------------------------------------
/** 벽타기 시작, 종료 이벤트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallRunStateChanged, bool, bStarted);

/** 벽 방향 변경 이벤트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWallRunSideChanged, EWallRunSide, NewSide);

class ACharacter;
class UCharacterMovementComponent;

/**
 * BP 원본: DynamicWallRunComponent_BP
 * 캐릭터의 부착하는 ActorComponent -> 매 틱마다 좌우 LineTrace로 벽을 감지하고 조건을 만족하면 벽타기를 실행
 * 사용법:
 * 1. 캐릭터 BP에서 이 컴포넌트를 Add Component
 * 2. 점프 입력 바인딩에서 OnJumpInput() 호출
 * 3. AnimBP에서 OnWallRunStateChanged / OnWallRunSideChanged 구독
 */
UCLASS(ClassGroup=(WallRun), meta=(BlueprintSpawnableComponent))
class PROJECTSE_API USEWallRunComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USEWallRunComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// --------------------------------------------------------------
	// 퍼블릭 인터페이스
	// --------------------------------------------------------------
	/** 점프 입력이 들어왔을 때 - 벽타기 중이면 벽 점프를 함 */
	UFUNCTION(blueprintCallable, Category = "WallRun")
	void OnJumpInput();
	
	/** 현재 벽타는 중 여부 */
	UFUNCTION(blueprintCallable, Category = "WallRun")
	bool IsWallRunning() const;
	
	/** 현재 어느 쪽 벽을 타고 있는지 */
	UFUNCTION(blueprintCallable, Category = "WallRun")
	EWallRunSide GetWallRunSide() const;
	
	/** 현재 벽의 법선 벡터 반환 - 카메라 틸트 계산, 파티클 방향 설정에 활용 */
	UFUNCTION(blueprintCallable, Category = "WallRun")
	FVector GetWallNormal() const;
	
	// --------------------------------------------------------------
	// 이벤트
	// --------------------------------------------------------------
	/** 벽타기 시작, 종료 시 호출 */
	UPROPERTY(BlueprintAssignable, Category = "WallRun|Events")
	FOnWallRunStateChanged OnWallRunStateChanged;
	
	/** 벽 방향 변경 시 호출 */
	UPROPERTY(BlueprintAssignable, Category = "WallRun|Events")
	FOnWallRunSideChanged OnWallRunSideChanged;

	
protected:
	// --------------------------------------------------------------
	// 설정 값 - 디테일 패널에서 조절
	// --------------------------------------------------------------
	/** 좌우 벽 감지 LineTrace 길이 */
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="WallRun|Settings")
	float WallDetectTraceLength = 75.f;
	
	/** 벽을 따라 이동하는 속도 */
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="WallRun|Settings")
	float WallRunSpeed = 800.f;
	
	/** 벽타기 중 중력 스케일 - 0에 가까울수록 거의 낙하x */
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="WallRun|Settings", meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float WallRunGravityScale = 0.25f;
	
	/** 벽타기 최대 지속 시간 */
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="WallRun|Settings", meta=(ClampMin = "0.1"))
	float MaxWallRunTime = 1.5f;
	
	/** 벽 점프 시 위로 가하는 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallRun|Settings")
    float WallHumpHeight = 600.f;
	
	/** 벽 점프 시 벽 법선 방향으로 밀어내는 힘 = 캐릭터가 벽에서 멀어지는 수평 힘 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallRun|Settings")
    float WallJumpOffForce = 500.f;
	
	/** 벽타기 시작 가능한 최소 공중 높이 - 땅에 붙어서 벽타기 방지 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallRun|Settings")
	float MinWallRunHeight = 50.f;
	
	/** 벽타기 시작에 필요한 최소 수평 속력 - 제자리 or 너무 느릴 땐 벽타기 시작x */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WallRun|Settings")
    float MinSpeedToStartWallRun = 200.f;
	

private:
	// --------------------------------------------------------------
	// 런타임 상태
	// --------------------------------------------------------------
	UPROPERTY()
	ACharacter* OwnerCharacter = nullptr;
	
	UPROPERTY()
	UCharacterMovementComponent* MovementComp = nullptr;
	
	/** 현재 벽타기 중 여부 */
	bool bIsWallRunning = false;
	
	/** 현재 타고 있는 벽의 방향 */
	EWallRunSide WallRunSide = EWallRunSide::None;
	
	/** 현재 벽의 법선 벡터 */
	FVector WallNormal = FVector::ZeroVector;
	
	/** 벽타기 경과 시간 - MaxWallRunTime 초과 시 자동 종료 */
	float WallRunTimer = 0.f;
	
	/** EndWallRun에서 원래 중력으로 복원 */
	float DefaultGravityScale = 1.f;
	
	/** EndWallRun에서 원래 AirControl(=공중 제어)로 복원 */
	float DefaultAirControl = 0.05f;
	
	// --------------------------------------------------------------
	// 내부 함수
	// --------------------------------------------------------------
	/** 좌우 LineTrace를 쏴 타고 올라갈 수 있는 벽 탐색 */
	bool FindRunnableWall(EWallRunSide& OutSide, FVector& OutWallNormal) const;
	
	/** 벽타기 시작 조건을 모두 검사 */
	bool CanStartWallRun() const;
	
	/** 벽타기 시작 처리 중력, AirControl 조정, 초기 속도 정리, 이벤트 브로드캐스트 */
	void BeginWallRun(EWallRunSide Side, const FVector& InWallNormal);
	
	/** 벽타기 종료 처리 - 중력, AirControl 복원, 상태 초기화, 이벤트 브로드캐스트 */
	void EndWallRun();
	
	/** 벽타기 중 매 틱 처리 - 타이머 증가, 벽 재감지, 속도 방향 유지, 캐릭터 회전 보간 */
	void UpdateWallRun(float DeltaTime);
	
	/** 벽을 따라가는 수평 단위 벡터 계산 - 벽의 법선 벡터와 Up 벡터의 CrossProduct */
	FVector ComputeWallRunDirection(const FVector& InWallNormal) const;
};
