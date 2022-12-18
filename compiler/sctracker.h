/* vim: set sts=2 ts=8 sw=2 tw=99 et: */
#ifndef _INCLUDE_SOURCEPAWN_COMPILER_TRACKER_H_
#define _INCLUDE_SOURCEPAWN_COMPILER_TRACKER_H_

#include <memory>

#include "lexer.h"
#include "pool-allocator.h"

class CompileContext;
class SemaContext;

struct funcenum_t {
    funcenum_t()
     : tag(0),
       name(),
       anonymous(false)
    {}
    int tag;
    sp::Atom* name;
    PoolArray<functag_t*> entries;
    bool anonymous;
};

struct structarg_t : public PoolObject
{
    structarg_t()
      : type(),
        name(nullptr),
        offs(0),
        index(0)
    {}

    typeinfo_t type;
    sp::Atom* name;
    unsigned int offs;
    int index;
};

struct pstruct_t : public PoolObject
{
    explicit pstruct_t(sp::Atom* name);

    sp::Atom* name;
    PoolArray<structarg_t*> args;
};

struct methodmap_method_t : public PoolObject
{
    explicit methodmap_method_t(methodmap_t* parent)
     : name(),
       parent(parent),
       target(nullptr),
       getter(nullptr),
       setter(nullptr),
       is_static(false)
    {}

    sp::Atom* name;
    methodmap_t* parent;
    symbol* target;
    symbol* getter;
    symbol* setter;
    bool is_static;

    int property_tag() const;
};

struct methodmap_t : public SymbolData
{
    methodmap_t(methodmap_t* parent, sp::Atom* name);

    methodmap_t* asMethodmap() override { return this; }

    methodmap_t* parent;
    int tag;
    bool nullable;
    bool keyword_nullable;
    sp::Atom* name;
    PoolMap<sp::Atom*, methodmap_method_t*> methods;

    bool must_construct_with_new() const {
        return nullable || keyword_nullable;
    }

    // Shortcut.
    methodmap_method_t* dtor;
    methodmap_method_t* ctor;

    // Set in MethodmapDecl::Bind.
    bool is_bound;

    // Original enum list.
    EnumData* enum_data;
};

/**
 * Pawn Structs
 */
pstruct_t* pstructs_add(sp::Atom* name);
void pstructs_free();
pstruct_t* pstructs_find(const char* name);
const structarg_t* pstructs_getarg(const pstruct_t* pstruct, sp::Atom* name);

/**
 * Function enumeration tags
 */
void funcenums_free();
funcenum_t* funcenums_add(CompileContext& cc, sp::Atom* name, bool anonymous);
funcenum_t* funcenum_for_symbol(CompileContext& cc, symbol* sym);
functag_t* functag_from_tag(int tag);

/**
 * Method maps.
 */
methodmap_t* methodmap_add(CompileContext& cc, methodmap_t* parent, sp::Atom* name);
methodmap_t* methodmap_find_by_name(sp::Atom* name);
methodmap_method_t* methodmap_find_method(methodmap_t* map, sp::Atom* name);
void methodmaps_free();

#endif //_INCLUDE_SOURCEPAWN_COMPILER_TRACKER_H_
