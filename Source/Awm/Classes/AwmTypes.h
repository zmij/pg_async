// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "AwmTypes.generated.h"

#pragma once

UENUM(BlueprintType)
namespace EAwmMatchState
{
	enum Type
	{
		Warmup,		/** Set where to place squads */
		Playing,	/** It's battle time! */
		Won,		/** Player won, long live the player */
		Lost,		/** Player lost, bad luck */
	};
}

UENUM(BlueprintType)
namespace EGameKey
{
	enum Type
	{
		Tap,
		Hold,
		Swipe,
		SwipeTwoPoints,
		Pinch,
	};
}

UENUM(BlueprintType)
namespace EAwmPhysMaterialType
{
	enum Type
	{
		Unknown,
		Concrete,
		Dirt,
		Water,
		Metal,
		Wood,
		Grass,
		Glass,
		Flesh,
	};
}

#define Awm_SURFACE_Default		SurfaceType_Default
#define Awm_SURFACE_Concrete	SurfaceType1
#define Awm_SURFACE_Dirt		SurfaceType2
#define Awm_SURFACE_Water		SurfaceType3
#define Awm_SURFACE_Metal		SurfaceType4
#define Awm_SURFACE_Wood		SurfaceType5
#define Awm_SURFACE_Grass		SurfaceType6
#define Awm_SURFACE_Glass		SurfaceType7
#define Awm_SURFACE_Flesh		SurfaceType8

/**
 * Decal config for effects
 */
USTRUCT(BlueprintType)
struct FDecalData
{
	GENERATED_USTRUCT_BODY()

	/** Material */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Decal)
	UMaterial* DecalMaterial;

	/** Quad size (width & height) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Decal)
	float DecalSize;

	/** Lifespan */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Decal)
	float LifeSpan;

	/** Defaults */
	FDecalData()
		: DecalSize(256.f)
		, LifeSpan(10.f)
	{
	}
};

/**
 * Projectile config
 */
USTRUCT(BlueprintType)
struct FProjectileWeaponData
{
	GENERATED_USTRUCT_BODY()

	/** Projectile class */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Projectile)
	TSubclassOf<class AAwmProjectile> ProjectileClass;

	/** Life time */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Projectile)
	float ProjectileLife;

	/** Damage at impact point */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = WeaponStat)
	int32 ExplosionDamage;

	/** Radius of damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = WeaponStat)
	float ExplosionRadius;

	/** Type of damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = WeaponStat)
	TSubclassOf<UDamageType> DamageType;

	/** Defaults */
	FProjectileWeaponData()
	{
		ProjectileClass = NULL;
		ProjectileLife = 10.0f;
		ExplosionDamage = 100;
		ExplosionRadius = 0.0f;
		DamageType = UDamageType::StaticClass();
	}
};

USTRUCT(BlueprintType)
struct FInstantHitInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector Origin;

	UPROPERTY()
	float ReticleSpread;

	UPROPERTY()
	int32 RandomSeed;
};

/**
 * Keeps info about one particular damage event
 */
USTRUCT(BlueprintType)
struct FDamageInfo
{
	GENERATED_USTRUCT_BODY()

	/** Total amount of damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DamageInfo)
	float Damage;

	/** Check that intigator is alive when damage is applying */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DamageInfo)
	bool InstigatorShouldBeAlive;

	/** Damage acceptor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DamageInfo)
	AActor* Target;

	/** Damage dealer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DamageInfo)
	AActor* Instigator;

	/** How much battle cycles should pass before damage would be applied */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DamageInfo)
	int32 DelayCycles;

	/** Flag that damage info was processed already */
	bool bProcessed;

	/** Defaults */
	FDamageInfo() {
		Damage = 0.0f;
		InstigatorShouldBeAlive = false;
		DelayCycles = 0;
		bProcessed = false;
	}
};
