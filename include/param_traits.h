///////////////////////////////////////////////////////////////////////////////
///  @file param_traits.h
///  Parameter traits for OOLUA::Proxy_class functions.
///  These traits determine wether the parameters need to be passed back to Lua
///  or not
///  @remarks
///  If parameter traits are not defined then it defaults to OOLUA::in_p.
///  For function return values OOLUA::function_return is used (inside the generating
///  function), this template is specialised for void types.
///  @author Liam Devine
///  \copyright
///  See licence.txt for more details.
///////////////////////////////////////////////////////////////////////////////
#ifndef PARAM_TRAITS_H_
#	define PARAM_TRAITS_H_

#	include "lvd_types.h"
#	include "determin_qualifier.h"
#	include "oolua_config.h"

#if OOLUA_STD_STRING_IS_INTEGRAL == 1
#	include <string>
#endif

struct lua_State;
namespace OOLUA
{

/** \addtogroup OOLuaTraits Traits
@{
\brief Provides direction and ownership information.
\details
In C++ the use of traits which have a lua_ or cpp_ prefix can control ownership for
parameters, returns values and stack entries whilst traits which specify direction
for parameters contain as part of their name "out", "in" or a combination.
*/

	/** \struct in_p
		\brief Input parameter trait
		\details
		The calling Lua procedure supplies the parameter to the proxied function.
		No change of ownership occurs.
		\note
		This is the default trait used for function parameters when no trait
		is supplied.
	 */
	template<typename T>struct in_p;

	/** \struct out_p
		\brief Output parameter trait
		\details
		The calling Lua procedure does not pass the parameter to the proxied
		function, instead one is created using the default constructor and
		passed to the proxied function. The result after the proxied call with
		be returned to the calling procedure. If this is a type which has a
		proxy then it will cause a heap allocation of the type, which Lua will
		own.
	*/
	template<typename T>struct out_p;

	/** \struct in_out_p
		\brief Input and output parameter trait
		\details
		The calling Lua procedure supplies the parameter to the proxied
		function, the value of the parameter after the proxied call will be
		passed back to the calling procedure as a return value.
		No change of ownership occurs.
	*/
	template<typename T>struct in_out_p;

	/** \struct cpp_in_p
		\brief Input parameter trait which will be owned by C++
		\details Parameter supplied via Lua changes ownership to C++.
	*/
	template<typename T>struct cpp_in_p;


	/** \struct lua_out_p
		\brief Output parameter trait which will be owned by Lua
		\details
		Lua code does not pass an instance to the C++ function, yet the pushed back
		value after the function call will be owned by Lua. This is meaningful only
		if called with a type which has a proxy and it is by reference, otherwise
		undefined.
	*/
	template<typename T>struct lua_out_p;

	/** \struct lua_return
		\breif Return trait for a type which will be owned by Lua
		\details
		The type returned from the function is a heap allocated instance whose
		ownership will be controlled by Lua. This is only valid for function
		return types.
	*/
	template<typename T>struct lua_return;

	/** \struct maybe_null
		\breif Return trait for a pointer which at runtime maybe NULL.
		\details
		The type returned from the function is a pointer instance whose
		runtime value maybe NULL. If it is NULL then lua_pushnil will be called
		else the pointer will be pushed as normal. No change of ownership will
		occur for the type. This is only valid for function return types.
	*/
	template<typename T>struct maybe_null;

	/** \struct cpp_acquire_ptr
		\brief Change of ownership to C++
		\details
		Informs the binding that C++ will take control of the pointer being used
		and call delete on it when appropriate. This is only valid for
		OOLUA::pull calls.
	*/
	template<typename T>struct cpp_acquire_ptr;

	/** \struct lua_acquire_ptr
		\brief Change of ownership to Lua
		\details
		Informs the binding that Lua will take control of the pointer being used
		and call delete on it when appropriate. This is only valid for
		OOLUA::push calls
	*/
	template<typename T>struct lua_acquire_ptr;

	/** \struct OOLUA::calling_lua_state
		\brief Informs that the calling state is a parameter for a function
	*/
	struct calling_lua_state;

	/** \enum Owner
		Which language owns the parameter Lua, Cpp or no change to ownership
	*/
	enum Owner
	{	No_change	/*!< No change of ownership*/
		, Cpp		/*!< Change in ownership, C++ will now own the instance*/
		, Lua		/*!< Change in ownership, Lua will now own the instance*/
	};

/**}@*/


	/** \cond INTERNAL*/
	template<typename T>
	class Proxy_class;

	class Table;
	template<int ID>struct Lua_ref;

	namespace INTERNAL
	{

		template<typename Type>
		struct has_a_proxy_type
		{
			typedef OOLUA::Proxy_class<Type> proxy_type;
			template <typename U>
			static char (& has_none_proxy_typedef(typename U::OoluaNoneProxy*))[1];

			template <typename U>
			static char (& has_none_proxy_typedef(...))[2];

			enum {value = sizeof(has_none_proxy_typedef<proxy_type>(0)) == 1 ? 0 : 1};
		};

		template <typename From>
		class can_convert_to_int
		{
			typedef char (&yes)[1];
			typedef char (&no)[2];
			struct Convertor
			{
				template <typename T>
				Convertor(T&);
			};
			static no test(Convertor);
			static no test(float&);
			static no test(double&);
			static yes test(int); // NOLINT
			static From& make_from();
		public:
			enum { value = sizeof test(make_from()) == sizeof(yes) ? 1 : 0 };
		};

		template<typename T>
		struct Raw_type
		{
			typedef typename LVD::remove_ref<T>::type no_ref;
			typedef typename LVD::remove_all_const<no_ref>::type no_ref_or_const;
			typedef typename LVD::remove_all_ptrs<no_ref_or_const>::type raw;

			typedef raw type;
		};

		template<typename T>
		struct Type_enum_defaults
		{
			enum { is_by_value = !LVD::by_reference<typename LVD::remove_all_const<T>::type >::value };
			enum { is_constant =  LVD::is_const<T>::value };
			enum
			{
				is_integral = LVD::is_integral_type< typename Raw_type<T>::raw >::value
					|| /*enum*/(!has_a_proxy_type<typename Raw_type<T>::raw>::value
								&& can_convert_to_int<typename Raw_type<T>::raw>::value)
			};
		};


		template<typename T>
		struct is_param_by_value
		{
			enum { value = !LVD::by_reference<T>::value };
		};

		template<typename T>
		struct strip_ref_n_const
		{
			typedef typename LVD::remove_all_const<
				typename LVD::remove_ref<T>
				::type>
				::type type;
		};


		template<typename T>
		struct no_pointers
		{
			typedef typename LVD::remove_all_ptrs<T>::type type;
		};

		template<typename T, int is_integral>struct Pull_type;

		template<typename T>
		struct Pull_type<T, 0>
		{
			typedef T* type;
		};

		template<typename T>
		struct Pull_type<T, 1>
		{
			typedef T type;
		};

		template<typename T, typename Original_Type, int is_integral>struct Pull_type_;

		template<typename Original_Type, typename T>
		struct pull_type_maybe_const
		{
			typedef typename LVD::if_else<
											LVD::is_const<Original_Type>::value
											|| INTERNAL::Type_enum_defaults<Original_Type>::is_by_value
											, T const*
											, T*
										>::type type;
		};

		/*specialisation to prevent Visual Studio warnings, that const applied to a function is ignored*/
		template<typename Original_Type>
		struct pull_type_maybe_const<Original_Type, int (lua_State*)> // NOLINT
		{
			typedef int(*type)(lua_State*); // NOLINT
		};

		/* should rename Pull_maybe_const */
		template<typename T, typename Original_Type>
		struct Pull_type_<T, Original_Type, 0>
		{
			typedef typename pull_type_maybe_const<Original_Type, T>::type type;
		};

		template<typename T, typename Original_Type>
		struct Pull_type_<T, Original_Type, 1>
		{
			typedef T type;
		};

	} // namespace INTERNAL // NOLINT


	template<typename T>
	struct in_p
	{
		typedef T type;
		typedef typename INTERNAL::Raw_type<T>::type raw;
		typedef typename INTERNAL::Pull_type_<raw, T, INTERNAL::Type_enum_defaults<type>::is_integral>::type pull_type;
		enum {in = 1};
		enum {out = 0};
		enum {owner = No_change};
		enum { is_by_value = INTERNAL::Type_enum_defaults<type>::is_by_value  };
		enum { is_constant = INTERNAL::Type_enum_defaults<type>::is_constant  };
		enum { is_integral = INTERNAL::Type_enum_defaults<type>::is_integral  };
	};

	template<typename T>
	struct in_out_p
	{
		typedef T type;
		typedef typename INTERNAL::Raw_type<T>::type raw;
		typedef typename INTERNAL::Pull_type<raw, LVD::is_integral_type<raw>::value >::type pull_type;
		enum { in = 1};
		enum { out = 1};
		enum { owner = No_change};
		enum { is_by_value = INTERNAL::Type_enum_defaults<type>::is_by_value  };
		enum { is_constant = INTERNAL::Type_enum_defaults<type>::is_constant  };
		enum { is_integral = INTERNAL::Type_enum_defaults<type>::is_integral  };
		typedef char type_has_to_be_by_reference [is_by_value ? -1 : 1 ];
	};


	template<>
	struct out_p<void>;

	template<typename T>
	struct out_p
	{
		typedef T type;
		typedef typename INTERNAL::Raw_type<T>::type raw;
		typedef raw pull_type;
		enum { in = 0};
		enum { out = 1};
		enum { owner = INTERNAL::Type_enum_defaults<raw>::is_integral? No_change : Lua};
		enum { is_by_value = 1};//yes OOLua creates on the stack
		enum { is_constant = INTERNAL::Type_enum_defaults<type>::is_constant  };
		enum { is_integral = INTERNAL::Type_enum_defaults<type>::is_integral  };
		typedef char type_has_to_be_by_reference [INTERNAL::Type_enum_defaults<type>::is_by_value ? -1 : 1 ];
	};


	//cpp takes ownership
	template<typename T>
	struct cpp_in_p
	{
		typedef T type;
		typedef typename INTERNAL::Raw_type<T>::type raw;
		typedef typename INTERNAL::Pull_type<raw, LVD::is_integral_type<raw>::value >::type pull_type;
		enum { in = 1};
		enum { out = 0};
		enum { owner = Cpp};
		enum { is_by_value = INTERNAL::Type_enum_defaults<type>::is_by_value  };
		enum { is_constant = INTERNAL::Type_enum_defaults<type>::is_constant  };
		enum { is_integral = INTERNAL::Type_enum_defaults<type>::is_integral  };
		typedef char type_has_to_be_by_reference [is_by_value ? -1 : 1 ];
		typedef char type_can_not_be_integral [is_integral ? -1 : 1 ];
		typedef char type_can_not_be_just_a_reference_to_type [	LVD::is_same<raw&, type>::value ? -1 : 1];
		typedef char type_can_not_be_just_a_reference_to_const_type [	LVD::is_same<raw const&, type>::value ? -1 : 1];
	};

	template<typename T>
	struct ref_to_const_ptr
	{
		enum{value = 0};
	};
	template<typename T>
	struct ref_to_const_ptr<T*const&>
	{
		enum{value = 1};
	};
	template<typename T>
	struct ref_to_ptr_const
	{
		enum{value = 0};
	};
	template<typename T>
	struct ref_to_ptr_const<T const*&>
	{
		enum{value = 1};
	};

	template<typename T>
	struct lua_out_p
	{
		typedef T type;
		typedef typename INTERNAL::Raw_type<T>::type raw;
		typedef typename INTERNAL::Pull_type_<raw, T, LVD::is_integral_type<raw>::value >::type pull_type;
		enum { in = 0};
		enum { out = 1};
		enum { owner = Lua};
		enum { is_by_value = INTERNAL::Type_enum_defaults<type>::is_by_value  };
		enum { is_constant = INTERNAL::Type_enum_defaults<type>::is_constant  };
		enum { is_integral = INTERNAL::Type_enum_defaults<type>::is_integral  };
		typedef char type_has_to_be_by_reference [is_by_value ? -1 : 1 ];
		typedef char type_can_not_be_integral [is_integral ? -1 : 1 ];
		typedef char must_be_assignable [ (is_constant && !ref_to_ptr_const<type>::value)
										 || (ref_to_const_ptr<type>::value)
											? -1 : 1];
		typedef char type_can_not_be_just_a_reference_to_type [	LVD::is_same<raw&, type>::value ? -1 : 1];
		typedef char type_can_not_be_just_a_pointer_to_type [LVD::is_same<raw*, type>::value ? -1 : 1];
	};

	template<typename T>
	struct lua_return
	{
		typedef T type;
		typedef typename INTERNAL::Raw_type<T>::type raw;
		typedef typename INTERNAL::Pull_type_<raw, T, LVD::is_integral_type<raw>::value >::type pull_type;
		enum { in = 0};
		enum { out = 1};
		enum { owner = Lua};
		enum { is_by_value = INTERNAL::Type_enum_defaults<type>::is_by_value  };
		enum { is_constant = INTERNAL::Type_enum_defaults<type>::is_constant  };
		enum { is_integral = INTERNAL::Type_enum_defaults<type>::is_integral  };
		typedef char type_has_to_be_by_reference [is_by_value ? -1 : 1 ];
		typedef char type_can_not_be_integral [is_integral ? -1 : 1 ];
		typedef char type_can_not_be_just_a_reference_to_type [	LVD::is_same<raw&, type>::value ? -1 : 1];
		typedef char type_can_not_be_just_a_const_reference_to_type [ LVD::is_same<raw const&, type>::value ? -1 : 1];
	};

	template<typename T>
	struct maybe_null
	{
		typedef T type;
		typedef typename INTERNAL::Raw_type<T>::type raw;
		typedef typename INTERNAL::Pull_type_<raw, T, LVD::is_integral_type<raw>::value >::type pull_type;
		enum { in = 0};
		enum { out = 1};
		enum { owner = No_change};
		enum { is_by_value = INTERNAL::Type_enum_defaults<type>::is_by_value  };
		enum { is_constant = INTERNAL::Type_enum_defaults<type>::is_constant  };
		enum { is_integral = INTERNAL::Type_enum_defaults<type>::is_integral  };
		typedef char type_can_not_be_integral [is_integral ? -1 : 1 ];
		typedef char type_has_to_be_by_reference [is_by_value ? -1 : 1 ];
		typedef char type_can_not_be_just_a_reference_to_type [	LVD::is_same<raw&, type>::value ? -1 : 1];
		typedef char type_can_not_be_just_a_const_reference_to_type [ LVD::is_same<raw const&, type>::value ? -1 : 1];
		/*Reference to pointer:
		this could be valid in some situations, until such a time as it is required
		or requested disable it*/
		typedef char type_can_not_be_a_reference_to_ptr [ LVD::is_same<raw *&, type>::value ? -1 : 1];
	};

	template<typename T>
	struct cpp_acquire_ptr
	{
		typedef T type;
		typedef typename INTERNAL::Raw_type<T>::type raw;
		typedef typename INTERNAL::Pull_type_<raw, T, LVD::is_integral_type<raw>::value>::type pull_type;
		enum { in = 1 };
		enum { out = 0 };
		enum { owner = Cpp };
		enum { is_by_value = INTERNAL::Type_enum_defaults<type>::is_by_value  };
		enum { is_constant = INTERNAL::Type_enum_defaults<type>::is_constant  };
		enum { is_integral = INTERNAL::Type_enum_defaults<type>::is_integral  };

		cpp_acquire_ptr():m_ptr(0){}

		raw* m_ptr;
		typedef char type_can_not_be_integral [is_integral ? -1 : 1 ];
		typedef char type_has_to_be_by_reference [is_by_value ? -1 : 1 ];
		typedef char type_can_not_be_just_a_reference_to_type [	LVD::is_same<raw&, type>::value ? -1 : 1];
	};


	template<typename T>
	struct lua_acquire_ptr
	{
		typedef T type;
		typedef typename INTERNAL::Raw_type<T>::type raw;
		typedef typename INTERNAL::Pull_type_<raw, T, LVD::is_integral_type<raw>::value >::type pull_type;
		enum { in = 0};
		enum { out = 1};
		enum { owner = Lua};
		enum { is_by_value = INTERNAL::Type_enum_defaults<type>::is_by_value  };
		enum { is_constant = INTERNAL::Type_enum_defaults<type>::is_constant  };
		enum { is_integral = INTERNAL::Type_enum_defaults<type>::is_integral  };

		lua_acquire_ptr(pull_type ptr):m_ptr(ptr){}
		lua_acquire_ptr():m_ptr(0){}
		pull_type m_ptr;
		typedef char type_can_not_be_integral [is_integral ? -1 : 1 ];
		typedef char type_has_to_be_by_reference [is_by_value ? -1 : 1 ];
		typedef char type_can_not_be_just_a_reference_to_type [	LVD::is_same<raw&, type>::value ? -1 : 1];
	};

	namespace INTERNAL
	{

		///////////////////////////////////////////////////////////////////////////////
		///  @struct lua_return_count
		///  Adds together the out values of the traits in the typelist
		///  which is an enum in the type
		///////////////////////////////////////////////////////////////////////////////
		template<typename T>struct lua_return_count;

		template <>
		struct lua_return_count<TYPE::Null_type>
		{
			enum { out = 0 };
		};

		template<typename T, typename T1>
		struct lua_return_count< Type_node<T, T1> >
		{
			enum { out = T::out + lua_return_count<T1>::out };
		};

		///////////////////////////////////////////////////////////////////////////////
		///  @struct has_param_traits
		///  Checks if the type is a trait type or not.
		///////////////////////////////////////////////////////////////////////////////
		template<typename T>struct has_param_traits;

		template<typename T>
		struct has_param_traits
		{
			enum {value = 0};
		};

		template<typename T>
		struct has_param_traits< in_p<T> >
		{
			enum {value = 1};
		};

		template<typename T>
		struct has_param_traits< out_p<T> >
		{
			enum {value = 1};
		};

		template<typename T>
		struct has_param_traits< in_out_p<T> >
		{
			enum {value = 1};
		};
		template<typename T>
		struct has_param_traits< lua_out_p<T> >
		{
			enum {value = 1};
		};
		template<typename T>
		struct has_param_traits< cpp_in_p<T> >
		{
			enum {value = 1};
		};

		///////////////////////////////////////////////////////////////////////////////
		///  @struct param_type_typedef
		///  Typedefs the raw underlying type weather it is already a raw type or
		///  a type wrapped in a trait.
		///////////////////////////////////////////////////////////////////////////////
		template<typename T, bool True>
		struct param_type_typedef
		{
			typedef typename T::type type;
			typedef typename T::pull_type pull_type;
			typedef typename T::raw raw;
			enum { in = T::in};
			enum { out = T::out};
			enum { owner = T::owner};
			enum { is_by_value = T::is_by_value };
			enum { is_constant = T::is_constant };
			enum { is_integral = T::is_integral };
		};

		template<typename T>
		struct param_type_typedef<T, false>
		{
			typedef in_p<T> in_param;
			typedef typename in_param::type type;
			typedef typename in_param::pull_type pull_type;
			typedef typename in_param::raw raw;
			enum { in = 1};
			enum { out = 0};
			enum { owner = No_change};
			enum { is_by_value = in_param::is_by_value };
			enum { is_constant = in_param::is_constant };
			enum { is_integral = in_param::is_integral };
		};

		///////////////////////////////////////////////////////////////////////////////
		///  @struct param_type
		///  Uses param_type_typedef and has_param_traits to define the type and its
		///  in_p in_out_p or out_p traits.
		///////////////////////////////////////////////////////////////////////////////
		template<typename T>
		struct param_type
		{
			typedef param_type_typedef<T, has_param_traits<T>::value> p_type;
			typedef typename p_type::type  type;
			typedef typename p_type::pull_type pull_type;
			typedef typename p_type::raw raw;

			enum { in = p_type::in };
			enum { out = p_type::out };
			enum { owner = p_type::owner };
			enum { is_by_value = p_type::is_by_value };
			enum { is_constant = p_type::is_constant };
			enum { is_integral = p_type::is_integral };
		};

		template<typename T>
		struct function_return
		{
			typedef T type;//real type
			typedef typename Raw_type<T>::type raw;//all modifiers removed
			typedef typename Pull_type<raw, LVD::is_integral_type<raw>::value >::type pull_type;
			enum { in = 0};
			enum { out = 1};
			enum { owner = No_change};
			enum { is_by_value = Type_enum_defaults<type>::is_by_value  };
			enum { is_constant = Type_enum_defaults<type>::is_constant  };
			enum { is_integral = Type_enum_defaults<type>::is_integral  };
		};

		template<>
		struct function_return<void>
		{
			typedef void type;
			typedef void pull_type;
			typedef void raw;
			enum { in = 0};
			enum { out = 0};
			enum { owner = No_change};
			enum { is_by_value = 1 };
			enum { is_constant = 0 };
			enum { is_integral = 1 };
		};





		template<typename T, bool True>
		struct return_type_typedef
		{
			typedef typename T::type type;
			typedef typename T::pull_type pull_type;
			typedef typename T::raw raw;
			enum { in = T::in};
			enum { out = T::out};
			enum { owner = T::owner};
			enum { is_by_value = T::is_by_value };
			enum { is_constant = T::is_constant };
			enum { is_integral = T::is_integral };
		};

		//uses default traits
		template<typename T>
		struct return_type_typedef<T, false>
		{
			typedef function_return<T> out_param;
			typedef typename out_param::type type;
			typedef typename out_param::pull_type pull_type;
			typedef typename out_param::raw raw;
			enum { in = out_param::in};
			enum { out = out_param::out};
			enum { owner = No_change};
			enum { is_by_value = out_param::is_by_value };
			enum { is_constant = out_param::is_constant };
			enum { is_integral = out_param::is_integral };
		};


		template<typename T>
		struct has_return_traits
		{
			enum{value = 0};
		};

		template<typename T>
		struct has_return_traits< lua_return<T> >
		{
			enum {value = 1};
		};

		template<typename T>
		struct return_type_traits
		{
			typedef return_type_typedef<T, has_return_traits<T>::value> r_type;
			typedef typename r_type::type  type;
			typedef typename r_type::pull_type pull_type;
			typedef typename r_type::raw raw;

			enum { in = r_type::in };
			enum { out = r_type::out };
			enum { owner = r_type::owner };
			enum { is_by_value = r_type::is_by_value  };
			enum { is_constant = r_type::is_constant  };
			enum { is_integral = r_type::is_integral  };
		};

	} // namespace INTERNAL // NOLINT

	///////////////////////////////////////////////////////////////////////////////
	///  Specialisation for the calling lua_State
	///////////////////////////////////////////////////////////////////////////////

	//so this must be a coroutine on the stack
	template struct in_p<lua_State*>;//disable it

	struct calling_lua_state {};

	template<>
	struct in_p<calling_lua_state>
	{
		typedef lua_State* type;
		typedef lua_State* raw;
		typedef lua_State* pull_type;
		enum {in = 1};
		enum {out = 0};
		enum {owner = No_change};
		enum { is_by_value = 0 };
		enum { is_constant = 0 };
		enum { is_integral = 1 };
	};

	///////////////////////////////////////////////////////////////////////////////
	///  Specialisation for C style strings
	///////////////////////////////////////////////////////////////////////////////
	template<>
	struct in_p<char*>
	{
		typedef char* type;
		typedef char* raw;//we are an integral types but very different from the char type
		typedef char const* pull_type;//we pull as the correct type, as cast will occur
		enum { in = 1 };
		enum { out = 0 };
		enum { owner = No_change };
		enum { is_by_value = 0 };
		enum { is_constant = 0 };
		enum { is_integral = 1 };
	};

	template<>
	struct in_p<char*&>;//disabled

	template<>
	struct in_p<char const*>
	{
		typedef char const* type;
		typedef char* raw;//integral yet very different to char
		typedef char const* pull_type;
		enum { in = 1 };
		enum { out = 0 };
		enum { owner = No_change };
		enum { is_by_value = 0 };
		enum { is_constant = 1 };
		enum { is_integral = 1 };
	};

	template<>
	struct in_p<char const*&>
	{
		typedef char const*& type;
		typedef char* raw;//integral yet very different to char
		typedef char const* pull_type;
		enum { in = 1 };
		enum { out = 0 };
		enum { owner = No_change };
		enum { is_by_value = 0 };
		enum { is_constant = 1 };
		enum { is_integral = 1 };
	};



	/*
	 Integral specialisation helper macros
	*/
#define oolua_end_integral_trait(trait_is_in, trait_is_out) \
		enum { in = trait_is_in}; \
		enum { out = trait_is_out}; \
		enum { owner = No_change}; \
		enum { is_by_value = INTERNAL::Type_enum_defaults<type>::is_by_value }; \
		enum { is_constant = INTERNAL::Type_enum_defaults<type>::is_constant }; \
		enum { is_integral = 1 };

#define oolua_integral_ref_trait(specialisation, pulling_type, trait_name, trait_is_in, trait_is_out) \
	template<int ID > \
	struct trait_name< specialisation > \
	{ \
		typedef specialisation type; \
		typedef pulling_type pull_type; \
		typedef typename INTERNAL::Raw_type<specialisation>::type raw; \
		oolua_end_integral_trait(trait_is_in, trait_is_out) \
	};

#define oolua_integral_trait(specialisation, pulling_type, trait_name, trait_is_in, trait_is_out) \
	template< > \
	struct trait_name<specialisation > \
	{ \
		typedef specialisation type; \
		typedef pulling_type pull_type; \
		typedef INTERNAL::Raw_type<specialisation>::type raw; \
		oolua_end_integral_trait(trait_is_in, trait_is_out) \
	};

	///////////////////////////////////////////////////////////////////////////////
	///  Specialisation for registry references
	///////////////////////////////////////////////////////////////////////////////

	oolua_integral_ref_trait(Lua_ref<ID>, Lua_ref<ID>, in_p, 1, 0)
	oolua_integral_ref_trait(Lua_ref<ID>&, Lua_ref<ID>, in_p, 1, 0)
	oolua_integral_ref_trait(Lua_ref<ID> const&, Lua_ref<ID>, in_p, 1, 0)

	oolua_integral_ref_trait(Lua_ref<ID>&, Lua_ref<ID>, out_p, 0, 1)
	oolua_integral_ref_trait(Lua_ref<ID>&, Lua_ref<ID>, in_out_p, 1, 1)

	oolua_integral_trait(Table, Table, in_p, 1, 0)

	oolua_integral_trait(Table&, Table, in_p, 1, 0)
	oolua_integral_trait(Table const&, Table, in_p, 1, 0)
	oolua_integral_trait(Table&, Table, out_p, 0, 1)
	oolua_integral_trait(Table&, Table, in_out_p, 1, 1)

	namespace INTERNAL
	{
		oolua_integral_ref_trait(Lua_ref<ID>, Lua_ref<ID> , function_return, 0, 1)
		oolua_integral_ref_trait(Lua_ref<ID>&, Lua_ref<ID>, function_return, 0, 1)
		oolua_integral_ref_trait(Lua_ref<ID>const&, Lua_ref<ID>, function_return, 0, 1)

		oolua_integral_trait(Table, Table, function_return, 0, 1)
		oolua_integral_trait(Table&, Table, function_return, 0, 1)
		oolua_integral_trait(Table const&, Table, function_return, 0, 1)
	} // namespace INTERNAL

#undef oolua_integral_ref_trait
} // namespace OOLUA


#if OOLUA_STD_STRING_IS_INTEGRAL == 1
/**[StdStringIntegralTraits]*/
namespace OOLUA
{
	oolua_integral_trait(std::string, std::string, in_p, 1, 0)
	oolua_integral_trait(std::string&, std::string, in_p, 1 , 0)
	oolua_integral_trait(std::string const, std::string, in_p, 1, 0)
	oolua_integral_trait(std::string const&, std::string, in_p, 1, 0)

	template<>struct out_p<std::string>;//disable
	oolua_integral_trait(std::string &, std::string, out_p, 0, 1)
	oolua_integral_trait(std::string &, std::string, in_out_p, 1, 1)
	namespace INTERNAL
	{
		oolua_integral_trait(std::string, std::string, function_return, 0, 1)
		oolua_integral_trait(std::string&, std::string, function_return, 0, 1)
		oolua_integral_trait(std::string const, std::string, function_return, 0, 1)
		oolua_integral_trait(std::string const&, std::string, function_return, 0, 1)
	} // namespace INTERNAL
} // namespace OOLUA
/**[StdStringIntegralTraits]*/
#endif



namespace OOLUA
{
	namespace INTERNAL
	{

		enum LUA
		{
			BOOLEAN = 1,

			NUMBER = 3,
			STRING = 4,
			TABLE = 5,
			FUNCTION = 6
		};
		/*
		 #define LUA_TNIL		0
		 #define LUA_TBOOLEAN		1
		 #define LUA_TLIGHTUSERDATA	2
		 #define LUA_TNUMBER		3
		 #define LUA_TSTRING		4
		 #define LUA_TTABLE		5
		 #define LUA_TFUNCTION		6
		 #define LUA_TUSERDATA		7
		 #define LUA_TTHREAD		8
		 */

		//Used for constructors to check parameters on the stack
		template<typename Cpp_type, int Lua_type>
		struct lua_type_is_cpp_type;

		template<typename Cpp_type>
		struct lua_type_is_cpp_type<Cpp_type, NUMBER>
		{
			enum {value = Type_enum_defaults<Cpp_type>::is_integral && !LVD::is_same<bool, Cpp_type>::value };
		};

		/**[StdStringIntegralConstructor]*/
		template<typename Cpp_type>
		struct lua_type_is_cpp_type<Cpp_type, STRING>
		{
			typedef Type_list<
			char*
#if OOLUA_STD_STRING_IS_INTEGRAL == 1
			, std::string
#endif
			/*Your string type here*/
			>::type Lua_string;
			enum {value = TYPELIST::IndexOf<Lua_string, Cpp_type>::value == -1 ? 0 : 1};
		};
		/**[StdStringIntegralConstructor]*/

		template<typename Cpp_type>
		struct lua_type_is_cpp_type<Cpp_type, BOOLEAN>
		{
			enum {value = LVD::is_same<bool, Cpp_type>::value};
		};
		template<typename Cpp_type>
		struct lua_type_is_cpp_type<Cpp_type, FUNCTION>
		{
			enum {value = LVD::is_same<Lua_ref<FUNCTION>, Cpp_type>::value};
		};
		template<typename Cpp_type>
		struct lua_type_is_cpp_type<Cpp_type, TABLE>
		{
			typedef Type_list<
			Lua_ref<TABLE>, Table
			>::type Table_types;
			enum {value = TYPELIST::IndexOf<Table_types, Cpp_type>::value == -1 ? 0 : 1};
		};
	} // namespace INTERNAL // NOLINT
	/**\endcond*/
} // namespace OOLUA

#endif //PARAM_TRAITS_H_
