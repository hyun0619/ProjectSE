#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelDesignTagConfig.generated.h"

class UMaterialInterface;

/**
 * 레벨 디자인 시각화 툴에서 사용할 "태그 + 시각 스타일" 설정.
 *
 * 새 태그(예: Climb, Slide, Vault)를 추가하려면 이 Data Asset 인스턴스를
 * 하나 더 만들기만 하면 됨. EUW와 서브시스템은 Asset Registry로 자동 인식.
 */
UCLASS(BlueprintType)
class LEVELDESIGNVISUALIZER_API ULevelDesignTagConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 검색 대상 Actor Tag (예: "WallRun", "Climb"). FName 비교라서 빠름. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identification")
	FName ActorTag;

	/** EUW에 표시될 사람이 읽는 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identification")
	FText DisplayName;

	/**
	 * 메시에 적용할 Overlay 머티리얼.
	 * 5.x의 UMeshComponent::SetOverlayMaterial 을 사용하므로
	 * Static/Skeletal Mesh 모두 작동함.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UMaterialInterface> OverlayMaterial;

	/** 머리 위 라벨 텍스트 색상 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor LabelColor = FLinearColor::Yellow;

	/** 액터 루트로부터의 Z축 높이(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.0"))
	float LabelHeightOffset = 200.f;

	/** 라벨 텍스트 월드 사이즈 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "1.0"))
	float LabelWorldSize = 50.f;
};
