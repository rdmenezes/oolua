#include "expose_hierarchy.h"

//this part using va args is not yet fixed
#if OOLUA_TEST_VA_ARGS == 1
#	undef OOLUA_TEST_VA_ARGS
#	define OOLUA_TEST_VA_ARGS  0
#endif

#if OOLUA_TEST_VA_ARGS == 1
TEST_EXPORT_OOLUA_FUNCTIONS_NON_CONST(Abstract1,func1,virtualVoidParam3Int)
#else
EXPORT_OOLUA_FUNCTIONS_2_NON_CONST(Abstract1,func1,virtualVoidParam3Int)
#endif
EXPORT_OOLUA_FUNCTIONS_0_CONST(Abstract1)


#if OOLUA_TEST_VA_ARGS == 1
TEST_EXPORT_OOLUA_FUNCTIONS_NON_CONST(Abstract2,func2_1)
#else
EXPORT_OOLUA_FUNCTIONS_1_NON_CONST(Abstract2,func2_1)
#endif
EXPORT_OOLUA_FUNCTIONS_0_CONST(Abstract2)

#if OOLUA_TEST_VA_ARGS == 1
TEST_EXPORT_OOLUA_FUNCTIONS_NON_CONST(Abstract3,func3_1)
#else
EXPORT_OOLUA_FUNCTIONS_1_NON_CONST(Abstract3,func3_1)
#endif
EXPORT_OOLUA_FUNCTIONS_0_CONST(Abstract3)

EXPORT_OOLUA_NO_FUNCTIONS(Derived1Abstract1)

EXPORT_OOLUA_NO_FUNCTIONS(TwoAbstractBases)

EXPORT_OOLUA_NO_FUNCTIONS(DerivedFromTwoAbstractBasesAndAbstract3)