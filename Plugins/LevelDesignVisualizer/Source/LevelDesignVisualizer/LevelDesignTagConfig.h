#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelDesignTagConfig.generated.h"

class UMaterialInterface;

/**
 * 메시에 강조를 적용하는 방식.
 */
UENUM(BlueprintType)
enum class ELDVizHighlightMode : uint8
{
	/** 원본 머티리얼 위에 한 패스 더 그리기. 외형 유지. 어두운/채도 높은 메시에선 묻힐 수 있음. */
	Overlay      UMETA(DisplayName = "Overlay (외형 유지, 약함)"),

	/** 메시의 모든 머티리얼 슬롯을 강조용으로 교체. 어떤 메시에서도 100% 보임. 외형 변경. */
	SlotReplace  UMETA(DisplayName = "Slot Replace (강력, 외형 변경)"),
};

/**
 * 레벨 디자인 시각화 툴의 "태그 + 시각 스타일" 설정.
 *
 * 새 태그(예: Climb, Slide, Vault)를 추가하려면 이 Data Asset 인스턴스를
 * 하나 더 만들기만 하면 됨.
 */
UCLASS(BlueprintType)
class LEVELDESIGNVISUALIZER_API ULevelDesignTagConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identification")
	FName ActorTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identification")
	FText DisplayName;

	/**
	 * 강조 방식. 색이 있는 메시까지 확실히 강조하려면 SlotReplace 권장.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	ELDVizHighlightMode HighlightMode = ELDVizHighlightMode::SlotReplace;

	/**
	 * 강조 머티리얼.
	 *  - Overlay 모드: OverlayMaterial 로 사용 (Translucent + 강한 Emissive 권장)
	 *  - SlotReplace 모드: 모든 슬롯에 적용 (Unlit + Emissive 단순 머티리얼이 깔끔)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UMaterialInterface> OverlayMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor LabelColor = FLinearColor::Yellow;

	/** 액터 **bounding box 윗면**으로부터의 추가 Z 오프셋(cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.0"))
	float LabelHeightOffset = 50.f;

	/** 라벨 텍스트 월드 사이즈 (배수 곱해지기 전 기준값) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "1.0"))
	float LabelWorldSize = 50.f;
};