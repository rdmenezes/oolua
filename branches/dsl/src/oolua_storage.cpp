
#include "lua_includes.h"
#include "oolua_userdata.h"
#include "oolua_storage.h"
#include "oolua_push_pull.h"


#if OOLUA_DEBUG_CHECKS == 1
#	include <cassert>
#endif

namespace OOLUA
{

	/** \cond INTERNAL*/
	namespace INTERNAL
	{

		//pushes the weak table on top and returns its absolute index
		//The weak table is a table in the Lua registry specific to OOLua,
		//which has void pointer keys and values of userdata pointers.
		int push_weak_table(lua_State* l)
		{
			Weak_table::getWeakTable(l);
			return lua_gettop(l);
		}

		//if found it is left on the top of the stack and returns true
		//else the stack is same as on entrance to the function and false returned
		bool is_there_an_entry_for_this_void_pointer(lua_State* l, void* ptr)
		{
			int wt = push_weak_table(l);
			bool result = is_there_an_entry_for_this_void_pointer(l, ptr, wt);
			lua_remove(l, wt);
			return result;
		}

		bool is_there_an_entry_for_this_void_pointer(lua_State* l, void* ptr, int tableIndex)
		{
			lua_pushlightuserdata(l, ptr);//weakTable ... ptr
			lua_rawget(l, tableIndex);//weakTable .... (full user data or nil)
			if( !lua_isnil(l, -1) )
			{
				return true;//leave ud on top
			}
			lua_pop(l, 1);//pop nil
			return false;
		}

		//on entering user data and weaktable are on the stack
		void add_ptr_if_required(lua_State* const l, void* ptr, int udIndex, int weakIndex)
		{
			lua_pushlightuserdata(l, ptr);//ptr
			lua_rawget(l, weakIndex);//(null or ptr)
			if( lua_isnil(l, -1) == 0 )
			{
				lua_pop(l, 1);//pop the ud
				return;
			}
			lua_pop(l, 1);//pop the null

			lua_pushlightuserdata(l, ptr);//key
			lua_pushvalue(l, udIndex);//key ud
			lua_rawset(l, weakIndex);//table[key]=value
		}

		bool ud_at_index_is_const(lua_State* l, int index)
		{
			return INTERNAL::userdata_is_constant( static_cast<Lua_ud *>( lua_touserdata(l, index) ) );
		}

		Lua_ud* new_userdata(lua_State* l, void* ptr, bool is_const, oolua_function_check_base base_checker, oolua_type_check_function type_check)
		{
			Lua_ud* ud = static_cast<Lua_ud*>(lua_newuserdata(l, sizeof(Lua_ud)));
			ud->flags = 0;
			reset_userdata(ud, ptr, is_const, base_checker, type_check);
#if OOLUA_CHECK_EVERY_USERDATA_IS_CREATED_BY_OOLUA == 1 && OOLUA_USERDATA_OPTIMISATION == 1
			OOLUA_SET_COOKIE(ud->flags);
#endif
			return ud;
		}

		void reset_userdata(Lua_ud* ud, void* ptr, bool is_const, oolua_function_check_base base_checker, oolua_type_check_function type_check)
		{
			ud->void_class_ptr = ptr;
			ud->base_checker = base_checker;
			ud->type_check = type_check;
			userdata_const_value(ud, is_const);
		}

	} // namespace INTERNAL //NOLINT
	/**\endcond*/
} // namespace OOLUA