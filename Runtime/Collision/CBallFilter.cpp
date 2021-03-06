#include "Runtime/Collision/CBallFilter.hpp"
#include "Runtime/Collision/CollisionUtil.hpp"

namespace urde {

void CBallFilter::Filter(const CCollisionInfoList& in, CCollisionInfoList& out) const {
  CollisionUtil::AddAverageToFront(in, out);
}

} // namespace urde
