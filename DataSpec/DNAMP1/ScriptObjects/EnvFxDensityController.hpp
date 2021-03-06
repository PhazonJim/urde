#pragma once

#include "../../DNACommon/DNACommon.hpp"
#include "IScriptObject.hpp"
#include "Parameters.hpp"

namespace DataSpec::DNAMP1 {
struct EnvFxDensityController : IScriptObject {
  AT_DECL_DNA_YAMLV
  String<-1> name;
  Value<bool> active;
  Value<float> density;
  Value<atUint32> maxDensityDeltaSpeed;
};
} // namespace DataSpec::DNAMP1
