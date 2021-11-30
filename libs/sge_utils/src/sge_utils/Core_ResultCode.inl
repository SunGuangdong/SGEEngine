//
//Designed to be included into Core.h
//

//namespace sge {

///////////////////////////////////////////////////////////
//Class ResultCode
//Usage : 
// Return value codes
// negative values mean ERROR,
// 0 means Everything SUCCEEDED
// positive values mean SUCCESS with some warrnings
//
// Reserved values :
// -1 Generic Error
// -2 InvalidArg
// 0 Success
//
// The object create by the default constructor is considered as Success
//
//class ResultCode
//{
//public :
//
//	ResultCode(const int code = 0,              // error code
//			   const char* debugMessage = NULL, //any debug messages
//			   const char* function = NULL)     //usually __FUNCTION__
//		: resultCode(code)
//	{
//		//dead code elimination will do the job
//		if(bool < 0)
//		{
//			if(function && debugMessage) sgeLogInfo("%s: %s\n", function, debugMessage);
//			sgeAssert(false);
//		}
//	}
//	bool Succeeded() const	{ return bool >= 0; }
//	bool Failed() const		{ return !Succeeded(); }
//	operator bool()			{ return Succeeded(); }
//
//	int resultCode;
//
//protected :
//
//};
//
////this must be a macro because of the __FUNCTION__
//#define SGE_ResultCode(code, debugMsg)	ResultCode(code, debugMsg, __FUNCTION__)
//#define SGE_ResultError(debugMsg)		SGE_ResultCode(-1, debugMsg)
//#define SGE_ResultInvalidArg(debugMsg)	SGE_ResultCode(-2, debugMsg)
//#define SGE_ResultFuncNotImpl(debugMsg)	SGE_ResultCode(-3, debugMsg)
//#define SGE_ResultSuccess()				SGE_ResultCode(0, "<msg>")

//}
