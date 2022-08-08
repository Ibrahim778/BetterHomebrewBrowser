#include <json.h>
#include <paf/stdc.h>

#include "json.hpp"
#include "print.h"

using namespace sce;

PAFAllocator::PAFAllocator()
{

}

PAFAllocator::~PAFAllocator()
{

}

void *PAFAllocator::allocate(size_t size, void *)
{
    return sce_paf_malloc(size);
}

void PAFAllocator::deallocate(void *ptr, void *)
{
    sce_paf_free(ptr);
}

NullAccess::NullAccess()
{
    vboolean.set(false);
    vinteger.set((int64_t)-99);
    vuinteger.set((uint64_t)99);
    vreal.set((double)-0.99);
    vstring.set(Json::String("invalid string"));
    vnull.set(Json::kValueTypeNull);
}

NullAccess::~NullAccess()
{

}

const Json::Value& NullAccess::NullAccessCB(Json::ValueType accessType, const Json::Value *parent, void *context)
{
    print("CB(%d)[parent:%d]\n", accessType, parent->getType());

    switch(accessType)
    {
    case Json::kValueTypeBoolean:
        return reinterpret_cast<NullAccess*>(context)->vboolean;
    case Json::kValueTypeInteger:
        return reinterpret_cast<NullAccess*>(context)->vinteger;
    case Json::kValueTypeUInteger:
        return reinterpret_cast<NullAccess*>(context)->vuinteger;
    case Json::kValueTypeReal:
        return reinterpret_cast<NullAccess*>(context)->vreal;
    case Json::kValueTypeString:
        return reinterpret_cast<NullAccess*>(context)->vstring;
    default:
        break;
    }

    return (reinterpret_cast<NullAccess*>(context))->vnull;
}
