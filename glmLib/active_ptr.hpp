/* -*- Mode: c++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * Copyright (C) 2023 RPf <gpl3@pfeifer-syscon.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <iostream>
#include <atomic>
#include <mutex>
#include <type_traits>
#include <typeinfo>
#include <unistd.h> // usleep



namespace psc {
namespace mem {

static constexpr bool active_debug{false};

    /*
     * this is crude implementation of a shared pointer pattern
     *   with a twist that it is resetable.
     * - a active_ptr is just a possible valid pointer
     * - use active_ptr.lease to create active_lease that can be checked with, if the lease is usable
     * - a active_ptr can be reseted that will free the used data after all leases have expired
     * ...
     * - it is known that this is highly inefficent
     * - it is not prepared for multithreading
     */
    template<typename T,typename A = T>
    class active_ctl
    {
    public:
        active_ctl()
        {
        }
        ~active_ctl()
        {
            if (active_debug) {
            std::cout << "~active_ctl "
                      << (*this) << std::endl;
            }
        }

        void inc_use()
        {
           ++m_usecount;
        }
        bool dec_use()
        {
            if (m_usecount > 0u) {
                --m_usecount;
                if (m_usecount == 0u) {
                    reset();
                    return true;
                }  // ready to reset
            }
            else {
                if (active_debug) {
                std::cout << "active_ctl use already 0! "
                          << (*this) << std::endl;
                }
            }
            return false;
        }

        void inc_lease()
        {
            ++m_leasecount;
        }

        bool dec_lease()
        {
            if (m_leasecount > 0u) {
                --m_leasecount;
            }
            else {
                if (active_debug) {
                std::cout << "active_ctl lease already 0! "
                          << (*this) << std::endl;
                }
            }
            return m_defered_reset && m_leasecount == 0u;
        }
        inline bool is_active()
        {
            return m_ptr && !m_defered_reset;
        }
        inline T* get_ptr() const
        {
            return m_ptr;
        }
        inline void set_defered_reset(bool defered_reset)
        {
            m_defered_reset = defered_reset;
        }
        inline uint32_t get_usecount() const
        {
            return m_usecount;
        }
        inline uint32_t get_leasecount() const
        {
            return m_leasecount;
        }
        template<typename...Args>
        void alloc(std::allocator<T> alloc, Args&&... args)
        {
            m_alloc = alloc;
            using traits_t = std::allocator_traits<decltype(alloc)>; // The matching trait
            m_ptr = traits_t::allocate(alloc, 1);         // with count
            traits_t::construct(alloc, m_ptr, args...);     // construct the element
        }
        void reset()
        {
            if (get_leasecount() == 0u) {
                if (active_debug) {
                std::cout << "active_ctl::reset dealloc "
                          << (*this)
                          << std::endl;
                }
                T* ptr = get_ptr();
                if (ptr) {
                    m_ptr = nullptr;
                    using traits_t = std::allocator_traits<decltype(m_alloc)>; // The matching trait
                    traits_t::destroy(m_alloc, ptr);
                    traits_t::deallocate(m_alloc, ptr, 1);
                    set_defered_reset(false);    // no need to check again
                }
            }
            else {
                if (active_debug) {
                std::cout << "active_ctl::reset defere"
                          << (*this)
                          << std::endl;
                }
                set_defered_reset(true);
            }
        }
        std::mutex& get_mutex()
        {
            return m_changeMutex;
        }
        inline bool is_unreferenced()
        {
            return get_usecount() == 0u && get_leasecount() == 0u;
        }

        friend std::ostream& operator<<(std::ostream& os, const active_ctl<T>& cntl)
        {
            os << "addr " << std::hex << cntl.get_ptr() << std::dec
               << " use " << cntl.get_usecount()
               << " lease " << cntl.get_leasecount();
            return os;
        }

    protected:
        std::allocator<A> get_alloc() const
        {
            return m_alloc;
        }
    private:
        // Shared pointer
        T* m_ptr{nullptr};
        std::atomic_uint32_t m_usecount{0u};
        std::atomic_uint32_t m_leasecount{0u};
        bool m_defered_reset{false};
        std::allocator<A> m_alloc;
        mutable std::mutex m_changeMutex; // serialize decrement+free
    };


    /**
     * T is the part created to use the pointer
     **/
    template<typename T, typename A = T>
    class active_lease
    {
    public:
        active_lease(active_ctl<T>* active_ctl)
        : m_active_ctl{active_ctl}
        {
            if (m_active_ctl) {
                m_ptr = dynamic_cast<T*>(m_active_ctl->get_ptr());  // only convert once
                if (m_ptr) {
                    m_active_ctl->inc_lease();
                    m_incremented = true;
                }
            }
        }
        ~active_lease()
        {
            if (active_debug) {
            std::cout << "~active_lease "
                      << (*this)
                      << std::endl;
            }
            if (m_incremented) {
                std::lock_guard<std::mutex> lock(m_active_ctl->get_mutex());
                if (m_active_ctl->dec_lease()) {
                    m_active_ctl->reset();
                }
            }
            removeControlIfUnsued();
        }
        // make sure this is only for local use, the rest is in the responsibility of the user
        explicit active_lease(const active_lease<T>& active_lease) = delete;
        active_lease<T>& operator=(const active_lease<T>& sp) = delete;

        // Overload * operator
        T& operator*()
        {
          return *m_ptr;
        }

        T* get() const
        {
          return m_ptr;
        }

        // Overload -> operator
        T* operator->()
        {
          return m_ptr;
        }

        explicit operator bool() const noexcept
        {
            return m_ptr != nullptr;
        }

        // try to avoid this
//        active_lease<T>& operator=(const active_lease<T>& lease)
//        {
//            if (m_incremented && m_active_ctl->dec_lease()) {
//                m_active_ctl->reset();
//            }
//            removeControlIfUnsued();
//            m_active_ctl = lease.m_active_ctl;
//            m_ptr = m_active_ctl;
//            if (m_ptr) {
//                m_active_ctl->inc_lease();
//                m_incremented = true;
//            }
//            else {
//                m_incremented = false;
//            }
//            return *this;
//        }

        friend std::ostream& operator<<(std::ostream& os, const active_lease<T>& lease)
        {
            os << "addr " << std::hex << lease.get() << std::dec
               << " lease " << lease.m_active_ctl->get_leasecount();
            return os;
        }
    private:
        void removeControlIfUnsued()
        {
            if (m_active_ctl
             && m_active_ctl->is_unreferenced()) {
                if (active_debug) {
                std::cout << "active_lease::removeControlIfUnsued delete "
                          << (*this)
                          << std::endl;
                }
                delete m_active_ctl;
                m_active_ctl = nullptr;
            }
        }

        active_ctl<T,A>* m_active_ctl;
        T* m_ptr{nullptr};
        bool m_incremented{false};

    };


    /**
     * T is the outer type (which may be incorrect, but we try to cast on pointer access)
     **/
    template<typename T, typename A = T>
    class active_ptr
    {
    public:
        // Constructor
        active_ptr(active_ctl<T,A>* active_ctl = nullptr)
        : m_active_ctl{active_ctl}
        {
            if (m_active_ctl) { // count only "used" instances
                m_active_ctl->inc_use();
            }
            if (active_debug) {
            std::cout << "active_ptr default const. "
                      << (*this)
                      << std::endl;
            }
        }

        // Copy constructor
        active_ptr(const active_ptr<T>& sp)
        : m_active_ctl{sp.m_active_ctl}
        {
            if (active_debug) {
            std::cout << "active_ptr copy lval "
                      << (*this)
                      << std::endl;
            }
            if (m_active_ctl) {
                m_active_ctl->inc_use();
            }
        }

        // Copy constructor
        active_ptr(active_ptr<T>&& sp)
        : m_active_ctl{sp.m_active_ctl}
        {
            if (active_debug) {
            std::cout << "active_ptr copy rval "
                      << (*this)
                      << std::endl;
            }
        }

        // conversion
        template<typename C
                ,typename std::enable_if<std::is_assignable<T&&, C>::value>::type* = nullptr>
        active_ptr(const active_ptr<C>& r)
        {
            // the template represents only the extern type, as we use dynamic cast on access this should work
            m_active_ctl = reinterpret_cast<active_ctl<T,A>*>(r.get_control());
            if (active_debug) {
            std::cout << "active_ptr conversion "
                      << (*this)
                      << std::endl;
            }
            if (m_active_ctl) {
                m_active_ctl->inc_use();
            }
        }
        /**
         */
        constexpr active_ptr(nullptr_t) noexcept
        : active_ptr()
        {
        }

        // Destructor
        ~active_ptr()
        {
            //std::lock_guard<std::mutex> guard(m_changeMutex);   // better to be safe than sorry???
            if (active_debug) {
            std::cout << "~active_ptr "
                      << (*this) << std::endl;
            }
            if (m_active_ctl) {
                if (m_active_ctl->is_active()) {
                    std::lock_guard<std::mutex> lock(m_active_ctl->get_mutex());
                    m_active_ctl->dec_use();
                }
                else {  // if inactive no locking required ...
                    m_active_ctl->dec_use();
                }
            }
            removeControlIfUnsued();
        }

        active_ptr<T>& operator=(const active_ptr<T>& sp)
        {
            if (m_active_ctl) {
                m_active_ctl->dec_use();
            }
            removeControlIfUnsued();
            m_active_ctl = sp.m_active_ctl;
            if (active_debug) {
            std::cout << "active_ptr assign lval "
                      << (*this)
                      << std::endl;
            }
            if (m_active_ctl) {
                m_active_ctl->inc_use();
            }
            return *this;
        }

        active_ptr<T>& operator=(active_ptr<T>&& sp)
        {
            if (m_active_ctl) {
                m_active_ctl->dec_use();
            }
            removeControlIfUnsued();
            m_active_ctl = sp.m_active_ctl;
            if (active_debug) {
            std::cout << "active_ptr assign rval "
                      << (*this)
                      << std::endl;
            }
            //m_active_ctl->inc_use();
            sp.m_active_ctl = nullptr;      // mark as "empty", some cases are guarded but not all...
            return *this;
        }

        bool operator==(const active_ptr<T>& sp) const noexcept
        {
            return m_active_ctl == sp.m_active_ctl;
        }

        void reset()
        {
            if (m_active_ctl) {
                std::lock_guard<std::mutex> lock(m_active_ctl->get_mutex());
                m_active_ctl->reset();
            }
        }

        // Reference count
        uint32_t use_count() const
        {
            return m_active_ctl
                    ? m_active_ctl->get_usecount()
                    : 0u;
        }

        uint32_t lease_count() const
        {
            return m_active_ctl
                    ? m_active_ctl->get_leasecount()
                    : 0u;
        }

        active_ctl<T,A>* get_control() const
        {
            return m_active_ctl;
        }


        // Get the pointer, use only for testing !
        T* get() const
        {
          return dynamic_cast<T*>(m_active_ctl
                                    ? m_active_ctl->get_ptr()
                                    : nullptr);
        }

        explicit operator bool() const noexcept
        {
            return m_active_ctl && m_active_ctl->is_active();
        }

        active_lease<T> lease() const
        {
            return active_lease<T>(m_active_ctl);
        }

        void removeControlIfUnsued()
        {
            if (active_debug) {
            std::cout << "active_ptr::removeControlIfUnsued"
                      << std::endl;
            }
            if (m_active_ctl
             && m_active_ctl->is_unreferenced()) {
                delete m_active_ctl;
                m_active_ctl = nullptr;
            }
        }


        friend std::ostream& operator<<(std::ostream& os, const active_ptr<T>& ptr)
        {
            os << " addr " << std::hex << ptr.get() << std::dec
               << " use " << ptr.use_count()
               << " lease " << ptr.lease_count();
            return os;
        }

        using element_type = T;
    private:
        active_ctl<T,A>* m_active_ctl;
    };


    template<typename T, typename...Args>
    inline active_ptr<T>
    make_active(Args&&... args)
    {
        auto m_active_ctl = new active_ctl<T>();
        std::allocator<T> alloc;
        m_active_ctl->alloc(alloc, std::forward<Args>(args)...);
        active_ptr<T> active_ptr{m_active_ctl};
        return active_ptr;
    }



    /// Convert type of `active_ptr` rvalue, via `dynamic_cast`
    /// @since C++20
    //template<typename T, typename U>
    //inline active_ptr<T>
    //dynamic_pointer_cast(active_ptr<U>&& r) noexcept
    //{
    //    using S = active_ptr<T>;
    //    if (auto* p = dynamic_cast<typename S::element_type*>(r.get())) {
    //        return S(std::move(r));
    //    }
    //    return S();
    //}

    /// Convert type of `active_ptr` lvalue, via `dynamic_cast`
    /// @since C++20
    template<typename T, typename U>
    inline active_ptr<T>
    dynamic_pointer_cast(const active_ptr<U>& r) noexcept
    {
        using S = active_ptr<T>;
        if (auto* p = dynamic_cast<typename S::element_type*>(r.get())) {
            return S{reinterpret_cast<active_ctl<T>*>(r.get_control())};
        }
        return S();
    }

} /* namespace mem */
} /* namespace psc */
