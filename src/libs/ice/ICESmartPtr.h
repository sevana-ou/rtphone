// Based on resiprocate's implementation of smart pointer, which is based on boost implementation
// Its license is smth close to BSD

#ifndef __ICE_SMART_PTR_H
#define __ICE_SMART_PTR_H

#ifndef USE_NATIVE_SMARTPTR

#include "ICESmartCount.h"
#include <memory>               // for std::auto_ptr
#include <algorithm>            // for std::swap
#include <functional>           // for std::less
#include <typeinfo>             // for std::bad_cast
#include <iosfwd>               // for std::basic_ostream
#include <cassert>

namespace ice
{


template<class T> class enable_shared_from_this;

struct static_cast_tag {};
struct const_cast_tag {};
struct dynamic_cast_tag {};
struct polymorphic_cast_tag {};

template<class T> struct SmartPtr_traits
{
   typedef T & reference;
};

template<> struct SmartPtr_traits<void>
{
   typedef void reference;
};

template<> struct SmartPtr_traits<void const>
{
   typedef void reference;
};

template<> struct SmartPtr_traits<void volatile>
{
   typedef void reference;
};

template<> struct SmartPtr_traits<void const volatile>
{
   typedef void reference;
};

// enable_shared_from_this support

template<class T, class Y> void sp_enable_shared_from_this( shared_count const & pn, enable_shared_from_this<T> const * pe, Y const * px )
{
   if(pe != 0) pe->_internal_weak_this._internal_assign(const_cast<Y*>(px), pn);
}

inline void sp_enable_shared_from_this( shared_count const & /*pn*/, ... )
{
}

//
//  SmartPtr
//
//  Reference counted copy semantics.
//  The object pointed to is deleted when the last SmartPtr pointing to it
//  is destroyed or reset.
//

template<class T> class SmartPtr
{
private:

   // Borland 5.5.1 specific workaround
   typedef SmartPtr<T> this_type;

public:

   typedef T element_type;
   typedef T value_type;
   typedef T * pointer;
   typedef typename SmartPtr_traits<T>::reference reference;

   SmartPtr(): px(0), pn() // never throws in 1.30+
   {
   }

   template<class Y>
   explicit SmartPtr(Y * p): px(p), pn(p, checked_deleter<Y>()) // Y must be complete
   {
       sp_enable_shared_from_this( pn, p, p );
   }

   //
   // Requirements: D's copy constructor must not throw
   //
   // SmartPtr will release p by calling d(p)
   //

   template<class Y, class D> SmartPtr(Y * p, D d): px(p), pn(p, d)
   {
       sp_enable_shared_from_this( pn, p, p );
   }

//  generated copy constructor, assignment, destructor are fine...

//  except that Borland C++ has a bug, and g++ with -Wsynth warns
#if defined(__BORLANDC__) || defined(__GNUC__)
   SmartPtr & operator=(SmartPtr const & r) // never throws
   {
       px = r.px;
       pn = r.pn; // shared_count::op= doesn't throw
       return *this;
   }
#endif

   template<class Y>
   SmartPtr(SmartPtr<Y> const & r): px(r.px), pn(r.pn) // never throws
   {
   }

   template<class Y>
   SmartPtr(SmartPtr<Y> const & r, static_cast_tag): px(static_cast<element_type *>(r.px)), pn(r.pn)
   {
   }

   template<class Y>
   SmartPtr(SmartPtr<Y> const & r, const_cast_tag): px(const_cast<element_type *>(r.px)), pn(r.pn)
   {
   }

   template<class Y>
   SmartPtr(SmartPtr<Y> const & r, dynamic_cast_tag): px(dynamic_cast<element_type *>(r.px)), pn(r.pn)
   {
      if(px == 0) // need to allocate new counter -- the cast failed
      {
         pn = /*resip::*/shared_count();
      }
   }

   template<class Y>
   SmartPtr(SmartPtr<Y> const & r, polymorphic_cast_tag): px(dynamic_cast<element_type *>(r.px)), pn(r.pn)
   {
      if(px == 0)
      {
         throw std::bad_cast();
      }
   }

   template<class Y>
   explicit SmartPtr(std::auto_ptr<Y> & r): px(r.get()), pn()
   {
      Y * tmp = r.get();
      pn = shared_count(r);
      sp_enable_shared_from_this( pn, tmp, tmp );
   }

   template<class Y>
   SmartPtr & operator=(SmartPtr<Y> const & r) // never throws
   {
      px = r.px;
      pn = r.pn; // shared_count::op= doesn't throw
      return *this;
   }

   template<class Y>
   SmartPtr & operator=(std::auto_ptr<Y> & r)
   {
      this_type(r).swap(*this);
      return *this;
   }

   void reset() // never throws in 1.30+
   {
      this_type().swap(*this);
   }

   template<class Y> void reset(Y * p) // Y must be complete
   {
      assert(p == 0 || p != px); // catch self-reset errors
      this_type(p).swap(*this);
   }

   template<class Y, class D> void reset(Y * p, D d)
   {
      this_type(p, d).swap(*this);
   }

   reference operator* () const // never throws
   {
      assert(px != 0);
      return *px;
   }

   T * operator-> () const // never throws
   {
      //assert(px != 0);
      return px;
   }
    
   T * get() const // never throws
   {
      return px;
   }

   // implicit conversion to "bool"
#if defined(__SUNPRO_CC) // BOOST_WORKAROUND(__SUNPRO_CC, <= 0x530)
   operator bool () const
   {
      return px != 0;
   }
#elif defined(__MWERKS__) // BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3003))
   typedef T * (this_type::*unspecified_bool_type)() const;
   operator unspecified_bool_type() const // never throws
   {
      return px == 0? 0: &this_type::get;
   }
#else 
   typedef T * this_type::*unspecified_bool_type;
   operator unspecified_bool_type() const // never throws
   {
      return px == 0? 0: &this_type::px;
   }
#endif

   // operator! is redundant, but some compilers need it
   bool operator! () const // never throws
   {
      return px == 0;
   }

   bool unique() const // never throws
   {
      return pn.unique();
   }

   long use_count() const // never throws
   {
      return pn.use_count();
   }

   void swap(SmartPtr<T> & other) // never throws
   {
      std::swap(px, other.px);
      pn.swap(other.pn);
   }

   template<class Y> bool _internal_less(SmartPtr<Y> const & rhs) const
   {
      return pn < rhs.pn;
   }

   void * _internal_get_deleter(std::type_info const & ti) const
   {
      return pn.get_deleter(ti);
   }

// Tasteless as this may seem, making all members public allows member templates
// to work in the absence of member template friends. (Matthew Langston)

private:

   template<class Y> friend class SmartPtr;

   T * px;                     // contained pointer
   shared_count pn;    // reference counter

};  // SmartPtr

template<class T, class U> inline bool operator==(SmartPtr<T> const & a, SmartPtr<U> const & b)
{
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=(SmartPtr<T> const & a, SmartPtr<U> const & b)
{
    return a.get() != b.get();
}

#if __GNUC__ == 2 && __GNUC_MINOR__ <= 96

// Resolve the ambiguity between our op!= and the one in rel_ops

template<class T> inline bool operator!=(SmartPtr<T> const & a, SmartPtr<T> const & b)
{
    return a.get() != b.get();
}

#endif

template<class T, class U> inline bool operator<(SmartPtr<T> const & a, SmartPtr<U> const & b)
{
    return a._internal_less(b);
}

template<class T> inline void swap(SmartPtr<T> & a, SmartPtr<T> & b)
{
    a.swap(b);
}

template<class T, class U> SmartPtr<T> static_pointer_cast(SmartPtr<U> const & r)
{
    return SmartPtr<T>(r, static_cast_tag());
}

template<class T, class U> SmartPtr<T> const_pointer_cast(SmartPtr<U> const & r)
{
    return SmartPtr<T>(r, const_cast_tag());
}

template<class T, class U> SmartPtr<T> dynamic_pointer_cast(SmartPtr<U> const & r)
{
    return SmartPtr<T>(r, dynamic_cast_tag());
}

// shared_*_cast names are deprecated. Use *_pointer_cast instead.

template<class T, class U> SmartPtr<T> shared_static_cast(SmartPtr<U> const & r)
{
    return SmartPtr<T>(r, static_cast_tag());
}

template<class T, class U> SmartPtr<T> shared_dynamic_cast(SmartPtr<U> const & r)
{
    return SmartPtr<T>(r, dynamic_cast_tag());
}

template<class T, class U> SmartPtr<T> shared_polymorphic_cast(SmartPtr<U> const & r)
{
    return SmartPtr<T>(r, polymorphic_cast_tag());
}

template<class T, class U> SmartPtr<T> shared_polymorphic_downcast(SmartPtr<U> const & r)
{
    assert(dynamic_cast<T *>(r.get()) == r.get());
    return shared_static_cast<T>(r);
}

template<class T> inline T * get_pointer(SmartPtr<T> const & p)
{
    return p.get();
}

// operator<<
#if defined(__GNUC__) &&  (__GNUC__ < 3)
template<class Y> std::ostream & operator<< (std::ostream & os, SmartPtr<Y> const & p)
{
    os << p.get();
    return os;
}
#else
template<class E, class T, class Y> std::basic_ostream<E, T> & operator<< (std::basic_ostream<E, T> & os, SmartPtr<Y> const & p)
{
    os << p.get();
    return os;
}
#endif

// get_deleter (experimental)
#if (defined(__GNUC__) &&  (__GNUC__ < 3)) || (defined(__EDG_VERSION__) && (__EDG_VERSION__ <= 238))
// g++ 2.9x doesn't allow static_cast<X const *>(void *)
// apparently EDG 2.38 also doesn't accept it
template<class D, class T> D * get_deleter(SmartPtr<T> const & p)
{
    void const * q = p._internal_get_deleter(typeid(D));
    return const_cast<D *>(static_cast<D const *>(q));
}
#else
template<class D, class T> D * get_deleter(SmartPtr<T> const & p)
{
    return static_cast<D *>(p._internal_get_deleter(typeid(D)));
}
#endif

}

#endif

#endif

