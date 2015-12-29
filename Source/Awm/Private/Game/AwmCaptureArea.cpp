// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

FCaptureAreaIncomeData::FCaptureAreaIncomeData()
{ }

FCaptureAreaIncomeData::FCaptureAreaIncomeData(float InTime, float InValue)
: Time(InTime), Value(InValue)
{ }

AAwmCaptureArea::AAwmCaptureArea(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    bReplicates = true;
    PrimaryActorTick.bCanEverTick = true;
    
    Radius = 1000.f;
    CheckTime = 1.f;
    
    TotalCapturePoints = 100;
    
    // More occupants = less points for each
    CapturePointsCurve.Add(FCaptureAreaIncomeData(1.0f, 1.00f));
    CapturePointsCurve.Add(FCaptureAreaIncomeData(1.0f, 0.95f));
    CapturePointsCurve.Add(FCaptureAreaIncomeData(1.0f, 0.90f));
    CapturePointsCurve.Add(FCaptureAreaIncomeData(1.0f, 0.85f));
    CapturePointsCurve.Add(FCaptureAreaIncomeData(1.0f, 0.80f));
    CapturePointsCurve.Add(FCaptureAreaIncomeData(1.0f, 0.75f));
    CapturePointsCurve.Add(FCaptureAreaIncomeData(1.0f, 0.70f));
    CapturePointsCurve.Add(FCaptureAreaIncomeData(1.0f, 0.65f));
    
    // Remove all points capture if damaged
    DamageCapturePercent = 1.0f;
    
    OccupantTeam = -1;
    OwnerTeam = -1;
    
    LostSuperiority = CaptureAreaLostSuperiorityType::RESET;
    
    // Add bonus point for owner team
    bBonusPointsBroadcast = true;
    BonusPointsIncome.Time = 4.f;
    BonusPointsIncome.Value = 1.f;
}

void AAwmCaptureArea::BeginPlay()
{
	Super::BeginPlay();
    // Only server side
    if (Role < ROLE_Authority) return;
    
    LastCheckTick = GetWorld()->GetTimeSeconds();
    
    GetWorldTimerManager().SetTimer(TimerHandle_DefaultTimer, this, &AAwmCaptureArea::DefaultTimer, CheckTime, true);
}

void AAwmCaptureArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    StopBonusTimer();
}

void AAwmCaptureArea::BonusTimer()
{
    BonusEvent.Broadcast(this);
}

void AAwmCaptureArea::DefaultTimer()
{
	if (!GetWorld()) return;
    
    float CurrentTime = GetWorld()->GetTimeSeconds();
    float DeltaTime = CurrentTime - LastCheckTick;
    LastCheckTick = CurrentTime;
    
    Calculate(CurrentTime, DeltaTime);
}

void AAwmCaptureArea::Calculate(float CurrentTime, float DeltaTime)
{
    // Recalculate if the predicted time is over
    bool bNeedCalculate = EstimatedTime < CurrentTime;
    
    float R = Radius * Radius;
    FVector Position = GetTransform().GetLocation();
    TArray<AActor*> Vehicles;
    
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAwmVehicle::StaticClass(), Vehicles);
    TArray<AController*> ControllersOfOccupants;
    TArray<AController*> Newbies;
    TMap<AController*,int32> DeltaPenetrations;
    
    // find vehicle
    for (int32 i = 0; i < Vehicles.Num(); i++) {
        AAwmVehicle* Vehicle = Cast<AAwmVehicle>(Vehicles[i]);
        if (Vehicle == NULL || !Vehicle->IsAlive()) continue;
        
        float Delta = (Vehicle->GetTransform().GetLocation() - Position).SizeSquared2D();
        if (Delta > R) continue;
        
        AController* Controller = Vehicle->GetController();
        if (Controller == NULL) continue;
        
        AAwmPlayerState* State = (AAwmPlayerState*) Controller->PlayerState;
        if (State->GetTeamNum() < 0) continue;
        
        // here all vehicle in area
        
        // Add 0 points to new occupant
        if (!CurrentCapturePointsMap.Contains(Controller))
        {
            CurrentCapturePointsMap.Add(Controller, 0);
            Newbies.Add(Controller);
            // Recalculate if new vehicle
            bNeedCalculate = true;
        }
        
        // Calculate delta penetrations
        if (DamageCapturePercent > 0.f && Penetrations.Contains(Controller))
        {
            int32 NumPenetrations = State->GetNumPenetrations() - *Penetrations.Find(Controller);
            if ( NumPenetrations > 0 ) {
                DeltaPenetrations.Add( Controller, NumPenetrations );
                // Recalculate if the occupant got penetration
                bNeedCalculate = true;
            }
        }
        Penetrations.Add(Controller, State->GetNumPenetrations());
        
        ControllersOfOccupants.Add(Controller);
    }
    
    TArray<AController*> Lost = GetLostControllers(ControllersOfOccupants);
    if (Lost.Num() > 0)
    {
        ControllersOfOccupants.Append(Lost);
        // Recalculate if the vehicle has disappeared
        bNeedCalculate = true;
    }
    
    // Create team list
    TMap<int32,TArray<AController*>> Teams;
    for(auto Controller : ControllersOfOccupants)
    {
        AAwmPlayerState* State = (AAwmPlayerState*) Controller->PlayerState;
        if (Teams.Contains(State->GetTeamNum()))
        {
            Teams.Find(State->GetTeamNum())->Add(Controller);
        }
        else
        {
            TArray<AController*> TeamList;
            TeamList.Add(Controller);
            Teams.Add(State->GetTeamNum(), TeamList);
        }
    }
    
    // Find a team with the max number of occupants
    int32 MaxTeam = -1;
    int32 MaxValue = 0;
    
    for(auto Pair : Teams) {
        float Value = 0;
        for(auto Controller : Pair.Value)
        {
            // skip lost controllers
            if (!Lost.Contains(Controller))
            {
                Value++;
            }
        }
        if (MaxValue < Value)
        {
            MaxTeam = Pair.Key;
            MaxValue = Value;
        }
        else if (MaxValue == Value)
        {
            MaxTeam = -1;
        }
    }
    
    if ( !bNeedCalculate ) return;
    
    //Long time without calculation
    if (LastCalculateTime > 0)
    {
        DeltaTime = CurrentTime - LastCalculateTime;
    }
    
    // Add points to occupants
    CalculateCapturePoints(Teams, Newbies, DeltaTime);
    // Remove excess points
    CalculateExcessPoints(Teams);
    // Remove points by LostSuperiority rule
    CalculateLostSuperiority(MaxTeam);
    // Remove "damaged" points
    CalculateDamageCapture(DeltaPenetrations);
    // Clear lost vehicle points & all information
    ClearLostControllers(Lost);
    
    TArray<int32> Keys;
    Teams.GetKeys(Keys);
    
    for(int32 Key : Keys)
    {
        for(auto Controller : Lost)
        {
            Teams.Find(Key)->RemoveSingle(Controller);
        }
    }
    
    // Calculate owner
    CalculateOwner(Teams);
    // Calculate estimate
    CalculateEstimate(Teams, CurrentTime);
}

TArray<AController*> AAwmCaptureArea::GetLostControllers(TArray<AController*>& ControllersOfOccupants)
{
    TArray<AController*> Lost;
    for(auto Pair : CurrentCapturePointsMap)
    {
        if (!ControllersOfOccupants.Contains(Pair.Key))
        {
            Lost.Add(Pair.Key);
        }
    }
    
    return Lost;
}

void AAwmCaptureArea::CalculateCapturePoints(TMap<int32,TArray<AController*>>& Teams, TArray<AController*>& Newbies, float DeltaTime)
{
    TMap<int32,float> TeamPoints;
    
    float LostSuperiorityFactor = 1.0f;
    
    if (LostSuperiority == CaptureAreaLostSuperiorityType::STOP && Teams.Num() > 1)
    {
        LostSuperiorityFactor = 0.0f;
    }
    
    for(auto Pair : Teams)
    {
        FCaptureAreaIncomeData IncomeData = GetCapturePointsCurve(Pair.Value.Num());
        float AddPointForEach = IncomeData.Value * (DeltaTime / IncomeData.Time) * LostSuperiorityFactor;
        for(auto Controller : Pair.Value)
        {
            if (Newbies.Contains(Controller)) continue;
            CurrentCapturePointsMap.Add(Controller, *CurrentCapturePointsMap.Find(Controller) + AddPointForEach);
        }
    }
}

void AAwmCaptureArea::CalculateExcessPoints(TMap<int32,TArray<AController*>>& Teams)
{
    TMap<int32,float> TeamsPoints = GetTeamsPoints(Teams);
    
    float MaxPointsValue = 0;
    float MidPointsValue = 0;
    
    for(auto Pair : TeamsPoints)
    {
        if (MaxPointsValue <= Pair.Value)
        {
            MidPointsValue = MaxPointsValue;
            MaxPointsValue = Pair.Value;
        }
        else if (MidPointsValue <= Pair.Value)
        {
            MidPointsValue = Pair.Value;
        }
    }
    
    if (MidPointsValue > 0.f)
    {
        float Remove = MidPointsValue;
        for(auto Pair : Teams)
        {
            float Points = *TeamsPoints.Find(Pair.Key);
            if (Points <= 0.f) continue;
            
            float RemovePercent = ( Points - Remove ) / Points;
            if (RemovePercent < 0.f) RemovePercent = 0.f;
            
            for(auto Controller : Pair.Value)
            {
                CurrentCapturePointsMap.Add(Controller, *CurrentCapturePointsMap.Find(Controller) * RemovePercent);
            }
        }
    }
}

void AAwmCaptureArea::CalculateLostSuperiority(int32 MaxTeam)
{
    if (LostSuperiority == CaptureAreaLostSuperiorityType::RESET)
    {
        if ( MaxTeam < 0 ) return;
        for(auto Pair : CurrentCapturePointsMap)
        {
            AAwmPlayerState* State = (AAwmPlayerState*) Pair.Key->PlayerState;
            if (State->GetTeamNum() != MaxTeam)
            {
                CurrentCapturePointsMap.Add(Pair.Key, 0.f);
            }
        }
    }
}

void AAwmCaptureArea::CalculateDamageCapture(TMap<AController*,int32>& DeltaPenetrations)
{
    for(auto Pair : CurrentCapturePointsMap)
    {
        float CurrentCapture = *CurrentCapturePointsMap.Find(Pair.Key);
        if (CurrentCapture > 0 && DeltaPenetrations.Contains(Pair.Key))
        {
            int32 NumPenetrations = *DeltaPenetrations.Find(Pair.Key);
            for(int32 i = 0; i < NumPenetrations; i++)
            {
                CurrentCapture -= CurrentCapture * DamageCapturePercent;
            }
            
            if (CurrentCapture < 0) CurrentCapture = 0;
            CurrentCapturePointsMap.Add(Pair.Key, CurrentCapture);
        }
    }
}

void AAwmCaptureArea::ClearLostControllers(TArray<AController*>& Lost)
{
    if (Lost.Num() == 0) return;
    
    // Remove all information about lost controllers
    for(auto Controller : Lost) {
        CurrentCapturePointsMap.Remove(Controller);
        Penetrations.Remove(Controller);
    }
}

void AAwmCaptureArea::CalculateOwner(TMap<int32,TArray<AController*>>& Teams)
{
    TMap<int32,float> TeamsPoints = GetTeamsPoints(Teams);
    
    bool ChangeOccupant = false;
    bool ChangeOwner = false;
    
    int32 MaxPointsTeam = -1;
    float MaxPointsValue = -1;
    
    for(auto Pair : TeamsPoints)
    {
        if (Pair.Value >= MaxPointsValue)
        {
            MaxPointsValue = Pair.Value;
            MaxPointsTeam = Pair.Key;
        }
    }
    
    if (OccupantTeam != MaxPointsTeam)
        ChangeOccupant = true;
    
    OccupantTeam = MaxPointsTeam;
    
    if (OccupantTeam > -1)
    {
        LastCalculateCapturePoints = *TeamsPoints.Find(OccupantTeam);
        if (LastCalculateCapturePoints >= TotalCapturePoints)
        {
            if ( OwnerTeam != OccupantTeam )
                ChangeOwner = true;
            
            OwnerTeam = OccupantTeam;
        }
        if (OccupantTeam == OwnerTeam)
        {
            ClearCapturePoints();
        }
    }
    else
    {
        ClearCapturePoints();
    }
    
    if (ChangeOccupant)
    {
        // broadcast
    }
    
    if (ChangeOwner)
    {
        StopBonusTimer();
        if (bBonusPointsBroadcast && HasOwner() && BonusPointsIncome.Time > 0.f && BonusPointsIncome.Value > 0.f)
        {
            GetWorldTimerManager().SetTimer(TimerHandle_BonusTimer, this, &AAwmCaptureArea::BonusTimer, BonusPointsIncome.Time, true);
        }
        // broadcast
    }
}

void AAwmCaptureArea::CalculateEstimate(TMap<int32,TArray<AController*>>& Teams, float CurrentTime)
{
    float LostSuperiorityFactor = 1.0f;
    
    if (LostSuperiority == CaptureAreaLostSuperiorityType::STOP && Teams.Num() > 1)
    {
        LostSuperiorityFactor = 0.0f;
    }
    
    int32 MaxPPSTeam = -1;
    float MaxPPSValue = 0;
    float MidPPSValue = 0;
    
    for(auto Pair : Teams)
    {
        FCaptureAreaIncomeData IncomeData = GetCapturePointsCurve(Pair.Value.Num());
        float PPS = Pair.Value.Num() * (IncomeData.Value / IncomeData.Time) * LostSuperiorityFactor;
        
        if (PPS >= MaxPPSValue)
        {
            MidPPSValue = MaxPPSValue;
            MaxPPSValue = PPS;
            MaxPPSTeam = Pair.Key;
        } else if (PPS >= MidPPSValue) {
            MidPPSValue = PPS;
        }
    }
    
    float PPS = MaxPPSValue - MidPPSValue;
    
    if (PPS == 0 || OccupantTeam == OwnerTeam)
    {
        // No changes
        EstimatedTime = CurrentTime + 60 * 10;
        EstimatedCapturePoints = LastCalculateCapturePoints;
    }
    else if (MaxPPSTeam == OccupantTeam)
    {
        // Current occupant won
        EstimatedTime = CurrentTime + (TotalCapturePoints - LastCalculateCapturePoints) / PPS;
        EstimatedCapturePoints = TotalCapturePoints;
    }
    else
    {
        // Current occupant lose
        EstimatedTime = CurrentTime + LastCalculateCapturePoints / PPS;
        EstimatedCapturePoints = 0;
    }
    
    LastCalculateTime = CurrentTime;
}

void AAwmCaptureArea::ClearCapturePoints()
{
    LastCalculateCapturePoints = 0;
    for(auto Pair : CurrentCapturePointsMap)
    {
        CurrentCapturePointsMap.Add(Pair.Key, 0.f);
    }
}

void AAwmCaptureArea::StopBonusTimer()
{
    GetWorldTimerManager().ClearTimer(TimerHandle_BonusTimer);
}

float AAwmCaptureArea::GetCurrentCapturePoints() const
{
    float TimeFactor = FMath::Clamp((EstimatedTime - GetWorld()->GetTimeSeconds()) / (EstimatedTime - LastCalculateTime),0.f, 1.f);
    return TimeFactor * LastCalculateCapturePoints + (1.f - TimeFactor) * EstimatedCapturePoints;
}

float AAwmCaptureArea::GetCaptureProgress() const
{
    return FMath::Clamp(GetCurrentCapturePoints() / (float)TotalCapturePoints, 0.f, 1.f);
}

bool AAwmCaptureArea::HasOccupant() const
{
    return OccupantTeam != -1;
}

int32 AAwmCaptureArea::GetOccupantTeam() const
{
    return OccupantTeam;
}

bool AAwmCaptureArea::HasOwner() const
{
    return OwnerTeam != -1;
}

int32 AAwmCaptureArea::GetOwnerTeam() const
{
    return OwnerTeam;
}

float AAwmCaptureArea::GetRadius() const
{
    return Radius;
}

TMap<int32,float> AAwmCaptureArea::GetTeamsPoints(TMap<int32,TArray<AController*>>& Teams)
{
    TMap<int32,float> TeamsPoints;
    
    // Calculate total points for each team
    for(auto Pair : Teams)
    {
        float Points = 0.f;
        for(auto Controller : Pair.Value)
        {
            Points += *CurrentCapturePointsMap.Find(Controller);
        }
        TeamsPoints.Add(Pair.Key, Points);
    }
    
    return TeamsPoints;
}

FCaptureAreaIncomeData& AAwmCaptureArea::GetCapturePointsCurve(int32 Num)
{
    Num--;
    if (Num >= CapturePointsCurve.Num())
    {
        Num = CapturePointsCurve.Num() - 1;
    }
    if (Num < 0)
    {
        Num = 0;
    }
    return CapturePointsCurve[Num];
}

void AAwmCaptureArea::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AAwmCaptureArea, OccupantTeam);
    DOREPLIFETIME(AAwmCaptureArea, OwnerTeam);
    DOREPLIFETIME(AAwmCaptureArea, LastCalculateCapturePoints);
    DOREPLIFETIME(AAwmCaptureArea, LastCalculateTime);
    DOREPLIFETIME(AAwmCaptureArea, EstimatedTime);
    DOREPLIFETIME(AAwmCaptureArea, EstimatedCapturePoints);
    
}