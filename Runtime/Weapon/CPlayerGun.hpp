#ifndef __URDE_CPLAYERGUN_HPP__
#define __URDE_CPLAYERGUN_HPP__

#include "RetroTypes.hpp"
#include "Character/CActorLights.hpp"
#include "CFidget.hpp"
#include "zeus/CTransform.hpp"
#include "zeus/CAABox.hpp"
#include "World/CPlayerCameraBob.hpp"
#include "CGunMotion.hpp"
#include "CGrappleArm.hpp"
#include "CAuxWeapon.hpp"
#include "CPowerBeam.hpp"
#include "CIceBeam.hpp"
#include "CWaveBeam.hpp"
#include "CPlasmaBeam.hpp"
#include "CPhazonBeam.hpp"
#include "Particle/CElementGen.hpp"
#include "Character/CModelData.hpp"
#include "World/CWorldShadow.hpp"
#include "World/ScriptObjectSupport.hpp"
#include "Graphics/CRainSplashGenerator.hpp"

namespace urde
{
class CFinalInput;

class CPlayerGun
{
public:
    enum class EMissleMode
    {
        Inactive,
        Active
    };
    enum class EBWeapon
    {

    };
    enum class EGunOverrideMode
    {
        Normal,
        One,
        Two,
        Three
    };
private:
    class CGunMorph
    {
    public:
        enum class EGunState
        {
            Zero,
            One
        };
    private:
        float x0_ = 0.f;
        float x4_gunTransformTime;
        float x8_ = 0.f;
        float xc_ = 0.1f;
        float x10_holoHoldTime;
        float x14_ = 2.f;
        float x18_transitionFactor = 1.f;
        u32 x1c_ = 2;
        EGunState x20_gunState = EGunState::One;

        union
        {
            struct
            {
                bool x24_24_ : 1;
                bool x24_25_ : 1;
            };
            u32 _dummy = 0;
        };

    public:
        CGunMorph(float gunTransformTime, float holoHoldTime)
        : x4_gunTransformTime(gunTransformTime), x10_holoHoldTime(std::fabs(holoHoldTime)) {}
        float GetTransitionFactor() const { return x18_transitionFactor; }
        EGunState GetGunState() const { return x20_gunState; }
    };

    class CMotionState
    {
    public:
        enum class EMotionState
        {
            Zero,
            One,
            Two
        };
    private:
        static float gGunExtendDistance;
        bool x0_24_ = true;
        float x4_ = 0.f;
        float x8_ = 0.f;
        float xc_ = 0.f;
        float x10_ = 0.f;
        float x14_ = 0.f;
        float x18_ = 0.f;
        float x1c_ = 0.f;
        EMotionState x20_state = EMotionState::Zero;
        u32 x24_ = 0;
    public:
        static void SetExtendDistance(float d) { gGunExtendDistance = d; }
        void SetState(EMotionState state) { x20_state = state; }
        void Update(bool b1, float dt, zeus::CTransform& xf, CStateManager& mgr);

    };

    CActorLights x0_lights;
    CSfxHandle x2e0_chargeSfx;
    CSfxHandle x2e4_invalidSfx;
    u32 x2e8_ = 0;
    u32 x2ec_firing = 0;
    u32 x2f0_ = 0;
    u32 x2f4_ = 0;
    u32 x2f8_ = 1;
    u32 x2fc_ = 0;
    u32 x300_ = 0;
    u32 x304_ = 0;
    u32 x308_bombCount = 3;
    u32 x30c_ = 0;
    u32 x310_selectedBeam = 0;
    u32 x314_pendingSelectedBeam = 0;
    u32 x318_ = 0;
    EMissleMode x31c_missileMode = EMissleMode::Inactive;
    u32 x320_ = 0;
    u32 x324_ = 4;
    u32 x328_ = 0x2000;
    u32 x32c_ = 0;
    u32 x330_chargeWeaponIdx = 0;
    u32 x334_ = 0;
    u32 x338_ = 0;
    EGunOverrideMode x33c_gunOverrideMode = EGunOverrideMode::Normal;
    float x340_chargeBeamFactor = 0.f;
    float x344_ = 0.f;
    float x348_ = 0.f;
    float x34c_shakeX = 0.f;
    float x350_shakeZ = 0.f;
    float x354_bombFuseTime;
    float x358_bombDropDelayTime;
    float x35c_bombTime = 0.f;
    float x360_ = 0.f;
    float x364_ = 0.f;
    float x368_ = 0.f;
    float x36c_ = 1.f;
    float x370_gunMotionSpeedMult = 1.f;
    float x374_ = 0.f;
    float x378_ = 0.f;
    float x37c_ = 0.f;
    float x380_ = 0.f;
    float x384_ = 0.f;
    float x388_ = 0.f;
    float x38c_ = 0.f;
    float x390_cooldown = 0.f;
    float x394_damageTimer = 0.f;
    float x398_damageAmt = 0.f;
    float x39c_ = 0.f;
    float x3a0_ = 0.f;
    CFidget x3a4_fidget;
    zeus::CVector3f x3dc_damageLocation;
    zeus::CTransform x3e8_xf;
    zeus::CTransform x418_beamLocalXf;
    zeus::CTransform x448_elbowWorldXf;
    zeus::CTransform x478_assistAimXf;
    zeus::CTransform x4a8_gunWorldXf;
    zeus::CTransform x4d8_gunLocalXf;
    zeus::CTransform x508_elbowLocalXf;
    TUniqueId x538_playerId;
    TUniqueId x53a_ = kInvalidUniqueId;
    TUniqueId x53c_lightId = kInvalidUniqueId;
    std::vector<CToken> x540_handAnimTokens;
    CPlayerCameraBob x550_camBob;
    u32 x658_ = 1;
    float x65c_ = 0.f;
    float x660_ = 0.f;
    float x664_ = 0.f;
    float x668_aimVerticalSpeed;
    float x66c_aimHorizontalSpeed;
    TUniqueId x670_ = kInvalidUniqueId;
    u32 x674_ = 0;
    CGunMorph x678_morph;
    CMotionState x6a0_motionState;
    zeus::CAABox x6c8_;
    CModelData x6e0_rightHandModel;
    CGunWeapon* x72c_currentBeam = nullptr;
    u32 x730_ = 0;
    CGunWeapon* x734_ = nullptr;
    CGunWeapon* x738_nextBeam = nullptr;
    std::unique_ptr<CGunMotion> x73c_gunMotion;
    std::unique_ptr<CGrappleArm> x740_grappleArm;
    std::unique_ptr<CAuxWeapon> x744_auxWeapon;
    std::unique_ptr<CRainSplashGenerator> x748_rainSplashGenerator;
    std::unique_ptr<CPowerBeam> x74c_powerBeam;
    std::unique_ptr<CIceBeam> x750_iceBeam;
    std::unique_ptr<CWaveBeam> x754_waveBeam;
    std::unique_ptr<CPlasmaBeam> x758_plasmaBeam;
    std::unique_ptr<CPhazonBeam> x75c_phazonBeam;
    CGunWeapon* x760_selectableBeams[4] = {}; // Used to be reserved_vector
    std::unique_ptr<CElementGen> x774_holoTransitionGen;
    std::unique_ptr<CElementGen> x77c_;
    rstl::reserved_vector<rstl::reserved_vector<TLockedToken<CGenDescription>, 2>, 2> x784_bombEffects;
    rstl::reserved_vector<TLockedToken<CGenDescription>, 5> x7c0_auxMuzzleEffects;
    rstl::reserved_vector<std::unique_ptr<CElementGen>, 5> x800_auxMuzzleGenerators;
    std::unique_ptr<CWorldShadow> x82c_shadow;
    s16 x830_chargeRumbleHandle = -1;

    union
    {
        struct
        {
            bool x832_24_ : 1;
            bool x832_25_ : 1;
            bool x832_26_ : 1;
            bool x832_27_ : 1;
            bool x832_28_ : 1;
            bool x832_29_ : 1;
            bool x832_30_ : 1;
            bool x832_31_ : 1;

            bool x833_24_isFidgeting : 1;
            bool x833_25_ : 1;
            bool x833_26_ : 1;
            bool x833_27_ : 1;
            bool x833_28_phazonBeamActive : 1;
            bool x833_29_pointBlankWorldSurface : 1;
            bool x833_30_ : 1;
            bool x833_31_ : 1;

            bool x834_24_charging : 1;
            bool x834_25_ : 1;
            bool x834_26_ : 1;
            bool x834_27_underwater : 1;
            bool x834_28_ : 1;
            bool x834_29_frozen : 1;
            bool x834_30_inBigStrike : 1;
            bool x834_31_ : 1;

            bool x835_24_canFirePhazon : 1;
            bool x835_25_inPhazonBeam : 1;
            bool x835_26_phazonBeamMorphing : 1;
            bool x835_27_intoPhazonBeam : 1;
            bool x835_28_bombReady : 1;
            bool x835_29_powerBombReady : 1;
            bool x835_30_inPhazonPool : 1;
            bool x835_31_actorAttached : 1;
        };
        u32 _dummy = 0;
    };

    void InitBeamData();
    void InitBombData();
    void InitMuzzleData();
    void InitCTData();
    void LoadHandAnimTokens();
    void CreateGunLight(CStateManager& mgr);
    void DeleteGunLight(CStateManager& mgr);
    void UpdateGunLight(const zeus::CTransform& pos, CStateManager& mgr);
    void SetGunLightActive(bool active, CStateManager& mgr);
    void SetPhazonBeamMorph(bool intoPhazonBeam);
    void Reset(CStateManager& mgr, bool b1);
    void ResetBeamParams(CStateManager& mgr, const CPlayerState& playerState, bool playSelectionSfx);
    CSfxHandle PlaySfx(u16 sfx, bool underwater, bool looped, float pan);
    void PlayAnim(NWeaponTypes::EGunAnimType type, bool loop);
    void CancelCharge(CStateManager& mgr, bool withEffect);
    bool ExitMissile();
    void StopChargeSound(CStateManager& mgr);
    void UnLoadFidget();
    void ReturnArmAndGunToDefault(CStateManager& mgr, bool b1);
    void ReturnToRestPose();
    void ChangeWeapon(const CPlayerState& playerState, CStateManager& mgr);
    void GetLctrWithShake(zeus::CTransform& xfOut, const CModelData& mData, const std::string& lctrName,
                          bool b1, bool b2);
    void UpdateLeftArmTransform(const CModelData& mData, const CStateManager& mgr);
    void ProcessGunMorph(float dt, CStateManager& mgr);
    void ProcessPhazonGunMorph(float dt, CStateManager& mgr);
    void UpdateChargeState(float dt, CStateManager& mgr);
    void UpdateAuxWeapons(float dt, const zeus::CTransform& targetXf, CStateManager& mgr);
    void DoUserAnimEvents(float dt, CStateManager& mgr);
    TUniqueId GetTargetId(CStateManager& mgr) const;
    void CancelLockOn();
    void UpdateWeaponFire(float dt, const CPlayerState& playerState, CStateManager& mgr);
    void UpdateGunIdle(bool b1, float camBobT, float dt, CStateManager& mgr);

public:
    explicit CPlayerGun(TUniqueId playerId);
    void TakeDamage(bool bigStrike, bool notFromMetroid, CStateManager& mgr);
    void AcceptScriptMsg(EScriptObjectMessage, TUniqueId, CStateManager&);
    void AsyncLoadSuit(CStateManager& mgr);
    void TouchModel(const CStateManager& stateMgr);
    EMissleMode GetMissleMode() const { return x31c_missileMode; }
    bool IsFidgeting() const { return x833_24_isFidgeting; }
    bool IsCharging() const { return x834_24_charging; }
    float GetChargeBeamFactor() const { return x340_chargeBeamFactor; }
    bool IsBombReady() const { return x835_28_bombReady; }
    u32 GetBombCount() const { return x308_bombCount; }
    bool IsPowerBombReady() const { return x835_29_powerBombReady; }
    u32 GetSelectedBeam() const { return x310_selectedBeam; }
    u32 GetPendingSelectedBeam() const { return x314_pendingSelectedBeam; }
    const CGunMorph& GetGunMorph() const { return x678_morph; }
    float GetHoloTransitionFactor() const { return x678_morph.GetTransitionFactor(); }
    void SetTransform(const zeus::CTransform& xf) { x3e8_xf = xf; }
    void SetAssistAimTransform(const zeus::CTransform& xf) { x478_assistAimXf = xf; }
    CGrappleArm& GetGrappleArm() { return *x740_grappleArm; }
    void DamageRumble(const zeus::CVector3f& location, float damage, const CStateManager& mgr);
    void ResetCharge(CStateManager& mgr, bool b1);
    void HandleBeamChange(const CFinalInput& input, CStateManager& mgr);
    void HandlePhazonBeamChange(CStateManager& mgr);
    void HandleWeaponChange(const CFinalInput& input, CStateManager& mgr);
    void ProcessInput(const CFinalInput& input, CStateManager& mgr);
    void ResetIdle(CStateManager& mgr);
    void CancelFiring(CStateManager& mgr);
    float GetBeamVelocity() const;
    void StopContinuousBeam(CStateManager& mgr, bool b1);
    void Update(float grappleSwingT, float cameraBobT, float dt, CStateManager& mgr);
    void PreRender(const CStateManager& mgr, const zeus::CFrustum& frustum, const zeus::CVector3f& camPos);
    void Render(const CStateManager& mgr, const zeus::CVector3f& pos, const CModelFlags& flags) const;
    void AddToRenderer(const zeus::CFrustum& frustum, const CStateManager& mgr) const;
    bool GetFiring() const { return x2ec_firing != 0; }
    TUniqueId DropPowerBomb(CStateManager& mgr);
    void SetActorAttached(bool b) { x835_31_actorAttached = b; }
};

}

#endif // __URDE_CPLAYERGUN_HPP__
