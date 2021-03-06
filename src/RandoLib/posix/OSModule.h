/*
 * This file is part of selfrando.
 * Copyright (c) 2015-2017 Immunant Inc.
 * For license information, see the LICENSE file
 * included with selfrando.
 *
 */

#pragma once

#include <RandoLib.h>
#include <TrapInfo.h>

struct FunctionList;
struct Function;

#ifdef __cplusplus
#include <utility>
#include <new>

#include "util/hashmap.h"

namespace os {

extern "C" {
#include "ModuleInfo.h"
}

class Module : public ModuleBase<Module> {
public:
    typedef ModuleInfo *Handle;
    typedef struct dl_phdr_info *PHdrInfoPointer;

    Module() = delete;
    RANDO_SECTION Module(Handle dynamic_ptr, PHdrInfoPointer phdr_info = nullptr);
    RANDO_SECTION ~Module();

    class Address : public ModuleBase<Module>::AddressBase<Address> {
    public:
        using ModuleBase<Module>::AddressBase<Address>::AddressBase;

        template<typename T = BytePointer>
        inline RANDO_SECTION T to_ptr() const {
            switch (m_space) {
            case AddressSpace::MEMORY:
            case AddressSpace::TRAP:
                return reinterpret_cast<T>(m_address);
            case AddressSpace::RVA:
                //return reinterpret_cast<T>(m_address + reinterpret_cast<uintptr_t>(m_module.m_phdr_info.dlpi_addr));
                return reinterpret_cast<T>(m_address + static_cast<uintptr_t>(m_module.m_phdr_info.dlpi_addr));
            default:
                return 0;
            }
        }
    };

    class Relocation : public ModuleBase<Module>::RelocationBase<size_t> {
    public:
        Relocation() = delete;

        template<typename Ptr>
        Relocation(const Module &mod, Ptr ptr, Type type)
            : RelocationBase(mod, ptr, type),
              m_has_symbol_ptr(false), m_symbol_ptr(nullptr), m_addend(0) { }

        template<typename Ptr>
        Relocation(const Module &mod, Ptr ptr, Type type, ptrdiff_t addend)
            : RelocationBase(mod, ptr, type),
              m_has_symbol_ptr(false), m_symbol_ptr(nullptr), m_addend(addend) { }

        Relocation(const Module &mod, const trap_reloc_t &reloc)
            : RelocationBase(mod, Address::from_trap(mod, reloc.address).to_ptr(), reloc.type),
              m_symbol_ptr(Address::from_trap(mod, reloc.symbol).to_ptr()), m_addend(reloc.addend) {
            m_has_symbol_ptr = (reloc.symbol != 0); // FIXME: what if zero addresses are legit???
        }

        // TODO: would be nice to move these into RelocationBase
        BytePointer get_target_ptr() const;
        void set_target_ptr(BytePointer);

        static Type get_pointer_reloc_type();

        static void fixup_export_trampoline(BytePointer*, const Module&, FunctionList*);
        static void fixup_entry_point(const Module&, uintptr_t, uintptr_t);

        inline ptrdiff_t get_addend() const {
            return m_addend;
        }

        BytePointer get_got_entry() const;

    private:
        bool m_has_symbol_ptr;
        const BytePointer m_symbol_ptr;
        ptrdiff_t m_addend;
    };

    class Section : public ModuleBase<Module>::SectionBase<Address> {
    public:
        using ModuleBase<Module>::SectionBase<Address>::SectionBase;

        RANDO_SECTION PagePermissions change_permissions(PagePermissions perms) const;

        RANDO_SECTION void flush_icache();
    };

public:
    // FIXME: TrapInfo could be pre-computed, and accessed via a function
    typedef void(*ExecSectionCallback)(const Module&, const Section&, ::TrapInfo&, bool, void*);
    RANDO_SECTION void for_all_exec_sections(bool, ExecSectionCallback, void*);

    typedef void(*ModuleCallback)(Module&, void*);
    static RANDO_SECTION void for_all_modules(ModuleCallback, void*);

    RANDO_SECTION void for_all_relocations(FunctionList *functions) const;

    RANDO_SECTION void preprocess_arch();
    RANDO_SECTION void relocate_arch(FunctionList *functions) const;

    inline RANDO_SECTION Section export_section() const {
        return Section(*this, m_module_info->program_info_table->xptramp_start,
                              m_module_info->program_info_table->xptramp_size);
    }

    inline RANDO_SECTION BytePointer get_got_ptr() const {
        return m_got;
    }

    inline RANDO_SECTION const char *get_module_name() const {
        return m_phdr_info.dlpi_name;
    }

#if RANDOLIB_WRITE_LAYOUTS
    void write_layout_file(FunctionList *functions,
                           size_t *shuffled_order) const;
#endif

    RANDO_SECTION void read_got_relocations(const TrapInfo *trap_info);

private:
    ModuleInfo *m_module_info;
    BytePointer m_image_base;
    BytePointer m_got;
    BytePointer m_eh_frame_hdr;

    // We keep our own copy of dl_phdr_info structure
    // since dl_iterate_phdr seems to reuse its own
    struct dl_phdr_info m_phdr_info;

    inline RANDO_SECTION Address RVA2Address(uintptr_t rva) const {
        return Address(*this, rva, AddressSpace::RVA);
    }

    enum RandoState : uint32_t {
        NOT_RANDOMIZED = 0, // This must be 0, to match the default
        RANDOMIZED = 1,
        CANT_RANDOMIZE = 2,
        SELF_RANDOMIZE = 3,
    };

    RANDO_SECTION void mark_randomized(RandoState);

    // Some loaders add the module base to all entries in .dynamic,
    // e.g., the glibc loaders, while others don't, e.g., the Android one.
    // We set this flag if the addresses have the base.
    bool m_dynamic_has_base;

    hashmap::HashMap<hashmap::PointerEntry<BytePointer>> m_got_entries;
    size_t m_linker_stubs;
};

} // namespace os
#endif // __cplusplus
