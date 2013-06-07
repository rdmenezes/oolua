
#ifndef CLASS_FROM_STACK_H_
#	define CLASS_FROM_STACK_H_

#	include "lua_includes.h"
#	include "proxy_class.h"
#	include "oolua_userdata.h"
#	include "oolua_config.h"

#	include "base_checker.h"

#if OOLUA_DEBUG_CHECKS == 1
#	include <cassert>
#endif

struct lua_State;

namespace OOLUA
{

	/** \cond INTERNAL*/
	namespace INTERNAL
	{

		template<typename T>
		T * check_index(lua_State * l, int narg);

		template<typename T>
		T  * check_index_no_const(lua_State * l, int narg);

		bool index_is_userdata(lua_State* l,int index,Lua_ud*& ud);
		void report_error_userdata_is_constant(lua_State* const l, char const* pulling_class_type);
		
#if OOLUA_CHECK_EVERY_USERDATA_IS_CREATED_BY_OOLUA == 0
		inline bool index_is_userdata(lua_State* l,int index,Lua_ud*& ud)
		{
			ud = static_cast<Lua_ud *>( lua_touserdata(l, index) );
			return ud != 0;
		}
#endif
		template<typename T>
		inline T* class_from_stack_top(lua_State * l)
		{
			const int top = lua_gettop(l);
			return top ? check_index<T>(l,top) : (T*)0;
		}

		template<typename T>
		inline T* none_const_class_from_stack_top(lua_State * l)
		{
			const int top = lua_gettop(l);
			return top ? check_index_no_const<T>(l,top) : (T*)0;
		}

		template<typename T>
		inline T* class_from_index(lua_State * l,int index)
		{
#if OOLUA_DEBUG_CHECKS == 1
			assert(index >0);
#endif
			return check_index<T>(l,index);
		}

		template<typename T>
		inline T* none_const_class_from_index(lua_State * l,int index)
		{
#if OOLUA_DEBUG_CHECKS == 1
			assert(index >0);
#endif
			return check_index_no_const<T>(l,index);
		}


		template<typename T>
		T* valid_base_ptr_or_null(lua_State* l,Lua_ud const* stack_ud,int userdata_index)
		{
			Lua_ud requested_ud = {0,&stack_top_type_is_base<T>,&OOLUA::register_class<T>,0};
			stack_ud->base_checker(l,&requested_ud,userdata_index);
			return static_cast<T* >(requested_ud.void_class_ptr);
		}


		template<typename T>
		T* check_index(lua_State *  l, int narg)
		{
			Lua_ud * ud;
			if( !index_is_userdata(l,narg,ud))return 0;
			if( ! ud_is_type<T>(ud) )
			{
				return valid_base_ptr_or_null<T>(l,ud,narg);
			}
			return static_cast<T* >(ud->void_class_ptr);
		}


		template<typename T>
		T* check_index_no_const(lua_State * l, int narg)
		{
			Lua_ud * ud;
			if( !index_is_userdata(l,narg,ud))return 0;
			if( userdata_is_constant(ud) )
			{
				report_error_userdata_is_constant(l,OOLUA::Proxy_class<T>::class_name);
				//does not return
			}
			if( ! ud_is_type<T>(ud) )
			{
				return valid_base_ptr_or_null<T>(l,ud,narg);
			}
			return static_cast<T* >(ud->void_class_ptr);
		}
		
	}
	/** \endcond */

}
#endif

