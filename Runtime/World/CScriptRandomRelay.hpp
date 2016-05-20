#ifndef CSCRIPTRANDOMRELAY_HPP
#define CSCRIPTRANDOMRELAY_HPP

#include "CEntity.hpp"

namespace urde
{
class CScriptRandomRelay : public CEntity
{
    u32 x34_connectionCount;
    u32 x38_variance;
    bool x3c_clamp;
public:
    CScriptRandomRelay(TUniqueId, const std::string&, const CEntityInfo&, s32, s32, bool, bool);
};
}

#endif // CSCRIPTRANDOMRELAY_HPP