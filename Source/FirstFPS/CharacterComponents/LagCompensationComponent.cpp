// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "FirstFPS/Character/BaseCharacter.h"
#include "FirstFPS/Weapons/Weapon.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "FirstFPS/FirstFPS.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SaveFramePackage(); // 在Server端中记录FramePackage
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (Character == nullptr || !Character->HasAuthority()) return;
	if (FrameHistory.Num() <= 1) {
		// 保存当前帧信息
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
	else {
		// HistoryLength: FrameHistory中累计存储数据的总时长
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (HistoryLength > MaxRecordTime) {
			// 累计存储时长 超过 指定时长, 删除旧的帧信息, 直到低于指定时长
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		// 保存当前帧信息
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
	}
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	/** 已知两个历史帧 OlderFrame 和 YoungerFrame, 插值计算角色在 这两个历史帧之间某一时刻HitTime 的 HitBox状态 */

	const float Distance = YoungerFrame.Time - OlderFrame.Time; // OlderFrame 和 YoungerFrame 两帧之间的时间间隔
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f); // 插值系数 (HitTime 位于 Older → Younger 的百分比位置)

	/* FFramePackage, 保存某一时刻的各部位HitBoxes */
	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;
	for (auto& YoungerPair : YoungerFrame.HitBoxInfo) {
		// 根据HitBox名称, 找到 OlderFrame 和 YoungerFrame 中对应的 HitBox 
		const FName& BoxInfoName = YoungerPair.Key;
		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];
		// 根据两个历史帧HitBox, 插值计算 HitTime 的 HitBox
		FBoxInformation InterpBoxInfo;
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction); // VInterpTo(): Vector插值, 处理位置、方向向量等;
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction); // RInterpTo(): Rotator 插值, 处理旋转
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;
		// 将 该部位的HitBox信息(HitTime) 加入 InterpFramePackage
		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	bool bReturn = HitCharacter == nullptr || HitCharacter->GetLagCompensation() == nullptr ||
				   HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
				   HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;
	if (bReturn) return FServerSideRewindResult();
	
	FFramePackage FrameToCheck; // 用来验证"该帧是否击中"的 FFramePackage 变量
	bool bShouldInterpolate = true; // 除非刚好击中时间与帧历史数据记录时间完全相同(可能性极低), 否则都需要通过插值计算
	
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory; // const& History: 被击中角色的帧历史数据(常量引用)
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;
	if (OldestHistoryTime > HitTime) { // 延迟过高, 无法进行服务器回溯(帧历史数据中并没有这么久远的数据)
		return FServerSideRewindResult();
	}
	if (OldestHistoryTime == HitTime) { // 如果客户端要求的 HitTime == Tail.Time, 直接使用 Tail 即可
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	if (NewestHistoryTime <= HitTime) { // HitTime >= 最新历史帧时间, 直接使用 最新帧Head 即可
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}
	/* 若待检查帧历史数据不在头尾节点, 不断向下遍历链表 */
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;
	/* 最终需要找到: Older.Time < HitTime < Younger.Time, 然后用这两个 Frame 做插值 */
	while (Older->GetValue().Time > HitTime) { 
		if (Older->GetNextNode() == nullptr)
			break;
		Older = Older->GetNextNode();			// Older向历史更早的方向移动一帧
		if (Older->GetValue().Time > HitTime) { // 如果移动后的 Older 依然比 HitTime 早, 说明还没有越过 HitTime, 则当前节点作为 新的Younger
			Younger = Older;
		}
	}

	if (Older->GetValue().Time == HitTime) { // 极少数情况下, 刚好找到了 HitTime 对应的 Frame 
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}
	if (bShouldInterpolate) {				 // 多数情况下, 需要在 Younger 和 Older 之间插值 (帧间插值)
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}
	FrameToCheck.Character = HitCharacter;

	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation); // 返回击中结果
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = (Character == nullptr) ? Cast<ABaseCharacter>(GetOwner()) : Character; // GetOwner(): 返回拥有当前对象(Component)的Actor
	if (Character) {
		Package.Time = GetWorld()->GetTimeSeconds(); // 获取Server当前时间
		Package.Character = Character;
		for (auto& BoxPair : Character->HitCollisionBoxes) {
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();
			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr) return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	CacheBoxPositions(HitCharacter, CurrentFrame);								// 缓存当前帧的HitBoxes状态 (后续须复原到该状态)
	MoveBoxes(HitCharacter, Package);											// 把角色 HitBoxes 移回"HitTime"的历史状态 
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision); // 关闭 被击中角色HitCharacter 的 Mesh 碰撞 (后续会启用 HitBox 碰撞)

	// 启用 head部位 对应 HitBox 的碰撞 (先只检测Head是否击中)
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("head")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	// 对各部位的HitBox逐一进行 LineTrace
	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	UWorld* World = GetWorld();
	if (World) {
		World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);
		if (ConfirmHitResult.bBlockingHit) { // 第一次 Trace 只检测 Head
			/* 恢复角色原有的状态 (恢复原有HitBoxes状态、关闭HitBoxes碰撞、重新开启Mesh碰撞) */
			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{ true, true };
		}
		else { // 如果没有击中Head, 开启其他部位的HitBox 碰撞后再进行一次 LineTrace
			for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes) {
				if (HitBoxPair.Value != nullptr) {
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
				}
			}
			World->LineTraceSingleByChannel(ConfirmHitResult, TraceStart, TraceEnd, ECC_HitBox);
			if (ConfirmHitResult.bBlockingHit) {
				/* 恢复角色原有的状态 (恢复原有HitBoxes状态、关闭HitBoxes碰撞、重新开启Mesh碰撞) */
				ResetHitBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
				return FServerSideRewindResult{ true, false };
			}
		}
	}
	/* 恢复角色原有的状态 (恢复原有HitBoxes状态、关闭HitBoxes碰撞、重新开启Mesh碰撞) */
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{ false, false };  // 两次LineTrace都没命中, 说明本次命中失败
}

void ULagCompensationComponent::CacheBoxPositions(ABaseCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	/* 将 被击中角色HitCharacter 的 HitBoxes信息 存入 参数OutFramePackage */
	if (HitCharacter == nullptr) return;
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes) {
		if (HitBoxPair.Value != nullptr) {
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package)
{
	/* 将 被击中角色HitCharacter 的 HitBoxes移到历史时刻(参数FFramePackage Package) */
	if (HitCharacter == nullptr) return;
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes) {
		if (HitBoxPair.Value != nullptr) {
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABaseCharacter* HitCharacter, const FFramePackage& Package)
{
	/* 把 HitBox 恢复到当前状态(参数FFramePackage Package), 并关闭 HitBox Collision */
	if (HitCharacter == nullptr) return;
	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes) {
		if (HitBoxPair.Value != nullptr) {
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
			HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABaseCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh()) {
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}


void ULagCompensationComponent::ServerScoreRequest_Implementation(ABaseCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	// 返回 服务器回溯 的 命中结果
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	if (Character && HitCharacter && Character->GetEquippedWeapon() && Confirm.bHitConfirmed) { // 如果命中, 则施加伤害
		const float Damage = Confirm.bHeadShot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();
		UGameplayStatics::ApplyDamage(HitCharacter, Damage, Character->Controller, Character->GetEquippedWeapon(), UDamageType::StaticClass());
	}
}