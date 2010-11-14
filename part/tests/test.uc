//=============================================================================
// Shield Gun
//=============================================================================
class ShieldGun extends Weapon
    config(user);

#EXEC OBJ LOAD FILE=InterfaceContent.utx

var Sound       ShieldHitSound;
var String		ShieldHitForce;

replication
{
    reliable if (Role == ROLE_Authority)
        ClientTakeHit;
}

simulated function DoAutoSwitch()
{
}

simulated event RenderOverlays( Canvas Canvas )
{
    local int m;

    if ((Hand < -1.0) || (Hand > 1.0))
    {
		for (m = 0; m < NUM_FIRE_MODES; m++)
		{
			if (FireMode[m] != None)
			{
				FireMode[m].DrawMuzzleFlash(Canvas);
			}
		}
	}
    Super.RenderOverlays(Canvas);
}

// AI Interface
function GiveTo(Pawn Other, optional Pickup Pickup)
{
	Super.GiveTo(Other, Pickup);
	
	if ( Bot(Other.Controller) != None )
		Bot(Other.Controller).bHasImpactHammer = true;
}

function bool CanAttack(Actor Other)
{
	return true;
}

simulated function Timer()
{
	local Bot B;

    if (ClientState == WS_BringUp)
    {
		// check if owner is bot waiting to do impact jump
		B = Bot(Instigator.Controller);
		if ( (B != None) && B.bPreparingMove && (B.ImpactTarget != None) )
		{
			B.ImpactJump();
			B = None;
		}
    }
	Super.Timer();
	if ( (B != None) && (B.Enemy != None) )
		BotFire(false);
}

function FireHack(byte Mode)
{
	if ( Mode == 0 )
	{
		FireMode[0].PlayFiring();
		FireMode[0].FlashMuzzleFlash();
		FireMode[0].StartMuzzleSmoke();
		IncrementFlashCount(0);
	}
}

/* BestMode()
choose between regular or alt-fire
*/
function byte BestMode()
{
	local float EnemyDist;
	local bot B;

	B = Bot(Instigator.Controller);
	if ( (B == None) || (B.Enemy == None) )
		return 1;

	EnemyDist = VSize(B.Enemy.Location - Instigator.Location);
	if ( EnemyDist > 2 * Instigator.GroundSpeed )
		return 1;
	if ( (B.MoveTarget != B.Enemy) && ((EnemyDist > 0.5 * Instigator.GroundSpeed) 
		|| (((B.Enemy.Location - Instigator.Location) Dot vector(Instigator.Rotation)) <= 0)) )
		return 1;
	return 0;
}

// super desireable for bot waiting to impact jump
function float GetAIRating()
{
	local Bot B;
	local float EnemyDist;

	B = Bot(Instigator.Controller);
	if ( B == None )
		return AIRating;

	if ( B.bPreparingMove && (B.ImpactTarget != None) )
		return 9;

	if ( B.PlayerReplicationInfo.HasFlag != None )
	{
		if ( Instigator.Health < 50 )
			return AIRating + 0.35;
		return AIRating + 0.25;
	}
		
	if ( B.Enemy == None )
		return AIRating;

	EnemyDist = VSize(B.Enemy.Location - Instigator.Location);
	if ( B.Stopped() && (EnemyDist > 100) )
		return 0.1;
		
	if ( (EnemyDist < 750) && (B.Skill <= 2) && !B.Enemy.IsA('Bot') && (ShieldGun(B.Enemy.Weapon) != None) )
		return FClamp(300/(EnemyDist + 1), 0.6, 0.75);

	if ( EnemyDist > 400 )
		return 0.1;
	if ( (Instigator.Weapon != self) && (EnemyDist < 120) )
		return 0.25;

	return ( FMin(0.6, 90/(EnemyDist + 1)) );
}

// End AI interface

function AdjustPlayerDamage( out int Damage, Pawn InstigatedBy, Vector HitLocation, 
						         out Vector Momentum, class<DamageType> DamageType)
{
    local int Drain;
	local vector Reflect;
    local vector HitNormal;
    local float DamageMax;

	DamageMax = 100.0;
	if ( DamageType == class'Fell' )
		DamageMax = 20.0;
    else if( !DamageType.default.bArmorStops || (DamageType == class'DamTypeShieldImpact' && InstigatedBy == Instigator) )
        return;

    if ( CheckReflect(HitLocation, HitNormal, 0) )
    {
        Drain = Min( Ammo[1].AmmoAmount*2, Damage );
		Drain = Min(Drain,DamageMax);
	    Reflect = MirrorVectorByNormal( Normal(Location - HitLocation), Vector(Instigator.Rotation) );
        Damage -= Drain;
        Momentum *= 1.25;
        Ammo[1].UseAmmo(Drain/2);
        DoReflectEffect(Drain/2);
    }
}

function DoReflectEffect(int Drain)
{
    PlaySound(ShieldHitSound, SLOT_None);
    ShieldAltFire(FireMode[1]).TakeHit(Drain);
    ClientTakeHit(Drain);
}

simulated function ClientTakeHit(int Drain)
{
	ClientPlayForceFeedback(ShieldHitForce);
    ShieldAltFire(FireMode[1]).TakeHit(Drain);
}

function bool CheckReflect( Vector HitLocation, out Vector RefNormal, int AmmoDrain )
{
    local Vector HitDir;
    local Vector FaceDir;

    if (!FireMode[1].bIsFiring || Ammo[0].AmmoAmount == 0) return false;

    FaceDir = Vector(Instigator.Controller.Rotation);
    HitDir = Normal(Instigator.Location - HitLocation + Vect(0,0,8));
    //Log(self@"HitDir"@(FaceDir dot HitDir));

    RefNormal = FaceDir;

    if ( FaceDir dot HitDir < -0.37 ) // 68 degree protection arc
    {
        if (AmmoDrain > 0)
            Ammo[0].UseAmmo(AmmoDrain);
        return true;
    }
    return false;
}

function AnimEnd(int channel)
{
    if (FireMode[0].bIsFiring)
    {
        LoopAnim('Charged');
    }
    else if (!FireMode[1].bIsFiring)    
    {
        Super.AnimEnd(channel);
    }
}

function float SuggestAttackStyle()
{
    return 0.8;
}

function float SuggestDefenseStyle()
{
    return -0.8;
}

simulated function float ChargeBar()
{
	return FMin(1,FireMode[0].HoldTime/ShieldFire(FireMode[0]).FullyChargedTime);
} 

defaultproperties
{
    ItemName="Shield Gun"
    IconMaterial=Material'InterfaceContent.Hud.SkinA'
    IconCoords=(X1=200,Y1=281,X2=321,Y2=371)

	bShowChargingBar=true
    bCanThrow=false
    FireModeClass(0)=ShieldFire
    FireModeClass(1)=ShieldAltFire
    InventoryGroup=1
    Mesh=mesh'Weapons.ShieldGun_1st'
    BobDamping=2.2
    PickupClass=class'ShieldGunPickup'
    EffectOffset=(X=15.0,Y=6.7,Z=1.2)
    bMeleeWeapon=true
    ShieldHitSound=Sound'WeaponSounds.ShieldGun.ShieldReflection'
    DrawScale=0.4
    PutDownAnim=PutDown
    DisplayFOV=60
    PlayerViewOffset=(X=2,Y=-0.7,Z=-2.7)
    PlayerViewPivot=(Pitch=500,Roll=0,Yaw=500)

    UV2Texture=Material'XGameShaders.WeaponEnvShader'    

    AttachmentClass=class'ShieldAttachment'
    SelectSound=Sound'WeaponSounds.ShieldGun_change'
	SelectForce="ShieldGun_change"
	ShieldHitForce="ShieldReflection"

	AIRating=0.35
	CurrentRating=0.35

	DefaultPriority=2
}
