// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"


// Sets default values
AShooterCharacter::AShooterCharacter():
BaseTurnRate(45.f),
BaseLookUpRate(45.f)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Create camera boom (pulls in towards the character if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom -> SetupAttachment(RootComponent);
	CameraBoom -> TargetArmLength = 300.f; //camera distance
	CameraBoom -> bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom ->SocketOffset = FVector(0.f, 50.f, 50.f);

	//Create Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera -> SetupAttachment(CameraBoom, USpringArmComponent::SocketName); //attach camera to end of CameraBoom
	FollowCamera -> bUsePawnControlRotation = false;

	//Don't rotate mesh when controller rotates. Let the controller only affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = true;

	//Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; //Character moves in direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); //... at this rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

}

// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void AShooterCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && (Value != 0.0f))
	{
		//find out which way is forward
		const FRotator Rotation{Controller->GetControlRotation()};
		FRotator YawRotation {0, Rotation.Yaw, 0};

		const FVector Direction {FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X)};
		AddMovementInput(Direction,Value);
	}
}

void AShooterCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && (Value != 0.0f))
	{
		//find out which way is right
		const FRotator Rotation{Controller->GetControlRotation()};
		FRotator YawRotation {0, Rotation.Yaw, 0};

		const FVector Direction {FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y)};
		AddMovementInput(Direction,Value);
	}
}

void AShooterCharacter::TurnRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::FireWeapon()
{
	if (FireSound)
	{
		UE_LOG(LogTemp, Warning, TEXT("Fire"));
		UGameplayStatics::PlaySound2D(this,FireSound);
	}
	const USkeletalMeshSocket* BarrelSocket = GetMesh()->GetSocketByName("BarrelSocket");
	if (BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetMesh());
		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
		}
		FHitResult HitResult;
		const FVector Start {SocketTransform.GetLocation()};
		const FQuat Rotation {SocketTransform.GetRotation()};
		const FVector RotationAxis {Rotation.GetAxisX()};
		const FVector End {Start + RotationAxis * 50'000.f};
		FVector BeamEndPoint {End};
		GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if (HitResult.bBlockingHit)
		{
			//DrawDebugLine(GetWorld(),Start, End, FColor::Blue, false, 2.f);
			//DrawDebugPoint(GetWorld(), HitResult.Location, 5.f, FColor::Green, false, 2.f);
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, HitResult.Location);
			}			
		}
		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEndPoint);
			}
		}
	}
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireWeapon);
}

