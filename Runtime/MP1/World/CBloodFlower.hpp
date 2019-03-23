#pragma once

#include "Weapon/CProjectileInfo.hpp"
#include "World/CPatterned.hpp"

namespace urde {
class CGenDescription;
class CElementGen;
class CWeaponDescription;
}

namespace urde::MP1 {
class CBloodFlower : public CPatterned {
  TLockedToken<CGenDescription> x568_podEffectDesc;
  std::unique_ptr<CElementGen> x574_podEffect;
  TLockedToken<CWeaponDescription> x578_;
  float x584_curAttackTime = 0.f;
  float x588_projectileOffset = 0.f;
  u32 x58c_projectileState = 0;
  CProjectileInfo x590_projectileInfo;
  float x5b8_ = 0.f;
  float x5bc_projectileDelay = 0.f;
  float x5c0_ = 0.f;
  TLockedToken<CGenDescription> x5c4_;
  s16 x5d4_;
  u32 x5d8_effectState = 0;
  CDamageInfo x5dc_;
  CDamageInfo x5f8_podDamage;
  float x614_;
  CAssetId x618_;
  CAssetId x61c_;
  CAssetId x620_;

  void ActivateTriggers(CStateManager& mgr, bool activate);
  void sub80119364(CStateManager&);
  void UpdateFire(CStateManager& mgr);
  void TurnEffectsOn(u32, CStateManager&);
  void TurnEffectsOff(u32, CStateManager&);
  void LaunchPollenProjectile(const zeus::CTransform&, CStateManager&, float, s32);
public:
  DEFINE_PATTERNED(BloodFlower)

  CBloodFlower(TUniqueId, std::string_view, const CEntityInfo&, const zeus::CTransform&, CModelData&&,
               const CPatternedInfo&, CAssetId, CAssetId, const CActorParameters&, CAssetId, const CDamageInfo&,
               const CDamageInfo&, const CDamageInfo&, CAssetId, CAssetId, CAssetId, float, CAssetId, u32);

  void AcceptScriptMsg(EScriptObjectMessage, TUniqueId, CStateManager&);
  void Think(float dt, CStateManager& mgr);
  void DoUserAnimEvent(CStateManager& mgr, const CInt32POINode& node, EUserEventType type, float dt);
  void Render(const CStateManager& mgr) const;
  CProjectileInfo* GetProjectileInfo() { return &x590_projectileInfo; }


  bool ShouldAttack(CStateManager&, float);
  bool ShouldTurn(CStateManager&, float);
  bool Leash(CStateManager&, float) { return x5c0_ < x3d0_playerLeashTime; }
  void Active(CStateManager&, EStateMsg, float);
  void InActive(CStateManager&, EStateMsg, float);
  void BulbAttack(CStateManager&, EStateMsg, float);
  void PodAttack(CStateManager&, EStateMsg, float);
};
} // namespace urde::MP1
