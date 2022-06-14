#ifndef JSON_H
#define JSON_H

#include <paf/stdc.h>
#include <json.h>

class PAFAllocator : public sce::Json::MemAllocator
{
public:
    PAFAllocator();
    ~PAFAllocator();

    virtual void *allocate(size_t size, void *);
    virtual void deallocate(void *ptr, void *);
};

class NullAccess
{
public:
	sce::Json::Value vboolean;
	sce::Json::Value vinteger;
	sce::Json::Value vuinteger;
	sce::Json::Value vreal;
	sce::Json::Value vstring;
	sce::Json::Value vnull;

    NullAccess();
    ~NullAccess();

    static const sce::Json::Value& NullAccessCB(sce::Json::ValueType accesstype, const sce::Json::Value* parent, void* context);
};

#endif