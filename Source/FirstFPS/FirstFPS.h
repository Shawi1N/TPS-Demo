// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/* ECC_GameTraceChannel1/2/3... 是UE自定义碰撞通道分配的C++枚举名. 它代表自定义的 Trace Channel / Object Channel (DefaultEngine.ini中记录了对应关系) */
#define ECC_SkeletalMesh ECollisionChannel::ECC_GameTraceChannel1
#define ECC_HitBox ECollisionChannel::ECC_GameTraceChannel2
