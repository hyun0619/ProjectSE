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

struct FLDVizCameraInterp
{
	FEditorViewportClient* Viewport = nullptr;
	FVector StartLoc = FVector::ZeroVector;
	FVector EndLoc   = FVector::ZeroVector;
	float Elapsed    = 0.f;
	float Duration   = 0.25f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLDVizSelectionChanged, AActor*, NewlySelectedActor);

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

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	int32 HighlightActors(ULevelDesignTagConfig* Config);

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	int32 ClearHighlight(ULevelDesignTagConfig* Config);

	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	bool ToggleHighlight(ULevelDesignTagConfig* Config);

	/**
	 * 새 API: 현재 상태 유지하며 새로고침.
	 *   - 활성: Clear → ReApply (새 액터 반영)
	 *   - 비활성: no-op
	 */
	UFUNCTION(BlueprintCallable, Category = "Level Design|Visualizer")
	void RefreshHighlight(ULevelDesignTagConfig* Config);

	UFUNCTION(BlueprintPure, Category = "Level Design|Visualizer")
	bool IsActive(ULevelDesignTagConfig* Config) const;

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

private:
	UPROPERTY()
	TMap<FName, FLDVizTagState> ActiveStates;

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

	void ApplySizeToAllLabels();
	bool TryGetEditorCameraLocation(FVector& OutLocation) const;
	bool ComputeMeshBoundsForActor(AActor* Actor, FVector& OutOrigin, FVector& OutExtent) const;

	void StartCameraInterp(FEditorViewportClient* VC, const FVector& EndLoc, float Duration);
	void TickCameraInterps(float DeltaTime);

	UWorld* GetEditorWorld() const;
	static FName MakeVizComponentTag(FName ActorTag);
};