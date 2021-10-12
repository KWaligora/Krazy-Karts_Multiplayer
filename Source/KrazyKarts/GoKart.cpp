#include "GoKart.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
}

void AGoKart::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME( AGoKart, ReplicatedLocation);
	DOREPLIFETIME( AGoKart, ReplicatedRotation);
}

// Calculate car Velocity
void AGoKart::CalculateCarVelocity(float DeltaTime)
{	
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Acceleration = Force / Mass;
	Velocity += Acceleration * DeltaTime;
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	FVector Translation = Velocity * 100 * DeltaTime; // *100 because meters to cm
	FHitResult Hit;
	AddActorWorldOffset(Translation, true, &Hit);
	
	// if car hit something
	if(Hit.IsValidBlockingHit())	
		Velocity = FVector::ZeroVector;	
}

void AGoKart::ApplyRotation(float DeltaTime, FQuat& RotationDelta)
{
	// Rotation angle with radians
	float DeltaLocation = FVector::DotProduct(GetActorForwardVector(),Velocity) * DeltaTime;
	float RotationAngle = DeltaLocation / MinTurningRadius * SteeringThrow;
	RotationDelta = FQuat(GetActorUpVector(), RotationAngle);
	Velocity = RotationDelta.RotateVector(Velocity);
}
// Set Location on Server and get on clients
void AGoKart::SynchTransform()
{
	if(HasAuthority())
	{
		ReplicatedLocation = GetActorLocation();
		ReplicatedRotation = GetActorRotation();
	}
	else
	{
		SetActorLocation(ReplicatedLocation);
		SetActorRotation(ReplicatedRotation);
	}
}

void AGoKart::MoveForward(float Value)
{
	Throttle = Value;
	Server_MoveForward(Value);
}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
	Server_MoveRight(Value);
}

// Set car throttle with player's input
void AGoKart::Server_MoveForward_Implementation(float Value)
{
	Throttle = Value;
}

bool AGoKart::Server_MoveForward_Validate(float Value)
{
	return FMath::Abs(Value) <= 1;
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);	
	
	// Move car
	CalculateCarVelocity(DeltaTime);
	UpdateLocationFromVelocity(DeltaTime);

	FQuat RotationDelta;
	ApplyRotation(DeltaTime, RotationDelta);
	AddActorWorldRotation(RotationDelta, true);
	DrawDebugString(GetWorld(), FVector(0,0,100), UEnum::GetValueAsString(GetLocalRole()), this, FColor::White, DeltaTime);

	SynchTransform();
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

void AGoKart::Server_MoveRight_Implementation(float Value)
{
	SteeringThrow = Value;
}

bool AGoKart::Server_MoveRight_Validate(float Value)
{
	return FMath::Abs(Value) <= 1;
}

FVector AGoKart::GetAirResistance()
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKart::GetRollingResistance()
{
	float AccelerationDueToGravity  = -GetWorld()->GetGravityZ() / 100; // UE declare gravity as -980
	float NormalForce = Mass * AccelerationDueToGravity;
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

