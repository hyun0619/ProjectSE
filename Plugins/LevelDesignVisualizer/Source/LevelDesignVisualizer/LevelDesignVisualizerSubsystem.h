#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "TickableEditorObject.h"
#include "LevelDesignTagConfig.h"
#include "LevelDesignVisualizerSubsystem.generated.h"

class AActor;
class UMaterialInterface;
class UMeshComponent;
class UTextRenderComponent;
class FEditorViewportClient;
class FObjectPreSaveContext;

USTRUCT()
struct FLDVizComponentSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UMeshComponent> Component;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> OriginalOverlay = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> OriginalSlotMaterials;

	UPROPERTY()
	ELDVizHighlightMode AppliedMode = ELDVizHighlightMode::Overlay;
};

USTRUCT()
struct FLDVizLabel
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UTextRenderComponent> Component;

	UPROPERTY()
	TWeakObjectPtr<AActor> OwnerActor;

	UPROPERTY()
	float BaseWorldSize = 50.f;

	UPROPERTY()
	float HeightOffset = 50.f;
};

USTRUCT()
struct FLDVizTagState
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<ULevelDesignTagConfig> ConfigRef;

	UPROPERTY()
	TArray<FLDVizComponentSnapshot> Snapshots;

	UPROPERTY()
	TArray<FLDVizLabel> Labels;
};

/** 사용자가 명시적으로 OFF 한 액터들 (Config 별로 보관). TMap 값에 TArray 를 직접 못 두니 래핑. */
USTRUCT()
struct FLDVizActorSet
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> Actors;
};

struct FLDVizCameraInterp
{
	FEditorViewportClient* Viewport = nullptr;
	FVector StartLoc = FVector::ZeroVector;
	FVector EndLoc   = FVector::ZeroVector;
	float Elapsed    = 0.f;
	float Duration   = 0.25f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLDVizSelectionChanged, AActor*, NewlySelectedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLDVizHighlightChanged);

UCLASS()
class LEVELDESIGNVISUALIZER_API ULevelDesignVisualizerSubsystem
	: public UEditorSubsystem
	, public FTickableEditorObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }

	// === Per-Config ===
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	int32 HighlightActors(ULevelDesignTagConfig* Config);

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	int32 ClearHighlight(ULevelDesignTagConfig* Config);

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	bool ToggleHighlight(ULevelDesignTagConfig* Config);

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	void RefreshHighlight(ULevelDesignTagConfig* Config);

	UFUNCTION(BlueprintPure, Category = "Level Design|Visualizer")
	bool IsActive(ULevelDesignTagConfig* Config) const;

	// === Per-Actor ===

	/** 해당 액터가 지금 시각화 적용중인지 (실제 렌더링 상태). */
	UFUNCTION(BlueprintPure, Category = "Level Design|Visualizer")
	bool IsActorHighlighted(AActor* Actor, ULevelDesignTagConfig* Config) const;

	/**
	 * 사용자가 이 액터를 시각화에 "포함하길 원하는지" (체크박스 상태).
	 * 기본값 true. 사용자가 OFF 한 액터만 false.
	 */
	UFUNCTION(BlueprintPure, Category = "Level Design|Visualizer")
	bool IsActorVisualizationEnabled(AActor* Actor, ULevelDesignTagConfig* Config) const;

	/**
	 * 사용자 의도 저장 + 즉시 적용.
	 *   bEnabled=true:  Disabled 목록에서 제거. Config 활성이면 즉시 ON.
	 *   bEnabled=false: Disabled 목록에 추가.  Config 활성이면 즉시 OFF.
	 * Config 비활성이면 의도만 저장하고 시각적 변경 없음.
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	void SetActorHighlighted(AActor* Actor, ULevelDesignTagConfig* Config, bool bEnabled);

	// === Global ===
	UFUNCTION(BlueprintPure, Category = "Level Design|Visualizer")
	bool IsAnyConfigActive() const;

	/**
	 * 선택된 Config 의 모든 액터를 강제로 ON/OFF.
	 * 사용자가 체크박스로 꺼둔 액터(DisabledActorsByTag[Config]) 기록까지 초기화 →
	 * 다음 ON 때 그 Config 의 모든 액터가 강제로 ON 상태로 들어옴.
	 * Toggle 기준: 그 Config 가 현재 활성이면 OFF, 아니면 ON.
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	void ToggleAllHighlights(ULevelDesignTagConfig* Config);

	/**
	 * 모든 Config 의 모든 액터를 강제로 ON/OFF.
	 * 모든 DisabledActorsByTag 기록 초기화.
	 * Toggle 기준: 아무 Config 라도 활성이면 전체 OFF, 모두 비활성이면 전체 ON.
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	void ForceToggleAllHighlights();

	// === 기타 ===
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	TArray<AActor*> FindActors(ULevelDesignTagConfig* Config) const;

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	TArray<ULevelDesignTagConfig*> GetAllConfigs() const;

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	void FocusOnActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	void ToggleFocusOnActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	void ClearAll();

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	void SetBillboardEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Level Design|Visualizer")
	bool GetBillboardEnabled() const { return bBillboardEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	void SetLabelSizeMultiplier(float Multiplier);

	UFUNCTION(BlueprintPure, Category = "Level Design|Visualizer")
	float GetLabelSizeMultiplier() const { return LabelSizeMultiplier; }

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	TArray<FName> GetActorExtraTags(AActor* Actor, FName ExcludeTag) const;

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	FText GetActorExtraTagsLabel(AActor* Actor, FName ExcludeTag) const;

	UFUNCTION(BlueprintPure, Category = "Level Design|Visualizer")
	AActor* GetCurrentSelectedActor() const;

	UPROPERTY(BlueprintAssignable, Category = "Level Design|Visualizer")
	FLDVizSelectionChanged OnSelectionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Level Design|Visualizer")
	FLDVizHighlightChanged OnHighlightChanged;

private:
	UPROPERTY()
	TMap<FName, FLDVizTagState> ActiveStates;

	/** 사용자가 명시적으로 OFF 한 액터들. ClearAll 에서는 비우지 않음 — 위젯 닫았다 다시 켜도 유지. */
	UPROPERTY()
	TMap<FName, FLDVizActorSet> DisabledActorsByTag;

	UPROPERTY()
	bool bBillboardEnabled = true;

	UPROPERTY()
	float LabelSizeMultiplier = 1.f;

	UPROPERTY()
	TArray<TWeakObjectPtr<ULevelDesignTagConfig>> PendingDuplicateReapply;

	TArray<FLDVizCameraInterp> CameraInterps;

	FDelegateHandle MapOpenedHandle;
	FDelegateHandle PreSaveWorldHandle;
	FDelegateHandle SelectionChangedHandle;
	FDelegateHandle DuplicateBeginHandle;
	FDelegateHandle DuplicateEndHandle;

	void OnMapOpened(const FString& Filename, bool bAsTemplate);
	void OnPreSaveWorld(UWorld* World, FObjectPreSaveContext Context);
	void OnEditorSelectionChanged(UObject* NewSelection);
	void OnDuplicateActorsBegin();
	void OnDuplicateActorsEnd();

	void ApplyToActor(AActor* Actor, const ULevelDesignTagConfig* Config, FLDVizTagState& OutState);
	void RestoreSnapshot(const FLDVizComponentSnapshot& Snap);
	void RemoveTextRendersFromActor(AActor* Actor, FName VizComponentTag);
	void RemoveActorFromState(AActor* Actor, FLDVizTagState& State, FName ActorTag);

	void ApplySizeToAllLabels();
	bool TryGetEditorCameraLocation(FVector& OutLocation) const;
	bool ComputeMeshBoundsForActor(AActor* Actor, FVector& OutOrigin, FVector& OutExtent) const;

	void StartCameraInterp(FEditorViewportClient* VC, const FVector& EndLoc, float Duration);
	void TickCameraInterps(float DeltaTime);

	void BroadcastHighlightChanged();

	UWorld* GetEditorWorld() const;
	static FName MakeVizComponentTag(FName ActorTag);
};