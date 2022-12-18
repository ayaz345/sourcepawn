/* vim: set ts=8 sts=4 sw=4 tw=99 et: */
#include "sctracker.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <utility>

#include <amtl/am-raii.h>
#include <amtl/am-vector.h>
#include "compile-context.h"
#include "lexer.h"
#include "sc.h"
#include "semantics.h"
#include "symbols.h"
#include "types.h"

std::vector<std::unique_ptr<funcenum_t>> sFuncEnums;
std::vector<methodmap_t*> sMethodmaps;

std::vector<pstruct_t*> sStructs;

pstruct_t::pstruct_t(sp::Atom* name)
  : name(name)
{
}

const structarg_t*
pstructs_getarg(const pstruct_t* pstruct, sp::Atom* name)
{
    for (const auto& arg : pstruct->args) {
        if (arg->name == name)
            return arg;
    }
    return nullptr;
}

pstruct_t*
pstructs_add(sp::Atom* name)
{
    auto p = new pstruct_t(name);
    sStructs.push_back(p);
    return sStructs.back();
}

void
pstructs_free()
{
    sStructs.clear();
}

pstruct_t*
pstructs_find(sp::Atom* name)
{
    for (const auto& p : sStructs) {
        if (p->name == name)
            return p;
    }
    return nullptr;
}

void
funcenums_free()
{
    sFuncEnums.clear();
}

funcenum_t* funcenums_add(CompileContext& cc, sp::Atom* name, bool anonymous) {
    if (anonymous) {
        if (auto type = cc.types()->find(name)) {
            assert(type->kind() == TypeKind::Function);
            assert(type->toFunction()->anonymous);
            return type->toFunction();
        }
    }

    auto e = std::make_unique<funcenum_t>();
    e->name = name;
    e->anonymous = anonymous;

    auto type = cc.types()->defineFunction(name, e.get());
    e->tag = type->tagid();

    sFuncEnums.push_back(std::move(e));
    return sFuncEnums.back().get();
}

funcenum_t* funcenum_for_symbol(CompileContext& cc, symbol* sym) {
    functag_t* ft = new functag_t;
    ft->ret_tag = sym->tag;

    std::vector<funcarg_t> args;
    for (arginfo& arg : sym->function()->args) {
        funcarg_t dest;
        dest.type = arg.type;

        if (dest.type.ident != iARRAY && dest.type.ident != iREFARRAY)
          assert(dest.type.dim.empty());

        args.emplace_back(dest);
    }
    new (&ft->args) PoolArray<funcarg_t>(args);

    auto name = ke::StringPrintf("::ft:%s:%d:%d", sym->name(), sym->addr(), sym->codeaddr);
    funcenum_t* fe = funcenums_add(cc, cc.atom(name), true);
    new (&fe->entries) PoolArray<functag_t*>({ft});

    return fe;
}

// Finds a functag that was created intrinsically.
functag_t*
functag_from_tag(int tag)
{
    Type* type = CompileContext::get().types()->find(tag);
    funcenum_t* fe = type->asFunction();
    if (!fe)
        return nullptr;
    if (fe->entries.empty())
        return nullptr;
    return fe->entries.back();
}

methodmap_t::methodmap_t(methodmap_t* parent, sp::Atom* name)
 : parent(parent),
   tag(0),
   nullable(false),
   keyword_nullable(false),
   name(name),
   dtor(nullptr),
   ctor(nullptr),
   is_bound(false),
   enum_data(nullptr)
{
}

int
methodmap_method_t::property_tag() const
{
    auto types = CompileContext::get().types();

    assert(getter || setter);
    if (getter)
        return getter->tag;
    if (setter->function()->args.size() != 2)
        return types->tag_void();
    arginfo* valp = &setter->function()->args[1];
    if (valp->type.ident != iVARIABLE)
        return types->tag_void();
    return valp->type.tag();
}

methodmap_t*
methodmap_add(CompileContext& cc, methodmap_t* parent, sp::Atom* name)
{
    auto map = new methodmap_t(parent, name);

    if (parent) {
        if (parent->nullable)
            map->nullable = parent->nullable;
        if (parent->keyword_nullable)
            map->keyword_nullable = parent->keyword_nullable;
    }

    map->tag = cc.types()->defineMethodmap(name->chars(), map)->tagid();
    sMethodmaps.push_back(std::move(map));

    return sMethodmaps.back();
}

methodmap_t*
methodmap_find_by_tag(int tag)
{
    return CompileContext::get().types()->find(tag)->asMethodmap();
}

methodmap_t*
methodmap_find_by_name(sp::Atom* name)
{
    auto type = CompileContext::get().types()->find(name);
    if (!type)
        return NULL;
    return methodmap_find_by_tag(type->tagid());
}

methodmap_method_t*
methodmap_find_method(methodmap_t* map, sp::Atom* name)
{
    auto iter = map->methods.find(name);
    if (iter != map->methods.end())
        return iter->second;

    if (map->parent)
        return methodmap_find_method(map->parent, name);
    return nullptr;
}

void
methodmaps_free()
{
    sMethodmaps.clear();
}
