// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BaseCharacterHUD.generated.h"

class UTexture2D;
class UCharacterOverlay;
class UUserWidget;

/** 自定义结构体: 存储每帧要显示的准星纹理 */
USTRUCT(BlueprintType) // 该结构体类型 可以作为 Blueprint中的变量类型 使用
struct FHUDPackage {
	GENERATED_BODY()   // 声明为 USTRUCT(), 需通过 GENERATED_BODY() 自动生成相关所需代码
public:
	UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;
	float CrosshairSpread; // 准星通常不是固定的, 而是根据角色状态变化. 因此需要通过 CrosshairSpread 来设置"准星扩散值"
};

UCLASS()
class FIRSTFPS_API ABaseCharacterHUD : public AHUD
{
	GENERATED_BODY()
public:
	/** AHUD类的重载渲染函数, 每帧会被调用一次. 负责在屏幕上显示简单UI元素 */
	virtual void DrawHUD() override;
	void AddCharacterOverlay();
	UPROPERTY(EditAnywhere, Category = "Player States")
	TSubclassOf<UUserWidget> CharacterOverlayClass;
	UCharacterOverlay* CharacterOverlay;

protected:
	virtual void BeginPlay() override;

private:
	FHUDPackage HUDPackage;
	
	/* 函数内部通过调用 AHUD::DrawTexture() 绘制(各部分的)准星纹理 */
	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread);
public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};
