#include "LevelDesignVisualizerSubsystem.h"
#include "LevelDesignTagConfig.h"

#include "Editor.h"
#include "EditorViewportClient.h"
#include "EngineUtils.h"
#include "Selection.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/MeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "UObject/ObjectSaveContext.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

DEFINE_LOG_CATEGORY_STATIC(LogLDViz, Log, All);

// ===========================================================================
// Lifecycle
// ===========================================================================

void ULevelDesignVisualizerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PreSaveWorldHandle = FEditorDelegates::PreSaveWorldWithContext.AddUObject(
		this, &ULevelDesignVisualizerSubsystem::OnPreSaveWorld);
	MapOpenedHandle = FEditorDelegates::OnMapOpened.AddUObject(
		this, &ULevelDesignVisualizerSubsystem::OnMapOpened);
	SelectionChangedHandle = USelection::SelectionChangedEvent.AddUObject(
		this, &ULevelDesignVisualizerSubsystem::OnEditorSelectionChanged);
	DuplicateBeginHandle = FEditorDelegates::OnDuplicateActorsBegin.AddUObject(
		this, &ULevelDesignVisualizerSubsystem::OnDuplicateActorsBegin);
	DuplicateEndHandle = FEditorDelegates::OnDuplicateActorsEnd.AddUObject(
		this, &ULevelDesignVisualizerSubsystem::OnDuplicateActorsEnd);
}

void ULevelDesignVisualizerSubsystem::Deinitialize()
{
	FEditorDelegates::PreSaveWorldWithContext.Remove(PreSaveWorldHandle);
	FEditorDelegates::OnMapOpened.Remove(MapOpenedHandle);
	FEditorDelegates::OnDuplicateActorsBegin.Remove(DuplicateBeginHandle);
	FEditorDelegates::OnDuplicateActorsEnd.Remove(DuplicateEndHandle);
	USelection::SelectionChangedEvent.Remove(SelectionChangedHandle);

	CameraInterps.Empty();
	ClearAll();
	Super::Deinitialize();
}

void ULevelDesignVisualizerSubsystem::OnEditorSelectionChanged(UObject*)
{
	OnSelectionChanged.Broadcast(GetCurrentSelectedActor());
}

void ULevelDesignVisualizerSubsystem::BroadcastHighlightChanged()
{
	OnHighlightChanged.Broadcast();
}

// ===========================================================================
// 복제 처리
// ===========================================================================

void ULevelDesignVisualizerSubsystem::OnDuplicateActorsBegin()
{
	PendingDuplicateReapply.Empty();
	for (auto& Pair : ActiveStates)
	{
		if (ULevelDesignTagConfig* Cfg = Pair.Value.ConfigRef.Get())
		{
			PendingDuplicateReapply.Add(Cfg);
		}
	}
	if (PendingDuplicateReapply.Num() > 0) ClearAll();
}

void ULevelDesignVisualizerSubsystem::OnDuplicateActorsEnd()
{
	if (PendingDuplicateReapply.Num() == 0) return;
	for (TWeakObjectPtr<ULevelDesignTagConfig>& CfgPtr : PendingDuplicateReapply)
	{
		if (ULevelDesignTagConfig* Cfg = CfgPtr.Get()) HighlightActors(Cfg);
	}
	PendingDuplicateReapply.Empty();
}

// ===========================================================================
// Tick
// ===========================================================================

void ULevelDesignVisualizerSubsystem::Tick(float DeltaTime)
{
	if (CameraInterps.Num() > 0) TickCameraInterps(DeltaTime);
	if (ActiveStates.IsEmpty()) return;

	FVector CamLoc = FVector::ZeroVector;
	const bool bHasCam = bBillboardEnabled && TryGetEditorCameraLocation(CamLoc);

	for (auto& Pair : ActiveStates)
	{
		for (FLDVizLabel& Label : Pair.Value.Labels)
		{
			UTextRenderComponent* Text = Label.Component.Get();
			AActor* Owner = Label.OwnerActor.Get();
			if (!Text || !Owner) continue;

			FVector NewLoc;
			FVector MeshOrigin, MeshExtent;
			if (ComputeMeshBoundsForActor(Owner, MeshOrigin, MeshExtent))
			{
				NewLoc = FVector(MeshOrigin.X, MeshOrigin.Y, MeshOrigin.Z + MeshExtent.Z + Label.HeightOffset);
			}
			else
			{
				NewLoc = Owner->GetActorLocation() + FVector(0.f, 0.f, Label.HeightOffset);
			}
			Text->SetWorldLocation(NewLoc);

			if (bHasCam)
			{
				const FVector Dir = CamLoc - NewLoc;
				if (!Dir.IsNearlyZero())
				{
					FRotator LookRot = Dir.Rotation();
					LookRot.Pitch = 0.f;
					LookRot.Roll  = 0.f;
					Text->SetWorldRotation(LookRot);
				}
			}
		}
	}
}

TStatId ULevelDesignVisualizerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULevelDesignVisualizerSubsystem, STATGROUP_Tickables);
}

bool ULevelDesignVisualizerSubsystem::IsTickable() const
{
	return !ActiveStates.IsEmpty() || CameraInterps.Num() > 0;
}

// ===========================================================================
// 카메라 보간
// ===========================================================================

void ULevelDesignVisualizerSubsystem::StartCameraInterp(
	FEditorViewportClient* VC, const FVector& EndLoc, float Duration)
{
	if (!VC) return;
	for (FLDVizCameraInterp& Existing : CameraInterps)
	{
		if (Existing.Viewport == VC)
		{
			Existing.StartLoc = VC->GetViewLocation();
			Existing.EndLoc   = EndLoc;
			Existing.Elapsed  = 0.f;
			Existing.Duration = FMath::Max(0.01f, Duration);
			return;
		}
	}
	FLDVizCameraInterp NewInterp;
	NewInterp.Viewport = VC;
	NewInterp.StartLoc = VC->GetViewLocation();
	NewInterp.EndLoc   = EndLoc;
	NewInterp.Elapsed  = 0.f;
	NewInterp.Duration = FMath::Max(0.01f, Duration);
	CameraInterps.Add(NewInterp);
}

void ULevelDesignVisualizerSubsystem::TickCameraInterps(float DeltaTime)
{
	TSet<FEditorViewportClient*> AliveVCs;
	if (GEditor)
	{
		for (FEditorViewportClient* VC : GEditor->GetAllViewportClients())
		{
			if (VC) AliveVCs.Add(VC);
		}
	}
	for (int32 i = CameraInterps.Num() - 1; i >= 0; --i)
	{
		FLDVizCameraInterp& Interp = CameraInterps[i];
		if (!AliveVCs.Contains(Interp.Viewport))
		{
			CameraInterps.RemoveAt(i);
			continue;
		}
		Interp.Elapsed += DeltaTime;
		float Alpha = FMath::Clamp(Interp.Elapsed / Interp.Duration, 0.f, 1.f);
		Alpha = FMath::SmoothStep(0.f, 1.f, Alpha);
		const FVector CurLoc = FMath::Lerp(Interp.StartLoc, Interp.EndLoc, Alpha);
		Interp.Viewport->SetViewLocation(CurLoc);
		Interp.Viewport->Invalidate();
		if (Interp.Elapsed >= Interp.Duration) CameraInterps.RemoveAt(i);
	}
}

// ===========================================================================
// Editor delegates
// ===========================================================================

void ULevelDesignVisualizerSubsystem::OnMapOpened(const FString&, bool)
{
	ActiveStates.Empty();
	PendingDuplicateReapply.Empty();
	CameraInterps.Empty();

	// 다른 맵 액터의 weak ref 들은 의미 없으니 disabled 도 비움
	DisabledActorsByTag.Empty();
	SuspendedDisabledByTag.Empty();
	
	BroadcastHighlightChanged();
}

void ULevelDesignVisualizerSubsystem::OnPreSaveWorld(UWorld*, FObjectPreSaveContext)
{
	if (!ActiveStates.IsEmpty()) ClearAll();
}

// ===========================================================================
// Per-Config API
// ===========================================================================

int32 ULevelDesignVisualizerSubsystem::HighlightActors(ULevelDesignTagConfig* Config)
{
	if (!Config || Config->ActorTag.IsNone()) return 0;
	if (ActiveStates.Contains(Config->ActorTag)) ClearHighlight(Config);

	const TArray<AActor*> Actors = FindActors(Config);
	if (Actors.Num() == 0) return 0;

	FLDVizTagState NewState;
	NewState.ConfigRef = Config;
	NewState.Snapshots.Reserve(Actors.Num() * 2);
	NewState.Labels.Reserve(Actors.Num());

	// 사용자가 OFF 한 액터는 건너뜀
	int32 Applied = 0;
	for (AActor* Actor : Actors)
	{
		if (!IsActorVisualizationEnabled(Actor, Config)) continue;
		ApplyToActor(Actor, Config, NewState);
		++Applied;
	}

	// 적용된 액터가 0개여도 ActiveStates 에는 추가 → IsActive=true 유지
	// (사용자가 명시적으로 켰는데 모두 disabled 인 케이스)
	ActiveStates.Add(Config->ActorTag, MoveTemp(NewState));

	BroadcastHighlightChanged();
	return Applied;
}

int32 ULevelDesignVisualizerSubsystem::ClearHighlight(ULevelDesignTagConfig* Config)
{
	if (!Config || Config->ActorTag.IsNone()) return 0;
	const FLDVizTagState* State = ActiveStates.Find(Config->ActorTag);
	if (!State) return 0;

	TSet<AActor*> AffectedActors;
	for (const FLDVizComponentSnapshot& Snap : State->Snapshots)
	{
		RestoreSnapshot(Snap);
		if (UMeshComponent* Mesh = Snap.Component.Get())
		{
			if (AActor* Owner = Mesh->GetOwner()) AffectedActors.Add(Owner);
		}
	}
	const FName CompTag = MakeVizComponentTag(Config->ActorTag);
	for (AActor* Actor : AffectedActors) RemoveTextRendersFromActor(Actor, CompTag);

	const int32 Count = AffectedActors.Num();
	ActiveStates.Remove(Config->ActorTag);
	BroadcastHighlightChanged();
	return Count;
}

bool ULevelDesignVisualizerSubsystem::ToggleHighlight(ULevelDesignTagConfig* Config)
{
	if (!Config) return false;
	if (IsActive(Config)) { ClearHighlight(Config); return false; }
	HighlightActors(Config);
	return true;
}

void ULevelDesignVisualizerSubsystem::RefreshHighlight(ULevelDesignTagConfig* Config)
{
	if (!Config) return;
	// 활성일 때만 재적용. 비활성이면 no-op (OFF 상태 유지).
	if (IsActive(Config)) HighlightActors(Config);
}

bool ULevelDesignVisualizerSubsystem::IsActive(ULevelDesignTagConfig* Config) const
{
	return Config && ActiveStates.Contains(Config->ActorTag);
}

// ===========================================================================
// Per-Actor API
// ===========================================================================

bool ULevelDesignVisualizerSubsystem::IsActorHighlighted(
	AActor* Actor, ULevelDesignTagConfig* Config) const
{
	if (!IsValid(Actor) || !Config || Config->ActorTag.IsNone()) return false;
	const FLDVizTagState* State = ActiveStates.Find(Config->ActorTag);
	if (!State) return false;
	for (const FLDVizLabel& Label : State->Labels)
	{
		if (Label.OwnerActor.Get() == Actor) return true;
	}
	return false;
}

bool ULevelDesignVisualizerSubsystem::IsActorVisualizationEnabled(
	AActor* Actor, ULevelDesignTagConfig* Config) const
{
	if (!IsValid(Actor) || !Config || Config->ActorTag.IsNone()) return true;
	const FLDVizActorSet* Set = DisabledActorsByTag.Find(Config->ActorTag);
	if (!Set) return true;
	for (const TWeakObjectPtr<AActor>& WP : Set->Actors)
	{
		if (WP.Get() == Actor) return false;
	}
	return true;
}

void ULevelDesignVisualizerSubsystem::SetActorHighlighted(
	AActor* Actor, ULevelDesignTagConfig* Config, bool bEnabled)
{
	if (!IsValid(Actor) || !Config || Config->ActorTag.IsNone()) return;

	FLDVizActorSet& Set = DisabledActorsByTag.FindOrAdd(Config->ActorTag);

	// 1) 사용자 의도 갱신 (disabled 목록)
	if (bEnabled)
	{
		for (int32 i = Set.Actors.Num() - 1; i >= 0; --i)
		{
			if (!Set.Actors[i].IsValid() || Set.Actors[i].Get() == Actor)
			{
				Set.Actors.RemoveAt(i);
			}
		}
	}
	else
	{
		bool bFound = false;
		for (const TWeakObjectPtr<AActor>& WP : Set.Actors)
		{
			if (WP.Get() == Actor) { bFound = true; break; }
		}
		if (!bFound) Set.Actors.Add(Actor);
	}

	// 2) Config 가 활성일 때만 즉시 시각화 반영
	FLDVizTagState* State = ActiveStates.Find(Config->ActorTag);
	if (State)
	{
		const bool bCurrentlyApplied = IsActorHighlighted(Actor, Config);

		if (bEnabled && !bCurrentlyApplied)
		{
			ApplyToActor(Actor, Config, *State);
		}
		else if (!bEnabled && bCurrentlyApplied)
		{
			RemoveActorFromState(Actor, *State, Config->ActorTag);
			// 모든 액터가 disabled 되어 State 가 비어도 ActiveStates 에서 제거하지 않음
			// (Config 의 ON 상태는 사용자가 명시적으로 토글한 결과이므로 유지)
		}
	}

	BroadcastHighlightChanged();
}

// ===========================================================================
// Global API
// ===========================================================================

bool ULevelDesignVisualizerSubsystem::IsAnyConfigActive() const
{
	return !ActiveStates.IsEmpty();
}

void ULevelDesignVisualizerSubsystem::ToggleAllHighlights(ULevelDesignTagConfig* Config)
{
	if (!Config || Config->ActorTag.IsNone()) return;
	const FName Tag = Config->ActorTag;

	if (SuspendedDisabledByTag.Contains(Tag))
	{
		// === 복원 모드: highlight OFF + 체크박스 상태 원래대로 ===
		if (IsActive(Config))
		{
			ClearHighlight(Config);
		}

		FLDVizActorSet Restored = SuspendedDisabledByTag.FindAndRemoveChecked(Tag);
		if (Restored.Actors.Num() > 0)
		{
			DisabledActorsByTag.Add(Tag, MoveTemp(Restored));
		}
	}
	else
	{
		// === 진입 모드: 현재 체크박스 OFF 기록 백업 + 강제 모두 ON ===
		FLDVizActorSet ToBackup;
		if (FLDVizActorSet* Existing = DisabledActorsByTag.Find(Tag))
		{
			ToBackup = MoveTemp(*Existing);
			DisabledActorsByTag.Remove(Tag);
		}
		// 빈 set 이라도 추가 → 다음번 클릭에서 "이 태그가 강제 모드 중" 판정 가능
		SuspendedDisabledByTag.Add(Tag, MoveTemp(ToBackup));

		if (IsActive(Config))
		{
			ClearHighlight(Config);
		}
		HighlightActors(Config);
	}

	BroadcastHighlightChanged();
}

void ULevelDesignVisualizerSubsystem::ForceToggleAllHighlights()
{
	// "강제 모드 중"인지: 백업 슬롯에 항목이 하나라도 있으면 모드 진행 중으로 간주
	const bool bInForceMode = SuspendedDisabledByTag.Num() > 0;

	if (bInForceMode)
	{
		// === 복원 모드: 모두 OFF + 모든 백업 복원 ===
		if (IsAnyConfigActive())
		{
			ClearAll();
		}

		for (auto& Pair : SuspendedDisabledByTag)
		{
			if (Pair.Value.Actors.Num() > 0)
			{
				DisabledActorsByTag.Add(Pair.Key, MoveTemp(Pair.Value));
			}
		}
		SuspendedDisabledByTag.Empty();
	}
	else
	{
		// === 진입 모드: 현재 모든 DisabledActorsByTag 백업 + 모든 Config ON ===
		SuspendedDisabledByTag = MoveTemp(DisabledActorsByTag);
		DisabledActorsByTag.Empty();

		// "ForceAll 모드 중" 마커가 필요하니, 백업이 비어있던 태그들도 빈 set 으로 표시.
		// (사용자가 아무도 체크박스 OFF 안 했을 때를 위한 처리)
		const TArray<ULevelDesignTagConfig*> AllConfigs = GetAllConfigs();
		for (const ULevelDesignTagConfig* Cfg : AllConfigs)
		{
			if (Cfg && !Cfg->ActorTag.IsNone() && !SuspendedDisabledByTag.Contains(Cfg->ActorTag))
			{
				SuspendedDisabledByTag.Add(Cfg->ActorTag, FLDVizActorSet{});
			}
		}

		if (IsAnyConfigActive())
		{
			ClearAll();
		}
		for (ULevelDesignTagConfig* Cfg : AllConfigs)
		{
			HighlightActors(Cfg);
		}
	}

	BroadcastHighlightChanged();
}

// ===========================================================================
// Lookup / Focus
// ===========================================================================

TArray<AActor*> ULevelDesignVisualizerSubsystem::FindActors(ULevelDesignTagConfig* Config) const
{
	TArray<AActor*> Result;
	if (!Config || Config->ActorTag.IsNone()) return Result;
	UWorld* World = GetEditorWorld();
	if (!World) return Result;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (IsValid(Actor) && Actor->ActorHasTag(Config->ActorTag)) Result.Add(Actor);
	}
	return Result;
}

TArray<ULevelDesignTagConfig*> ULevelDesignVisualizerSubsystem::GetAllConfigs() const
{
	TArray<ULevelDesignTagConfig*> Result;
	FAssetRegistryModule& Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = Module.Get();

	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(ULevelDesignTagConfig::StaticClass()->GetClassPathName(), Assets, true);

	Result.Reserve(Assets.Num());
	for (const FAssetData& Asset : Assets)
	{
		if (ULevelDesignTagConfig* Config = Cast<ULevelDesignTagConfig>(Asset.GetAsset()))
		{
			Result.Add(Config);
		}
	}
	Result.Sort([](const ULevelDesignTagConfig& A, const ULevelDesignTagConfig& B)
	{
		const FString L = A.DisplayName.IsEmpty() ? A.ActorTag.ToString() : A.DisplayName.ToString();
		const FString R = B.DisplayName.IsEmpty() ? B.ActorTag.ToString() : B.DisplayName.ToString();
		return L < R;
	});
	return Result;
}

void ULevelDesignVisualizerSubsystem::FocusOnActor(AActor* Actor)
{
	if (!IsValid(Actor) || !GEditor) return;
	if (UEditorActorSubsystem* ActorSub = GEditor->GetEditorSubsystem<UEditorActorSubsystem>())
	{
		TArray<AActor*> Selection{ Actor };
		ActorSub->SetSelectedLevelActors(Selection);
	}
	GEditor->MoveViewportCamerasToActor(*Actor, false);
	CameraInterps.Empty();
}

void ULevelDesignVisualizerSubsystem::ToggleFocusOnActor(AActor* Actor)
{
	if (!IsValid(Actor) || !GEditor) return;

	USelection* Sel = GEditor->GetSelectedActors();
	const bool bAlreadySelected = Sel && Sel->IsSelected(Actor);

	if (!bAlreadySelected) { FocusOnActor(Actor); return; }

	GEditor->SelectNone(true, true, false);

	for (FEditorViewportClient* VC : GEditor->GetAllViewportClients())
	{
		if (!VC || !VC->IsPerspective()) continue;
		const FVector ActorLoc = Actor->GetActorLocation();
		const FVector CamLoc   = VC->GetViewLocation();
		FVector ToCam = CamLoc - ActorLoc;
		const float Dist = ToCam.Size();
		if (Dist > KINDA_SMALL_NUMBER)
		{
			ToCam /= Dist;
			const float NewDist = FMath::Max(Dist * 2.0f, 800.f);
			const FVector EndLoc = ActorLoc + ToCam * NewDist;
			StartCameraInterp(VC, EndLoc, 0.25f);
		}
	}
}

AActor* ULevelDesignVisualizerSubsystem::GetCurrentSelectedActor() const
{
	if (!GEditor) return nullptr;
	if (USelection* Sel = GEditor->GetSelectedActors()) return Sel->GetTop<AActor>();
	return nullptr;
}

// ===========================================================================
// 기타
// ===========================================================================

void ULevelDesignVisualizerSubsystem::ClearAll()
{
	if (ActiveStates.IsEmpty()) return;

	for (const auto& Pair : ActiveStates)
	{
		const FName Tag = Pair.Key;
		const FLDVizTagState& State = Pair.Value;
		const FName CompTag = MakeVizComponentTag(Tag);

		TSet<AActor*> AffectedActors;
		for (const FLDVizComponentSnapshot& Snap : State.Snapshots)
		{
			RestoreSnapshot(Snap);
			if (UMeshComponent* Mesh = Snap.Component.Get())
			{
				if (AActor* Owner = Mesh->GetOwner()) AffectedActors.Add(Owner);
			}
		}
		for (AActor* Actor : AffectedActors) RemoveTextRendersFromActor(Actor, CompTag);
	}
	ActiveStates.Empty();

	// ※ DisabledActorsByTag 는 비우지 않음 → 다음에 다시 켤 때 체크박스 상태 유지
	BroadcastHighlightChanged();
}

void ULevelDesignVisualizerSubsystem::SetBillboardEnabled(bool bEnabled)
{
	bBillboardEnabled = bEnabled;
	if (!bBillboardEnabled)
	{
		for (auto& Pair : ActiveStates)
		{
			for (FLDVizLabel& Label : Pair.Value.Labels)
			{
				if (UTextRenderComponent* Text = Label.Component.Get())
				{
					Text->SetWorldRotation(FRotator::ZeroRotator);
				}
			}
		}
	}
}

void ULevelDesignVisualizerSubsystem::SetLabelSizeMultiplier(float Multiplier)
{
	LabelSizeMultiplier = FMath::Max(0.01f, Multiplier);
	ApplySizeToAllLabels();
}

void ULevelDesignVisualizerSubsystem::ApplySizeToAllLabels()
{
	for (auto& Pair : ActiveStates)
	{
		for (FLDVizLabel& Label : Pair.Value.Labels)
		{
			if (UTextRenderComponent* Text = Label.Component.Get())
			{
				Text->SetWorldSize(Label.BaseWorldSize * LabelSizeMultiplier);
			}
		}
	}
}

TArray<FName> ULevelDesignVisualizerSubsystem::GetActorExtraTags(AActor* Actor, FName ExcludeTag) const
{
	TArray<FName> Out;
	if (!IsValid(Actor)) return Out;
	for (const FName& Tag : Actor->Tags)
	{
		if (!Tag.IsNone() && Tag != ExcludeTag) Out.Add(Tag);
	}
	return Out;
}

FText ULevelDesignVisualizerSubsystem::GetActorExtraTagsLabel(AActor* Actor, FName ExcludeTag) const
{
	if (!IsValid(Actor)) return FText::GetEmpty();
	TArray<FString> Parts;
	for (const FName& Tag : Actor->Tags)
	{
		if (!Tag.IsNone() && Tag != ExcludeTag) Parts.Add(Tag.ToString());
	}
	if (Parts.Num() == 0) return FText::GetEmpty();
	return FText::FromString(FString::Printf(TEXT("[%s]"), *FString::Join(Parts, TEXT(", "))));
}

bool ULevelDesignVisualizerSubsystem::TryGetEditorCameraLocation(FVector& OutLocation) const
{
	if (!GEditor) return false;
	FEditorViewportClient* Fallback = nullptr;
	for (FEditorViewportClient* VC : GEditor->GetAllViewportClients())
	{
		if (!VC || !VC->IsPerspective()) continue;
		if (VC->Viewport && VC->Viewport->HasFocus())
		{
			OutLocation = VC->GetViewLocation();
			return true;
		}
		if (!Fallback) Fallback = VC;
	}
	if (Fallback) { OutLocation = Fallback->GetViewLocation(); return true; }
	return false;
}

bool ULevelDesignVisualizerSubsystem::ComputeMeshBoundsForActor(
	AActor* Actor, FVector& OutOrigin, FVector& OutExtent) const
{
	if (!IsValid(Actor)) return false;
	TArray<UMeshComponent*> Meshes;
	Actor->GetComponents<UMeshComponent>(Meshes);

	FBox CombinedBox(ForceInitToZero);
	bool bAnyValid = false;
	for (UMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh) || !Mesh->IsVisible()) continue;
		const FBox MeshBox = Mesh->Bounds.GetBox();
		if (!bAnyValid) { CombinedBox = MeshBox; bAnyValid = true; }
		else CombinedBox += MeshBox;
	}
	if (!bAnyValid) return false;
	OutOrigin = CombinedBox.GetCenter();
	OutExtent = CombinedBox.GetExtent();
	return true;
}

// ===========================================================================
// Apply / restore
// ===========================================================================

void ULevelDesignVisualizerSubsystem::ApplyToActor(
	AActor* Actor, const ULevelDesignTagConfig* Config, FLDVizTagState& OutState)
{
	if (!IsValid(Actor) || !Config) return;

	UMaterialInterface* HiMat = Config->OverlayMaterial.LoadSynchronous();

	TArray<UMeshComponent*> MeshComps;
	Actor->GetComponents<UMeshComponent>(MeshComps);

	for (UMeshComponent* Mesh : MeshComps)
	{
		if (!IsValid(Mesh)) continue;

		FLDVizComponentSnapshot Snap;
		Snap.Component   = Mesh;
		Snap.AppliedMode = Config->HighlightMode;

		if (Config->HighlightMode == ELDVizHighlightMode::SlotReplace)
		{
			const int32 NumSlots = Mesh->GetNumMaterials();
			Snap.OriginalSlotMaterials.Reserve(NumSlots);
			for (int32 i = 0; i < NumSlots; ++i) Snap.OriginalSlotMaterials.Add(Mesh->GetMaterial(i));
			for (int32 i = 0; i < NumSlots; ++i) Mesh->SetMaterial(i, HiMat);
		}
		else
		{
			Snap.OriginalOverlay = Mesh->GetOverlayMaterial();
			Mesh->SetOverlayMaterial(HiMat);
		}
		OutState.Snapshots.Add(Snap);
	}

	const FName CompTag = MakeVizComponentTag(Config->ActorTag);
	{
		TArray<UTextRenderComponent*> ExistingTexts;
		Actor->GetComponents<UTextRenderComponent>(ExistingTexts);
		for (UTextRenderComponent* Text : ExistingTexts)
		{
			if (Text && Text->ComponentHasTag(CompTag)) Text->DestroyComponent();
		}
	}

	USceneComponent* Root = Actor->GetRootComponent();
	if (!Root) return;

	UTextRenderComponent* TextComp = NewObject<UTextRenderComponent>(Actor, NAME_None, RF_Transient);
	if (!TextComp) return;

	TextComp->ComponentTags.Add(CompTag);
	TextComp->ComponentTags.Add(TEXT("LDViz"));

#if WITH_EDITOR
	TextComp->SetText(FText::FromString(Actor->GetActorLabel()));
#else
	TextComp->SetText(FText::FromString(Actor->GetName()));
#endif

	TextComp->SetTextRenderColor(Config->LabelColor.ToFColor(true));
	TextComp->SetHorizontalAlignment(EHTA_Center);
	TextComp->SetVerticalAlignment(EVRTA_TextCenter);

	TextComp->SetupAttachment(Root);
	TextComp->RegisterComponent();
	TextComp->SetUsingAbsoluteRotation(true);
	TextComp->SetUsingAbsoluteScale(true);
	TextComp->SetWorldScale3D(FVector::OneVector);
	TextComp->SetWorldSize(Config->LabelWorldSize * LabelSizeMultiplier);

	FLDVizLabel Label;
	Label.Component     = TextComp;
	Label.OwnerActor    = Actor;
	Label.BaseWorldSize = Config->LabelWorldSize;
	Label.HeightOffset  = Config->LabelHeightOffset;
	OutState.Labels.Add(Label);
}

void ULevelDesignVisualizerSubsystem::RestoreSnapshot(const FLDVizComponentSnapshot& Snap)
{
	UMeshComponent* Mesh = Snap.Component.Get();
	if (!Mesh) return;

	if (Snap.AppliedMode == ELDVizHighlightMode::SlotReplace)
	{
		const int32 Num = Snap.OriginalSlotMaterials.Num();
		for (int32 i = 0; i < Num; ++i) Mesh->SetMaterial(i, Snap.OriginalSlotMaterials[i]);
	}
	else
	{
		Mesh->SetOverlayMaterial(Snap.OriginalOverlay);
	}
}

void ULevelDesignVisualizerSubsystem::RemoveActorFromState(
	AActor* Actor, FLDVizTagState& State, FName ActorTag)
{
	if (!IsValid(Actor)) return;

	for (int32 i = State.Snapshots.Num() - 1; i >= 0; --i)
	{
		UMeshComponent* Mesh = State.Snapshots[i].Component.Get();
		if (!Mesh || Mesh->GetOwner() == Actor)
		{
			if (Mesh) RestoreSnapshot(State.Snapshots[i]);
			State.Snapshots.RemoveAt(i);
		}
	}
	for (int32 i = State.Labels.Num() - 1; i >= 0; --i)
	{
		if (State.Labels[i].OwnerActor.Get() == Actor) State.Labels.RemoveAt(i);
	}
	RemoveTextRendersFromActor(Actor, MakeVizComponentTag(ActorTag));
}

void ULevelDesignVisualizerSubsystem::RemoveTextRendersFromActor(AActor* Actor, FName VizComponentTag)
{
	if (!IsValid(Actor) || VizComponentTag.IsNone()) return;
	TArray<UTextRenderComponent*> TextComps;
	Actor->GetComponents<UTextRenderComponent>(TextComps);
	for (UTextRenderComponent* Text : TextComps)
	{
		if (IsValid(Text) && Text->ComponentHasTag(VizComponentTag)) Text->DestroyComponent();
	}
}

UWorld* ULevelDesignVisualizerSubsystem::GetEditorWorld() const
{
	return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
}

FName ULevelDesignVisualizerSubsystem::MakeVizComponentTag(FName ActorTag)
{
	if (ActorTag.IsNone()) return NAME_None;
	return FName(*FString::Printf(TEXT("LDViz_%s"), *ActorTag.ToString()));
}