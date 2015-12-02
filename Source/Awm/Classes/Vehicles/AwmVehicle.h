// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmVehicle.generated.h"

UCLASS(config = Game, Blueprintable, BlueprintType)
class AAwmVehicle : public AWheeledVehicle
{
	GENERATED_UCLASS_BODY()


	//////////////////////////////////////////////////////////////////////////
	// Initialization

	/** spawn inventory, setup initial variables */
	virtual void PostInitializeComponents() override;

	/** update mesh for first person view */
	virtual void PawnClientRestart() override;

	/** [server] perform PlayerState related setup */
	virtual void PossessedBy(class AController* C) override;

	/** [client] perform PlayerState related setup */
	virtual void OnRep_PlayerState() override;

	/** Update the character. (Running, health etc). */
	virtual void Tick(float DeltaSeconds) override;

	/** cleanup inventory */
	virtual void Destroyed() override;


	//////////////////////////////////////////////////////////////////////////
	// Meshes

	/** handle mesh colors on specified material instance */
	void UpdateTeamColors(UMaterialInstanceDynamic* UseMID);

	/** Update the team color of all player meshes. */
	void UpdateTeamColorsAllMIDs();


	//////////////////////////////////////////////////////////////////////////
	// Damage & death

public:
	/** Take damage, handle death */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	/** Pawn suicide */
	virtual void Suicide();

	/** Kill this pawn */
	virtual void KilledBy(class APawn* EventInstigator);

	/** Returns True if the pawn can die in the current state */
	virtual bool CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const;

	/**
	 * Kills pawn.  Server/authority only.
	 * @param KillingDamage - Damage amount of the killing blow
	 * @param DamageEvent - Damage event of the killing blow
	 * @param Killer - Who killed this pawn
	 * @param DamageCauser - the Actor that directly caused the damage (i.e. the Projectile that exploded, the Weapon that fired, etc)
	 * @returns true if allowed
	 */
	virtual bool Die(float KillingDamage, struct FDamageEvent const& DamageEvent, class AController* Killer, class AActor* DamageCauser);

	// Die when we fall out of the world.
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	/** Called on the actor right before replication occurs */
	virtual void PreReplication( IRepChangedPropertyTracker & ChangedPropertyTracker ) override;

protected:
	/** notification when killed, for both the server and client. */
	virtual void OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser);

	/** Total cleanup on death */
	virtual void DeathCleanup();

	/** Give a chance to blueprint to cleanup stuff */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Awm|Vehicle", meta = (DisplayName = "Death Cleanup"))
	void BlueprintDeathCleanup();

	/** Responsible for cleaning up bodies on clients */
	virtual void TornOff();

	/** play effects on hit */
	virtual void PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser);

	/** sets up the replication for taking a hit */
	void ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser, bool bKilled);

	/** play hit or death on client */
	UFUNCTION()
	void OnRep_LastTakeHitInfo();

public:
	/** Identifies if pawn is in its dying state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Health)
	uint32 bIsDying:1;


	//////////////////////////////////////////////////////////////////////////
	// Vehicle Stats

public:
	/** Current health of the Pawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category=Health)
	float Health;

	/** Get max health */
	UFUNCTION(BlueprintCallable, Category = "Awm|Vehicle")
	int32 GetMaxHealth() const;

	/** Check if pawn is still alive */
	UFUNCTION(BlueprintCallable, Category = "Awm|Vehicle")
	bool IsAlive() const;


	//////////////////////////////////////////////////////////////////////////
	// Weapon usage
public:
	/** [local] starts weapon fire */
	void StartWeaponFire();

	/** [local] stops weapon fire */
	void StopWeaponFire();

	/** check if pawn can fire weapon */
	bool CanFire() const;

	/** check if pawn can reload weapon */
	bool CanReload() const;

	/** [server + local] change targeting state */
	void SetTargeting(bool bNewTargeting);


	//////////////////////////////////////////////////////////////////////////
	// Input handlers

public:
	/** setup pawn specific input handlers */
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	/** 
	 * Move forward/back
	 *
	 * @param Val Movment input to apply
	 */
	void MoveForward(float Val);

	/** 
	 * Strafe right/left 
	 *
	 * @param Val Movment input to apply
	 */	
	void MoveRight(float Val);

	/** player pressed start fire action */
	void OnStartFire();

	/** player released start fire action */
	void OnStopFire();

	/** player pressed targeting action */
	void OnStartTargeting();

	/** player released targeting action */
	void OnStopTargeting();

	/** player pressed next weapon action */
	void OnNextWeapon();

	/** player pressed prev weapon action */
	void OnPrevWeapon();

	/** player pressed reload action */
	void OnReload();


	//////////////////////////////////////////////////////////////////////////
	// Reading data

public:
	/** Get targeting state */
	UFUNCTION(BlueprintCallable, Category="Awm|Vehicle|Weapon")
	bool IsTargeting() const;

	/** Get firing state */
	UFUNCTION(BlueprintCallable, Category="Awm|Vehicle|Weapon")
	bool IsFiring() const;

protected:
	/** Replicate where this pawn was last hit and damaged */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_LastTakeHitInfo)
	struct FTakeHitInfo LastTakeHitInfo;

	/** Time at which point the last take hit info for the actor times out and won't be replicated; Used to stop join-in-progress effects all over the screen */
	float LastTakeHitTimeTimeout;

	/** current targeting state */
	UPROPERTY(Transient, Replicated)
	uint8 bIsTargeting : 1;

	/** current firing state */
	uint8 bWantsToFire : 1;

	/** material instances for setting team color in mesh (3rd person view) */
	UPROPERTY(Transient)
	TArray<UMaterialInstanceDynamic*> MeshMIDs;


	//////////////////////////////////////////////////////////////////////////
	// Inventory

public:
	/** Get currently equipped weapon */
	UFUNCTION(BlueprintCallable, Category="Awm|Vehicle|Weapon")
	class AAwmWeapon* GetWeapon() const;

	/** Get weapon attach point */
	UFUNCTION(BlueprintCallable, Category = "Awm|Vehicle|Weapon")
	FName GetWeaponAttachPoint() const;

	/** Get total number of inventory items */
	int32 GetInventoryCount() const;

	/** 
	 * Get weapon from inventory at index. Index validity is not checked.
	 *
	 * @param Index Inventory index
	 */
	class AAwmWeapon* GetInventoryWeapon(int32 index) const;

protected:
	/** updates current weapon */
	void SetCurrentWeapon(class AAwmWeapon* NewWeapon, class AAwmWeapon* LastWeapon = NULL);

	/** current weapon rep handler */
	UFUNCTION()
	void OnRep_CurrentWeapon(class AAwmWeapon* LastWeapon);

	/** [server] spawns default inventory */
	void SpawnDefaultInventory();

	/** [server] remove all weapons from inventory and destroy them */
	void DestroyInventory();

	/**
	* [server] add weapon to inventory
	*
	* @param Weapon	Weapon to add.
	*/
	void AddWeapon(class AAwmWeapon* Weapon);

	/**
	* [server] remove weapon from inventory
	*
	* @param Weapon	Weapon to remove.
	*/
	void RemoveWeapon(class AAwmWeapon* Weapon);

	/**
	* Find in inventory
	*
	* @param WeaponClass	Class of weapon to find.
	*/
	class AAwmWeapon* FindWeapon(TSubclassOf<class AAwmWeapon> WeaponClass);

	/**
	* [server + local] equips weapon from inventory
	*
	* @param Weapon	Weapon to equip
	*/
	void EquipWeapon(class AAwmWeapon* Weapon);

	/** equip weapon */
	UFUNCTION(reliable, server, WithValidation)
	void ServerEquipWeapon(class AAwmWeapon* NewWeapon);

	/** update targeting state */
	UFUNCTION(reliable, server, WithValidation)
	void ServerSetTargeting(bool bNewTargeting);

protected:
	/** socket or bone name for attaching weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
	FName WeaponAttachPoint;

	/** default inventory list */
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
	TArray<TSubclassOf<class AAwmWeapon> > DefaultInventoryClasses;

	/** weapons in inventory */
	UPROPERTY(Transient, Replicated)
	TArray<class AAwmWeapon*> Inventory;

	/** currently equipped weapon */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_CurrentWeapon)
	class AAwmWeapon* CurrentWeapon;


};
