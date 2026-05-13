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

	// 복제 처리
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

// ===========================================================================
// 복제 처리 — 핵심 로직
// ===========================================================================

void ULevelDesignVisualizerSubsystem::OnDuplicateActorsBegin()
{
	// 복제 직전: 어떤 Config 가 활성이었는지 기억해두고, 모든 시각화 해제
	// → 원본 액터의 머티리얼/TextRender 가 깨끗하게 복원된 상태로 복제됨
	PendingDuplicateReapply.Empty();
	for (auto& Pair : ActiveStates)
	{
		if (ULevelDesignTagConfig* Cfg = Pair.Value.ConfigRef.Get())
		{
			PendingDuplicateReapply.Add(Cfg);
		}
	}

	if (PendingDuplicateReapply.Num() > 0)
	{
		UE_LOG(LogLDViz, Log, TEXT("[LDViz] Duplicate begin — clearing %d active state(s)."),
			PendingDuplicateReapply.Num());
		ClearAll();
	}
}

void ULevelDesignVisualizerSubsystem::OnDuplicateActorsEnd()
{
	if (PendingDuplicateReapply.Num() == 0) return;

	UE_LOG(LogLDViz, Log, TEXT("[LDViz] Duplicate end — reapplying %d state(s)."),
		PendingDuplicateReapply.Num());

	for (TWeakObjectPtr<ULevelDesignTagConfig>& CfgPtr : PendingDuplicateReapply)
	{
		if (ULevelDesignTagConfig* Cfg = CfgPtr.Get())
		{
			HighlightActors(Cfg); // 원본+복제본 모두 새로 캡처
		}
	}
	PendingDuplicateReapply.Empty();
}

// ===========================================================================
// Tick
// ===========================================================================

void ULevelDesignVisualizerSubsystem::Tick(float DeltaTime)
{
	// 1) 카메라 보간
	if (CameraInterps.Num() > 0)
	{
		TickCameraInterps(DeltaTime);
	}

	// 2) 라벨 빌보드
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
	// 시각화 활성 중이거나 카메라 보간 진행 중이면 틱
	return !ActiveStates.IsEmpty() || CameraInterps.Num() > 0;
}

// ===========================================================================
// 카메라 보간
// ===========================================================================

void ULevelDesignVisualizerSubsystem::StartCameraInterp(
	FEditorViewportClient* VC, const FVector& EndLoc, float Duration)
{
	if (!VC) return;

	// 같은 뷰포트에 진행 중인 보간이 있으면 갱신 (덮어쓰기)
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
	// 살아있는 뷰포트 목록 (raw 포인터 안전성 체크)
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
		// Ease-in-out: 줌인과 비슷한 부드러운 느낌
		Alpha = FMath::SmoothStep(0.f, 1.f, Alpha);

		const FVector CurLoc = FMath::Lerp(Interp.StartLoc, Interp.EndLoc, Alpha);
		Interp.Viewport->SetViewLocation(CurLoc);
		Interp.Viewport->Invalidate();

		if (Interp.Elapsed >= Interp.Duration)
		{
			CameraInterps.RemoveAt(i);
		}
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
}

void ULevelDesignVisualizerSubsystem::OnPreSaveWorld(UWorld*, FObjectPreSaveContext)
{
	if (!ActiveStates.IsEmpty())
	{
		UE_LOG(LogLDViz, Log, TEXT("[LDViz] Auto-clearing before save."));
		ClearAll();
	}
}

// ===========================================================================
// Highlight / clear
// ===========================================================================

int32 ULevelDesignVisualizerSubsystem::HighlightActors(ULevelDesignTagConfig* Config)
{
	if (!Config || Config->ActorTag.IsNone()) return 0;

	if (ActiveStates.Contains(Config->ActorTag))
	{
		ClearHighlight(Config);
	}

	const TArray<AActor*> Actors = FindActors(Config);
	if (Actors.Num() == 0) return 0;

	FLDVizTagState NewState;
	NewState.ConfigRef = Config; // 복제/Refresh 시 다시 찾을 수 있도록
	NewState.Snapshots.Reserve(Actors.Num() * 2);
	NewState.Labels.Reserve(Actors.Num());

	for (AActor* Actor : Actors)
	{
		ApplyToActor(Actor, Config, NewState);
	}

	ActiveStates.Add(Config->ActorTag, MoveTemp(NewState));
	UE_LOG(LogLDViz, Log, TEXT("[LDViz] Highlighted %d actors for '%s'."),
		Actors.Num(), *Config->ActorTag.ToString());
	return Actors.Num();
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

	// 활성 상태일 때만 재적용 (ON 상태 유지)
	// 비활성이면 아무 시각적 변경 없음 (OFF 상태 유지)
	if (IsActive(Config))
	{
		HighlightActors(Config); // 내부에서 Clear → ReApply
	}
}

bool ULevelDesignVisualizerSubsystem::IsActive(ULevelDesignTagConfig* Config) const
{
	return Config && ActiveStates.Contains(Config->ActorTag);
}

TArray<AActor*> ULevelDesignVisualizerSubsystem::FindActors(ULevelDesignTagConfig* Config) const
{
	TArray<AActor*> Result;
	if (!Config || Config->ActorTag.IsNone()) return Result;
	UWorld* World = GetEditorWorld();
	if (!World) return Result;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (IsValid(Actor) && Actor->ActorHasTag(Config->ActorTag))
		{
			Result.Add(Actor);
		}
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

// ===========================================================================
// Focus
// ===========================================================================

void ULevelDesignVisualizerSubsystem::FocusOnActor(AActor* Actor)
{
	if (!IsValid(Actor) || !GEditor) return;
	if (UEditorActorSubsystem* ActorSub = GEditor->GetEditorSubsystem<UEditorActorSubsystem>())
	{
		TArray<AActor*> Selection{ Actor };
		ActorSub->SetSelectedLevelActors(Selection);
	}
	GEditor->MoveViewportCamerasToActor(*Actor, false);

	// 줌인이 시작된 경우, 진행 중이던 줌아웃 보간은 취소 (충돌 방지)
	CameraInterps.Empty();
}

void ULevelDesignVisualizerSubsystem::ToggleFocusOnActor(AActor* Actor)
{
	if (!IsValid(Actor) || !GEditor) return;

	USelection* Sel = GEditor->GetSelectedActors();
	const bool bAlreadySelected = Sel && Sel->IsSelected(Actor);

	if (!bAlreadySelected)
	{
		FocusOnActor(Actor);
		return;
	}

	// 선택 해제
	GEditor->SelectNone(true, true, false);

	// 부드럽게 줌아웃: 각 perspective 뷰포트의 위치를 액터에서 멀어지는 방향으로 보간
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
	if (USelection* Sel = GEditor->GetSelectedActors())
	{
		return Sel->GetTop<AActor>();
	}
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
		if (!Tag.IsNone() && Tag != ExcludeTag)
		{
			Parts.Add(Tag.ToString());
		}
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
			for (int32 i = 0; i < NumSlots; ++i)
			{
				Snap.OriginalSlotMaterials.Add(Mesh->GetMaterial(i));
			}
			for (int32 i = 0; i < NumSlots; ++i)
			{
				Mesh->SetMaterial(i, HiMat);
			}
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