#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "LevelDesignVisualizerSubsystem.generated.h"

class AActor;
class UMaterialInterface;
class UMeshComponent;
class ULevelDesignTagConfig;
class FObjectPreSaveContext;

/** 한 메시 컴포넌트의 원본 Overlay 머티리얼 스냅샷. 토글 OFF 시 복원에 사용. */
USTRUCT()
struct FLDVizComponentSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UMeshComponent> Component;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> OriginalOverlay = nullptr;
};

/** 태그 한 개의 활성 상태(= 영향을 받은 컴포넌트들의 스냅샷 모음) */
USTRUCT()
struct FLDVizTagState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FLDVizComponentSnapshot> Snapshots;
};

/**
 * 레벨 디자인 시각화 에디터 서브시스템.
 *
 * 책임:
 *  - 액터 태그 기반으로 메시에 Overlay 머티리얼 + TextRender 라벨 부여 (Highlight)
 *  - 토글/검색/포커스 API 제공 (EUW가 호출)
 *  - 레벨 저장 직전, 맵 전환 시 자동 정리해 .umap 오염 방지
 *
 * 비-책임 (위임):
 *  - UI:           EUW (Blueprint)
 *  - 태그별 스타일: ULevelDesignTagConfig (Data Asset)
 */
UCLASS()
class LEVELDESIGNVISUALIZER_API ULevelDesignVisualizerSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	//~ UEditorSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End UEditorSubsystem

	/**
	 * Config의 ActorTag를 가진 레벨 액터들에 시각화 적용.
	 * 이미 활성인 경우 자동으로 한 번 정리 후 다시 적용(=Refresh).
	 * @return 영향을 받은 액터 수
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	int32 HighlightActors(ULevelDesignTagConfig* Config);

	/**
	 * Config의 시각화 해제. 원본 Overlay 복원, 우리가 추가한 TextRender 제거.
	 * @return 정리된 액터 수
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	int32 ClearHighlight(ULevelDesignTagConfig* Config);

	/**
	 * 현재 상태에 따라 토글. FlipFlop 구현용.
	 * @return 토글 후 ON 상태면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	bool ToggleHighlight(ULevelDesignTagConfig* Config);

	/** 해당 Config가 현재 시각화 ON 상태인지 */
	UFUNCTION(BlueprintPure, Category = "Level Design|Visualizer")
	bool IsActive(ULevelDesignTagConfig* Config) const;

	/**
	 * Config의 ActorTag를 가진 현재 레벨 액터 목록.
	 * 토글 상태와 무관하게 호출 시점의 레벨에서 새로 검색함(목록 갱신용).
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	TArray<AActor*> FindActors(ULevelDesignTagConfig* Config) const;

	/**
	 * 프로젝트의 모든 ULevelDesignTagConfig Data Asset 자동 수집.
	 * EUW의 ComboBox 채우기에 사용. DisplayName 기준 정렬.
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	TArray<ULevelDesignTagConfig*> GetAllConfigs() const;

	/** 액터를 선택하고 뷰포트 카메라를 이동/줌인 */
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	void FocusOnActor(AActor* Actor);

	/** 모든 시각화 즉시 해제. 저장 직전 / 맵 전환 시 자동 호출. */
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	void ClearAll();

private:
	/** ActorTag(FName) → 활성 상태. 키를 태그로 두면 같은 태그 재토글이 깔끔. */
	UPROPERTY()
	TMap<FName, FLDVizTagState> ActiveStates;

	FDelegateHandle MapOpenedHandle;
	FDelegateHandle PreSaveWorldHandle;

	void OnMapOpened(const FString& Filename, bool bAsTemplate);
	void OnPreSaveWorld(UWorld* World, FObjectPreSaveContext Context);

	void ApplyToActor(AActor* Actor, const ULevelDesignTagConfig* Config, FLDVizTagState& OutState);
	void RemoveTextRendersFromActor(AActor* Actor, FName VizComponentTag);

	UWorld* GetEditorWorld() const;

	/** "LDViz_<Tag>" 형태의 ComponentTag 생성. 정리할 TextRender를 식별. */
	static FName MakeVizComponentTag(FName ActorTag);
};
