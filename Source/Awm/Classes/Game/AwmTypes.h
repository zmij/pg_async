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
namespace EVehicleMovement
{
	enum Type
	{
		Tank,
		Wheel,
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

#define AWM_SURFACE_Default		SurfaceType_Default
#define AWM_SURFACE_Concrete	SurfaceType1
#define AWM_SURFACE_Dirt		SurfaceType2
#define AWM_SURFACE_Water		SurfaceType3
#define AWM_SURFACE_Metal		SurfaceType4
#define AWM_SURFACE_Wood		SurfaceType5
#define AWM_SURFACE_Grass		SurfaceType6
#define AWM_SURFACE_Glass		SurfaceType7
#define AWM_SURFACE_Flesh		SurfaceType8

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
		ExplosionRadius = 300.0f;
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

/** replicated information on a hit we've taken */
USTRUCT()
struct FTakeHitInfo
{
	GENERATED_USTRUCT_BODY()

	/** The amount of damage actually applied */
	UPROPERTY()
	float ActualDamage;

	/** The damage type we were hit with. */
	UPROPERTY()
	UClass* DamageTypeClass;

	/** Who hit us */
	UPROPERTY()
	TWeakObjectPtr<class AAwmVehicle> PawnInstigator;

	/** Who actually caused the damage */
	UPROPERTY()
	TWeakObjectPtr<class AActor> DamageCauser;

	/** Specifies which DamageEvent below describes the damage received. */
	UPROPERTY()
	int32 DamageEventClassID;

	/** Rather this was a kill */
	UPROPERTY()
	uint32 bKilled:1;

private:

	/** A rolling counter used to ensure the struct is dirty and will replicate. */
	UPROPERTY()
	uint8 EnsureReplicationByte;

	/** Describes general damage. */
	UPROPERTY()
	FDamageEvent GeneralDamageEvent;

	/** Describes point damage, if that is what was received. */
	UPROPERTY()
	FPointDamageEvent PointDamageEvent;

	/** Describes radial damage, if that is what was received. */
	UPROPERTY()
	FRadialDamageEvent RadialDamageEvent;

public:
	FTakeHitInfo()
		: ActualDamage(0)
		, DamageTypeClass(NULL)
		, PawnInstigator(NULL)
		, DamageCauser(NULL)
		, DamageEventClassID(0)
		, bKilled(false)
		, EnsureReplicationByte(0)
	{}

	FDamageEvent& GetDamageEvent()
	{
		switch (DamageEventClassID)
		{
		case FPointDamageEvent::ClassID:
			if (PointDamageEvent.DamageTypeClass == NULL)
			{
				PointDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return PointDamageEvent;

		case FRadialDamageEvent::ClassID:
			if (RadialDamageEvent.DamageTypeClass == NULL)
			{
				RadialDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return RadialDamageEvent;

		default:
			if (GeneralDamageEvent.DamageTypeClass == NULL)
			{
				GeneralDamageEvent.DamageTypeClass = DamageTypeClass ? DamageTypeClass : UDamageType::StaticClass();
			}
			return GeneralDamageEvent;
		}
	}

	void SetDamageEvent(const FDamageEvent& DamageEvent)
	{
		DamageEventClassID = DamageEvent.GetTypeID();
		switch (DamageEventClassID)
		{
		case FPointDamageEvent::ClassID:
			PointDamageEvent = *((FPointDamageEvent const*)(&DamageEvent));
			break;
		case FRadialDamageEvent::ClassID:
			RadialDamageEvent = *((FRadialDamageEvent const*)(&DamageEvent));
			break;
		default:
			GeneralDamageEvent = DamageEvent;
		}

		DamageTypeClass = DamageEvent.DamageTypeClass;
	}

	void EnsureReplication()
	{
		EnsureReplicationByte++;
	}
};

USTRUCT(BlueprintType)
struct FFingerTouch
{
	GENERATED_USTRUCT_BODY()

	/** Screen-space touch coordinates (in pixels) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FVector2D TouchLocation;

	/** Screen-space touch origin coordinates (in pixels) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector2D TouchOrigin;

	/** States that finger is in touch state */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bFingerDown;

	/** States that touch consumed by UMG-based UI */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bConsumed;

	/** Defaults */
	FFingerTouch()
	{
		TouchLocation = FVector2D::ZeroVector;
		TouchOrigin = FVector2D::ZeroVector;
		bFingerDown = false;
		bConsumed = false;
	}
};
