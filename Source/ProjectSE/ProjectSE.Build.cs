// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectSE : ModuleRules
{
	public ProjectSE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			// --- 언리얼 기본 필수 모듈 ---
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			// --- AI 및 내비게이션 ---
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			// --- 시각 효과 및 UI ---
			"Niagara",
			"UMG",
			"Slate",
			// --- 게임플레이 프레임워크 (데이터 주도 설계) ---
			"GameplayMessageRuntime", // GMS (이벤트 기반 통신 시스템)
			"GameplayTags",           // 태그 기반 상태 및 식별자 관리
			"GameplayAbilities",      // GAS - 어빌리티 시스템 코어
			"GameplayTasks",          // GAS - 비동기 태스크 처리
			// --- 애니메이션 및 모션 매칭 (GASP 관련) ---
			"MotionWarping",                      // 트래버설(파쿠르 등) 시 목표 지점으로 캐릭터를 끌어당기는 컴포넌트
			"AnimationWarpingRuntime",            // 오리엔테이션 워핑, 스트라이드 워핑 등 애니메이션 보정 기능
          
			"AnimationLocomotionLibraryRuntime",  // GASP에서 사용하는 모션 매칭 보조 라이브러리
			"PoseSearch",                         // 데이터베이스에서 알맞은 애니메이션 포즈를 검색하는 모듈
			"Chooser",                            // 상황에 맞는 애니메이션 데이터베이스나 로직을 선택하는 플러그인
			"MotionTrajectory"          // UCharacterTrajectoryComponent를 C++에서 생성하고 제어하기 위해 필수적인 모듈
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ProjectSE",
			"ProjectSE/Character",
			"ProjectSE/Variant_Strategy",
			"ProjectSE/Variant_Strategy/UI",
			"ProjectSE/Variant_TwinStick",
			"ProjectSE/Variant_TwinStick/AI",
			"ProjectSE/Variant_TwinStick/Gameplay",
			"ProjectSE/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
