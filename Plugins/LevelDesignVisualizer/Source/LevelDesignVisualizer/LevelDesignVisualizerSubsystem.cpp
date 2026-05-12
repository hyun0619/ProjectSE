#include "LevelDesignVisualizerSubsystem.h"
#include "LevelDesignTagConfig.h"

#include "Editor.h"
#include "EngineUtils.h"
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

	// 레벨 저장 직전 자동 정리 → .umap 에 시각화가 박히지 않도록
	PreSaveWorldHandle = FEditorDelegates::PreSaveWorldWithContext.AddUObject(
		this, &ULevelDesignVisualizerSubsystem::OnPreSaveWorld);

	// 맵 전환 시 잔여 상태(GC된 weak ptr 들) 청소
	MapOpenedHandle = FEditorDelegates::OnMapOpened.AddUObject(
		this, &ULevelDesignVisualizerSubsystem::OnMapOpened);
}

void ULevelDesignVisualizerSubsystem::Deinitialize()
{
	FEditorDelegates::PreSaveWorldWithContext.Remove(PreSaveWorldHandle);
	FEditorDelegates::OnMapOpened.Remove(MapOpenedHandle);

	ClearAll();

	Super::Deinitialize();
}

void ULevelDesignVisualizerSubsystem::OnMapOpened(const FString& /*Filename*/, bool /*bAsTemplate*/)
{
	// 이전 맵 액터들은 이미 GC됨. 우리 맵만 비우면 됨.
	ActiveStates.Empty();
}

void ULevelDesignVisualizerSubsystem::OnPreSaveWorld(UWorld* /*World*/, FObjectPreSaveContext /*Context*/)
{
	if (!ActiveStates.IsEmpty())
	{
		UE_LOG(LogLDViz, Log, TEXT("[LDViz] Auto-clearing visualizer state before world save."));
		ClearAll();
	}
}

// ===========================================================================
// Public API
// ===========================================================================

int32 ULevelDesignVisualizerSubsystem::HighlightActors(ULevelDesignTagConfig* Config)
{
	if (!Config || Config->ActorTag.IsNone())
	{
		UE_LOG(LogLDViz, Warning, TEXT("[LDViz] HighlightActors: invalid Config or empty ActorTag."));
		return 0;
	}

	// 이미 ON이면 한 번 끄고 다시 켜기(=Refresh): 액터가 추가/삭제된 상황 대응
	if (ActiveStates.Contains(Config->ActorTag))
	{
		ClearHighlight(Config);
	}

	const TArray<AActor*> Actors = FindActors(Config);
	if (Actors.Num() == 0)
	{
		UE_LOG(LogLDViz, Log, TEXT("[LDViz] No actors found with tag '%s'."), *Config->ActorTag.ToString());
		return 0;
	}

	FLDVizTagState NewState;
	NewState.Snapshots.Reserve(Actors.Num() * 2); // 평균 메시 개수 추정
	for (AActor* Actor : Actors)
	{
		ApplyToActor(Actor, Config, NewState);
	}

	ActiveStates.Add(Config->ActorTag, MoveTemp(NewState));

	UE_LOG(LogLDViz, Log, TEXT("[LDViz] Highlighted %d actors for tag '%s'."),
		Actors.Num(), *Config->ActorTag.ToString());
	return Actors.Num();
}

int32 ULevelDesignVisualizerSubsystem::ClearHighlight(ULevelDesignTagConfig* Config)
{
	if (!Config || Config->ActorTag.IsNone()) return 0;

	const FLDVizTagState* State = ActiveStates.Find(Config->ActorTag);
	if (!State) return 0;

	// 1) Overlay 머티리얼 복원
	TSet<AActor*> AffectedActors;
	for (const FLDVizComponentSnapshot& Snap : State->Snapshots)
	{
		UMeshComponent* Mesh = Snap.Component.Get();
		if (!Mesh) continue;

		Mesh->SetOverlayMaterial(Snap.OriginalOverlay);
		if (AActor* Owner = Mesh->GetOwner())
		{
			AffectedActors.Add(Owner);
		}
	}

	// 2) 우리가 부착한 TextRender 만 정확히 제거 (ComponentTag 매칭)
	const FName CompTag = MakeVizComponentTag(Config->ActorTag);
	for (AActor* Actor : AffectedActors)
	{
		RemoveTextRendersFromActor(Actor, CompTag);
	}

	const int32 Count = AffectedActors.Num();
	ActiveStates.Remove(Config->ActorTag);

	UE_LOG(LogLDViz, Log, TEXT("[LDViz] Cleared %d actors for tag '%s'."),
		Count, *Config->ActorTag.ToString());
	return Count;
}

bool ULevelDesignVisualizerSubsystem::ToggleHighlight(ULevelDesignTagConfig* Config)
{
	if (!Config) return false;

	if (IsActive(Config))
	{
		ClearHighlight(Config);
		return false;
	}
	HighlightActors(Config);
	return true;
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

	FAssetRegistryModule& Module =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = Module.Get();

	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(
		ULevelDesignTagConfig::StaticClass()->GetClassPathName(),
		Assets,
		/*bSearchSubClasses=*/ true);

	Result.Reserve(Assets.Num());
	for (const FAssetData& Asset : Assets)
	{
		if (ULevelDesignTagConfig* Config = Cast<ULevelDesignTagConfig>(Asset.GetAsset()))
		{
			Result.Add(Config);
		}
	}

	// DisplayName 알파벳순(없으면 ActorTag 기준)
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

	// false = 활성 뷰포트만이 아니라 모든 퍼스펙티브 뷰포트를 이동
	GEditor->MoveViewportCamerasToActor(*Actor, /*bActiveViewportOnly=*/ false);
}

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
			UMeshComponent* Mesh = Snap.Component.Get();
			if (!Mesh) continue;

			Mesh->SetOverlayMaterial(Snap.OriginalOverlay);
			if (AActor* Owner = Mesh->GetOwner())
			{
				AffectedActors.Add(Owner);
			}
		}

		for (AActor* Actor : AffectedActors)
		{
			RemoveTextRendersFromActor(Actor, CompTag);
		}
	}

	ActiveStates.Empty();
}

// ===========================================================================
// Private helpers
// ===========================================================================

void ULevelDesignVisualizerSubsystem::ApplyToActor(
	AActor* Actor,
	const ULevelDesignTagConfig* Config,
	FLDVizTagState& OutState)
{
	if (!IsValid(Actor) || !Config) return;

	// 1) 모든 MeshComponent 의 Overlay 머티리얼 적용 + 원본 보존
	//    Static/Skeletal 모두 처리되도록 부모 클래스 UMeshComponent 사용.
	UMaterialInterface* Overlay = Config->OverlayMaterial.LoadSynchronous();

	TArray<UMeshComponent*> MeshComps;
	Actor->GetComponents<UMeshComponent>(MeshComps);

	for (UMeshComponent* Mesh : MeshComps)
	{
		if (!IsValid(Mesh)) continue;

		FLDVizComponentSnapshot Snap;
		Snap.Component = Mesh;
		Snap.OriginalOverlay = Mesh->GetOverlayMaterial();
		OutState.Snapshots.Add(Snap);

		Mesh->SetOverlayMaterial(Overlay);
	}

	// 2) TextRender 부착 (액터당 1개, 중복 부착 방지)
	const FName CompTag = MakeVizComponentTag(Config->ActorTag);

	TArray<UTextRenderComponent*> ExistingTexts;
	Actor->GetComponents<UTextRenderComponent>(ExistingTexts);
	for (UTextRenderComponent* Text : ExistingTexts)
	{
		if (Text && Text->ComponentHasTag(CompTag))
		{
			return; // 이미 부착됨
		}
	}

	USceneComponent* Root = Actor->GetRootComponent();
	if (!Root) return;

	// RF_Transient: 레벨과 함께 저장되지 않음 → .umap 오염 방지
	UTextRenderComponent* TextComp =
		NewObject<UTextRenderComponent>(Actor, NAME_None, RF_Transient);
	if (!TextComp) return;

	TextComp->ComponentTags.Add(CompTag);
	TextComp->ComponentTags.Add(TEXT("LDViz")); // 보너스: 일괄 정리용 공통 태그

#if WITH_EDITOR
	TextComp->SetText(FText::FromString(Actor->GetActorLabel()));
#else
	TextComp->SetText(FText::FromString(Actor->GetName()));
#endif

	TextComp->SetTextRenderColor(Config->LabelColor.ToFColor(true));
	TextComp->SetWorldSize(Config->LabelWorldSize);
	TextComp->SetHorizontalAlignment(EHTA_Center);
	TextComp->SetVerticalAlignment(EVRTA_TextCenter);

	TextComp->SetupAttachment(Root);
	TextComp->RegisterComponent();
	TextComp->SetRelativeLocation(FVector(0.f, 0.f, Config->LabelHeightOffset));
}

void ULevelDesignVisualizerSubsystem::RemoveTextRendersFromActor(AActor* Actor, FName VizComponentTag)
{
	if (!IsValid(Actor) || VizComponentTag.IsNone()) return;

	TArray<UTextRenderComponent*> TextComps;
	Actor->GetComponents<UTextRenderComponent>(TextComps);

	for (UTextRenderComponent* Text : TextComps)
	{
		if (IsValid(Text) && Text->ComponentHasTag(VizComponentTag))
		{
			Text->DestroyComponent();
		}
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
