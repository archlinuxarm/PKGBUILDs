// <forward_list.tcc> -*- C++ -*-

// Copyright (C) 2008, 2009 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file forward_list.tcc
 *  This is a Standard C++ Library header.
 */

#ifndef _FORWARD_LIST_TCC
#define _FORWARD_LIST_TCC 1

_GLIBCXX_BEGIN_NAMESPACE(std)

  template<typename _Alloc>
    void
    _Fwd_list_node_base<_Alloc>::
    _M_transfer_after(_Pointer __bbegin)
    {
      _Pointer __bend = __bbegin;
      while (__bend && __bend->_M_next)
	__bend = __bend->_M_next;
      _M_transfer_after(__bbegin, __bend);
    }

  template<typename _Alloc>
    void
    _Fwd_list_node_base<_Alloc>::
    _M_transfer_after(_Pointer __bbegin, _Pointer __bend)
    {
      _Pointer __keep = __bbegin->_M_next;
      if (__bend)
	{
	  __bbegin->_M_next = __bend->_M_next;
	  __bend->_M_next = _M_next;
	}
      else
	__bbegin->_M_next = 0;
      _M_next = __keep;
    }
 
  template<typename _Alloc>
    void
    _Fwd_list_node_base<_Alloc>::
    _M_reverse_after()
    {
      _Pointer __tail = _M_next;
      if (!__tail)
	return;
      while (_Pointer __temp = __tail->_M_next)
	{
	  _Pointer __keep = _M_next;
	  _M_next = __temp;
	  __tail->_M_next = __temp->_M_next;
	  _M_next->_M_next = __keep;
	}
    }

 /**
  *  @brief  Sort the singly linked list starting after this node.
  *          This node is assumed to be an empty head node (of type
  *          _Fwd_list_node_base).
  */
  template<typename _Tp, class _Alloc>
    template<typename _Comp>
      void
      _Fwd_list_node<_Tp, _Alloc>::
      _M_sort_after(_Comp __comp)
      {
        // If `next' is 0, return immediately.
        _Pointer __list = __static_pointer_cast<_Pointer>(this->_M_next);
        if (!__list)
          return;

        unsigned long __insize = 1;

        while (1)
          {
            _Pointer __p = __list;
            __list = 0;
            _Pointer __tail = 0;

            // Count number of merges we do in this pass.
            unsigned long __nmerges = 0;

            while (__p)
              {
                ++__nmerges;
                // There exists a merge to be done.
                // Step `insize' places along from p.
                _Pointer __q = __p;
                unsigned long __psize = 0;
                for (unsigned long __i = 0; __i < __insize; ++__i)
                  {
                    ++__psize;
                    __q = __static_pointer_cast<_Pointer>(__q->_M_next);
                    if (!__q)
                      break;
                  }

                // If q hasn't fallen off end, we have two lists to merge.
                unsigned long __qsize = __insize;

                // Now we have two lists; merge them.
                while (__psize > 0 || (__qsize > 0 && __q))
                  {
                    // Decide whether next node of merge comes from p or q.
                    _Pointer __e;
                    if (__psize == 0)
                      {
                        // p is empty; e must come from q.
                        __e = __q;
                        __q = __static_pointer_cast<_Pointer>(__q->_M_next);
                        --__qsize;
                      }
                    else if (__qsize == 0 || !__q)
                      {
                        // q is empty; e must come from p.
                        __e = __p;
                        __p = __static_pointer_cast<_Pointer>(__p->_M_next);
                        --__psize;
                      }
                    else if (__comp(__p->_M_value, __q->_M_value))
                      {
                        // First node of p is lower; e must come from p.
                        __e = __p;
                        __p = __static_pointer_cast<_Pointer>(__p->_M_next);
                        --__psize;
                      }
                    else
                      {
                        // First node of q is lower; e must come from q.
                        __e = __q;
                        __q = __static_pointer_cast<_Pointer>(__q->_M_next);
                        --__qsize;
                      }

                    // Add the next node to the merged list.
                    if (__tail)
                      __tail->_M_next = __e;
                    else
                      __list = __e;
                    __tail = __e;
                  }

                // Now p has stepped `insize' places along, and q has too.
                __p = __q;
              }
            __tail->_M_next = 0;

            // If we have done only one merge, we're finished.
            // Allow for nmerges == 0, the empty list case.
            if (__nmerges <= 1)
              {
                this->_M_next = __list;
                return;
              }

            // Otherwise repeat, merging lists twice the size.
            __insize *= 2;
          }
      }
 
  template<typename _Tp, typename _Alloc>
    _Fwd_list_base<_Tp, _Alloc>::
    _Fwd_list_base(const _Fwd_list_base& __lst, const _Alloc& __a)
    : _M_impl(__a)
    {
      this->_M_impl._M_head._M_next = 0;
      typename _Node_base::_Pointer __to = &this->_M_impl._M_head;
      typename _Node::_Pointer __curr 
        = __static_pointer_cast<typename _Node::_Pointer>
                               (__lst._M_impl._M_head._M_next);
      while (__curr)
        {
          __to->_M_next = _M_create_node(__curr->_M_value);
          __to = __to->_M_next;
          __curr = __static_pointer_cast<typename _Node::_Pointer>
                                        (__curr->_M_next);
        }
    }

  template<typename _Tp, typename _Alloc>
    template<typename... _Args>
      typename _Fwd_list_base<_Tp, _Alloc>::_Node_base::_Pointer
      _Fwd_list_base<_Tp, _Alloc>::
      _M_insert_after(const_iterator __pos, _Args&&... __args)
      {
        typename _Node_base::_Pointer __to 
          = __const_pointer_cast<typename _Node_base::_Pointer>
                                (__pos._M_node);
        typename _Node::_Pointer __thing 
          = __static_pointer_cast<typename _Node::_Pointer>( 
                _M_create_node(std::forward<_Args>(__args)...) );
        __thing->_M_next = __to->_M_next;
        __to->_M_next = __thing;
        return __static_pointer_cast<typename _Node_base::_Pointer>
                                    (__to->_M_next);
      }

  template<typename _Tp, typename _Alloc>
    typename _Fwd_list_base<_Tp, _Alloc>::_Node_base::_Pointer
    _Fwd_list_base<_Tp, _Alloc>::
    _M_erase_after(typename _Node_base::_Pointer __pos)
    {
      typename _Node::_Pointer __curr 
        = __static_pointer_cast<typename _Node::_Pointer>(__pos->_M_next);
      if (__curr)
        {
          typename _Node_base::_Pointer __next = __curr->_M_next;
          __pos->_M_next = __next;
          _M_get_Node_allocator().destroy(__curr);
          _M_put_node(__curr);
        }
      return __pos;
    }

  template<typename _Tp, typename _Alloc>
    typename _Fwd_list_base<_Tp, _Alloc>::_Node_base::_Pointer
    _Fwd_list_base<_Tp, _Alloc>::
    _M_erase_after(typename _Node_base::_Pointer __pos, 
                   typename _Node_base::_Pointer __last)
    {
      typename _Node::_Pointer __curr 
        = __static_pointer_cast<typename _Node::_Pointer>(__pos->_M_next);
      while (__curr)
        {
          typename _Node::_Pointer __temp = __curr;
          __curr = __static_pointer_cast<typename _Node::_Pointer>
                                        (__curr->_M_next);
          _M_get_Node_allocator().destroy(__temp);
          _M_put_node(__temp);
          __pos->_M_next = __curr;
          if (__temp == __last)
            break;
        }
      return __pos;
    }
  
  // Called by the range constructor to implement [23.1.1]/9
  template<typename _Tp, typename _Alloc>
    template<typename _InputIterator>
      void
      forward_list<_Tp, _Alloc>::
      _M_initialize_dispatch(_InputIterator __first, _InputIterator __last,
                             __false_type)
      {
        typename _Node_base::_Pointer __to = &this->_M_impl._M_head;
        for (; __first != __last; ++__first)
          {
            __to->_M_next = this->_M_create_node(*__first);
            __to = __to->_M_next;
          }
      }

  // Called by forward_list(n,v,a), and the range constructor
  // when it turns out to be the same thing.
  template<typename _Tp, typename _Alloc>
    void
    forward_list<_Tp, _Alloc>::
    _M_fill_initialize(size_type __n, const value_type& __value)
    {
      typename _Node_base::_Pointer __to = &this->_M_impl._M_head;
      for (; __n > 0; --__n)
        {
          __to->_M_next = this->_M_create_node(__value);
          __to = __to->_M_next;
        }
    }

  template<typename _Tp, typename _Alloc>
    forward_list<_Tp, _Alloc>&
    forward_list<_Tp, _Alloc>::
    operator=(const forward_list& __list)
    {
      if (&__list != this)
        {
          iterator __prev1 = before_begin();
          iterator __curr1 = begin();
          iterator __last1 = end();
          const_iterator __first2 = __list.cbegin();
          const_iterator __last2 = __list.cend();
          while (__curr1 != __last1 && __first2 != __last2)
            {
              *__curr1 = *__first2;
              ++__prev1;
              ++__curr1;
              ++__first2;
            }
          if (__first2 == __last2)
            erase_after(__prev1, __last1);
          else
            insert_after(__prev1, __first2, __last2);
        }
      return *this;
    }

  template<typename _Tp, typename _Alloc>
    void
    forward_list<_Tp, _Alloc>::
    resize(size_type __sz, value_type __val)
    {
      iterator __k = before_begin();

      size_type __len = 0;
      while (__k._M_next() != end() && __len < __sz)
        {
          ++__k;
          ++__len;
        }
      if (__len == __sz)
        erase_after(__k, end());
      else
        insert_after(__k, __sz - __len, __val);
    }

  template<typename _Tp, typename _Alloc>
    void
    forward_list<_Tp, _Alloc>::
    splice_after(const_iterator __pos, forward_list&& __list)
    {
      if (!__list.empty() && &__list != this)
        {
          typename _Node_base::_Pointer __tmp 
            = __const_pointer_cast<typename _Node_base::_Pointer>
                                  (__pos._M_node);
          const_iterator __before = __list.cbefore_begin();
          __tmp->_M_transfer_after(__const_pointer_cast
                                     <typename _Node_base::_Pointer>
                                     (__before._M_node));
        }
    }

  template<typename _Tp, typename _Alloc>
    void
    forward_list<_Tp, _Alloc>::
    splice_after(const_iterator __pos, forward_list&& __list,
                 const_iterator __before, const_iterator __last)
    {
      typename _Node_base::_Pointer __tmp 
        = __const_pointer_cast<typename _Node_base::_Pointer>(__pos._M_node);
      __tmp->_M_transfer_after(__const_pointer_cast
                                 <typename _Node_base::_Pointer>
                                 (__before._M_node),
                               __const_pointer_cast
                                 <typename _Node_base::_Pointer>
                                 (__last._M_node));
    }

  template<typename _Tp, typename _Alloc>
    void
    forward_list<_Tp, _Alloc>::
    remove(const _Tp& __val)
    {
      typename _Node::_Pointer __curr 
        = __static_pointer_cast<typename _Node::_Pointer>
                               (&this->_M_impl._M_head);
      while (typename _Node::_Pointer __temp = 
             __static_pointer_cast<typename _Node::_Pointer>(__curr->_M_next))
        {
          if (__temp->_M_value == __val)
            this->_M_erase_after(__curr);
          else
            __curr = __static_pointer_cast<typename _Node::_Pointer>
                                          (__curr->_M_next);
        }
    }

  template<typename _Tp, typename _Alloc>
    template<typename _Pred>
      void
      forward_list<_Tp, _Alloc>::
      remove_if(_Pred __pred)
      {
        typename _Node::_Pointer __curr 
          = __static_pointer_cast<typename _Node::_Pointer>
                                 (&this->_M_impl._M_head);
        while (typename _Node::_Pointer __temp = 
               __static_pointer_cast<typename _Node::_Pointer>(__curr->_M_next))
          {
            if (__pred(__temp->_M_value))
              this->_M_erase_after(__curr);
            else
              __curr = __static_pointer_cast<typename _Node::_Pointer>
                                            (__curr->_M_next);
          }
      }

  template<typename _Tp, typename _Alloc>
    template<typename _BinPred>
      void
      forward_list<_Tp, _Alloc>::
      unique(_BinPred __binary_pred)
      {
        iterator __first = begin();
        iterator __last = end();
        if (__first == __last)
          return;
        iterator __next = __first;
        while (++__next != __last)
        {
          if (__binary_pred(*__first, *__next))
            erase_after(__first);
          else
            __first = __next;
          __next = __first;
        }
      }

  template<typename _Tp, typename _Alloc>
    template<typename _Comp>
      void
      forward_list<_Tp, _Alloc>::
      merge(forward_list&& __list, _Comp __comp)
      {
        typename _Node_base::_Pointer __node = &this->_M_impl._M_head;
        while (__node->_M_next && __list._M_impl._M_head._M_next)
          {
            if (__comp(__static_pointer_cast<typename _Node::_Pointer>
                       (__list._M_impl._M_head._M_next)->_M_value,
                       __static_pointer_cast<typename _Node::_Pointer>
                       (__node->_M_next)->_M_value))
              __node->_M_transfer_after(&__list._M_impl._M_head,
                                        __list._M_impl._M_head._M_next);
            __node = __node->_M_next;
          }
        if (__list._M_impl._M_head._M_next)
          {
            __node->_M_next = __list._M_impl._M_head._M_next;
            __list._M_impl._M_head._M_next = 0;
          }
      }

  template<typename _Tp, typename _Alloc>
    bool
    operator==(const forward_list<_Tp, _Alloc>& __lx,
               const forward_list<_Tp, _Alloc>& __ly)
    {
      //  We don't have size() so we need to walk through both lists
      //  making sure both iterators are valid.
      auto __ix = __lx.cbegin();
      auto __iy = __ly.cbegin();
      while (__ix != __lx.cend() && __iy != __ly.cend())
        {
          if (*__ix != *__iy)
            return false;
          ++__ix;
          ++__iy;
        }
      if (__ix == __lx.cend() && __iy == __ly.cend())
        return true;
      else
        return false;
    }

_GLIBCXX_END_NAMESPACE // namespace std

#endif /* _FORWARD_LIST_TCC */

