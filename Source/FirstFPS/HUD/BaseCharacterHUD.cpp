// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"

void ABaseCharacterHUD::BeginPlay()
{
	AddCharacterOverlay();
}

void ABaseCharacterHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass) {
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void ABaseCharacterHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize; // 游戏屏幕大小
	if (GEngine) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
		/** 每帧在屏幕中心绘制一个由 5 张纹理组成的动态准星, 并根据 HUDPackage.CrosshairSpread 控制每个准星片的偏移大小和方向 */
		if (HUDPackage.CrosshairsCenter) {
			FVector2D Spread(0.f, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, Spread);
		}
		if (HUDPackage.CrosshairsTop) {
			FVector2D Spread(0.f, -HUDPackage.CrosshairSpread);
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, Spread);
		}
		if (HUDPackage.CrosshairsBottom) {
			FVector2D Spread(0.f, HUDPackage.CrosshairSpread);
			DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, Spread);
		}
		if (HUDPackage.CrosshairsRight) {
			FVector2D Spread(HUDPackage.CrosshairSpread, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, Spread);
		}
		if (HUDPackage.CrosshairsLeft) {
			FVector2D Spread(-HUDPackage.CrosshairSpread, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, Spread);
		}
	}

}

void ABaseCharacterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread)
{
	/* 获取纹理贴图的宽度和高度 */
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	/* 根据屏幕中心坐标和给定Texture大小, 计算出Texture左上角应该绘制的位置(让绘制区域中心点对准屏幕中心!) */
	const FVector2D TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.f) + Spread.X, // Spread为偏移量
		ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y
	);

	/** AHUD::DrawTexture(): 把 Texture 放到游戏屏幕上的矩形区域中 */
	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextureHeight, 0.f, 0.f, 1.f, 1.f, FLinearColor::White);
}
