
//-------------------------------------------------------------------------------------------------
///
/// @file 
///
/// @brief 
///
/// @author Jeff Wilson
/// 
///
//-------------------------------------------------------------------------------------------------


#ifndef _HELPER_TOOLS_HPP_
#define _HELPER_TOOLS_HPP_


#if (defined (WIN32) || defined (_WIN32))
//#include <math.h>

//from: http://wwwx.cs.unc.edu/~sud/tips/Programming_Tips.html

/* Win32 doesn't seem to have these functions.
** Therefore implement inline versions of these functions here.
*/
__inline long int
lrint (double flt)
{
  int intgr;
  _asm
  {
      fld flt
      fistp intgr
  } ;
  return intgr ;
}

__inline long int
lrintf (float flt)
{
   int intgr;
   _asm
  {
    fld flt
    fistp intgr
  } ;
  return intgr ;
}
#endif


//-------------------------------------------------------------------------------------------------
// normalizeFloat
//-------------------------------------------------------------------------------------------------
///
/// @brief Takes a float within range [inMin, inMax] and maps it to [outMin, outMax]
///
/// @param[in]		inF		the float to be linearly mapped/normalized
/// @param[in]		inMin	min value of incoming range
/// @param[in]		inMax	max value of incoming range
/// @param[in]		outMin	min value of outgoing range
/// @param[in]		outMax	max value of outgoing range
///
/// @return        float	the linearly mapped/normalized value
///
/// @pre	inMin must be less than inMax
/// @pre	outMin must be less than outMax (outMin and outMax MUST NOT be equal)
/// @pre	inF must be greater than or equal to inMin and less than or equal to inMax
///
/// @post	returns a float linearly mapped to the new range
/// @post	return float will be within the range [outMin, outMax]
///
__inline float normalizeFloat(float inF, float inMin, float inMax, float outMin, float outMax) {

	return outMin + ((inF - inMin) / (inMax - inMin))*(outMax - outMin);

} //normalizeFloat()


//-------------------------------------------------------------------------------------------------
// normalizeDouble
//-------------------------------------------------------------------------------------------------
///
/// @brief Takes a double within range [inMin, inMax] and maps it to [outMin, outMax]
///
/// @param[in]		inD		the double to be linearly mapped/normalized
/// @param[in]		inMin	min value of incoming range
/// @param[in]		inMax	max value of incoming range
/// @param[in]		outMin	min value of outgoing range
/// @param[in]		outMax	max value of outgoing range
///
/// @return        double	the linearly mapped/normalized value
///
/// @pre	inMin must be less than inMax
/// @pre	outMin must be less than outMax (outMin and outMax MUST NOT be equal)
/// @pre	inF must be greater than or equal to inMin and less than or equal to inMax
///
/// @post	returns a double linearly mapped to the new range
/// @post	return double will be within the range [outMin, outMax]
///
__inline double normalizeDouble(double inD, double inMin, double inMax, double outMin, double outMax) {

	return outMin + ((inD - inMin) / (inMax - inMin))*(outMax - outMin);

} //normalizeDouble()



//
//
//void * arrayGet(void *v, size_t elmSize, UINT32 i,UINT32 j,UINT32 jSize);
//void * arrayGet(void *v, size_t elmSize, UINT32 i,UINT32 j, UINT32 k, UINT32 jSize, UINT32 kSize);
//float * arrayGet(float *f, UINT32 i,UINT32 j,UINT32 jSize);
//float * arrayGet(float *f, UINT32 i,UINT32 j, UINT32 k, UINT32 jSize, UINT32 kSize);

//inline void * arrayGet(void *v, size_t elmSize, UINT32 i,UINT32 j,UINT32 jSize) {
//
//	return (void *)((unsigned char *)v+elmSize*(i*jSize + j)); 
//} //ptrToIndex()
//
//inline void * arrayGet(void *v, size_t elmSize, UINT32 i,UINT32 j, UINT32 k, UINT32 jSize, UINT32 kSize) {
//
//	return (void*)((unsigned char *)v+elmSize*( (i*jSize + j)*kSize + k)); 
//} //ptrToIndex()
//
//inline float * arrayGet(float *f, UINT32 i,UINT32 j,UINT32 jSize) {
//	return (float*)arrayGet((void*)f, sizeof(float), i,j,jSize);
//}
//
//
//inline float * arrayGet(float *f, UINT32 i,UINT32 j, UINT32 k, UINT32 jSize, UINT32 kSize) {
//	return (float *)arrayGet((void *)f, sizeof(float), i,j,k, jSize, kSize);
//}


#endif //#ifndef _HELPER_TOOLS_HPP_