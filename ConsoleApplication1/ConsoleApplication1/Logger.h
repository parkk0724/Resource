#pragma once
//---------------------------------------------------------------------------------------------------------------------
// TYPICAL USAGE:
// 
// 1) Call the GCC_ASSERT() macro just like you would for _ASSERTE()
//		Example -> GCC_ASSERT(ptr != NULL);
// 2) Call the GCC_ERROR() macro to send out an error message
//		Example -> GCC_ERROR("Something Blew Up!");
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
// LOGGING:
// 
// Calling  GCC_LOG("tag", "Log message") will print:
//  [tag] Log Message
// To enable these logs, you need to either manually call Logger::SetDisplayFlags() or set up a logging.xml file.
// logging.xml should follow this form:
// 
//  <Logger>
//      <Log tag="Actor" debugger="1" file="0"/>
//  </Logger>
// 
// The above chunk will cause all logs sent with the "Actor" tag to be displayed in the debugger.  If you set file 
// to 1 as well, it would log out to a file as well.  Don't check in logging.xml to SVN, it should be a local-only 
// file.
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------------------------------------------------------

// display flags
const unsigned char LOGFLAG_WRITE_TO_LOG_FILE = 1 << 0;
const unsigned char LOGFLAG_WRITE_TO_DEBUGGER = 1 << 1;


//---------------------------------------------------------------------------------------------------------------------
// This is the public Logger interface.  You must call Init() before any logging can take place and Destroy() when
// you're done to clean it up.  Call SetDisplayFlags() to set the display flags for a particular logging tag.  By 
// default, they are all off.  Although you can, you probably shouldn't call Log() directly.  Use the GCC_LOG() macro 
// instead since it can be stripped out by the compiler for Release builds.
//---------------------------------------------------------------------------------------------------------------------
namespace Logger
{
	// This class is used by the debug macros and shouldn't be accessed externally.
	class ErrorMessenger
	{
		bool m_enabled;

	public:
		ErrorMessenger(void);
		void Show(const std::string& errorMessage, bool isFatal, const char* funcName, const char* sourceFile, unsigned int lineNum);
	};

	// construction; must be called at the beginning and end of the program
	void Init(const char* loggingConfigFilename);
	void Destroy(void);

	// logging functions
	void Log(const std::string& tag, const std::string& message, const char* funcName, const char* sourceFile, unsigned int lineNum);
	void SetDisplayFlags(const std::string& tag, unsigned char flags);
}


//---------------------------------------------------------------------------------------------------------------------
// Debug macros
//---------------------------------------------------------------------------------------------------------------------

// Fatal Errors are fatal and are always presented to the user.
#define GCC_FATAL(str) \
	do \
	{ \
		static Logger::ErrorMessenger* pErrorMessenger = GCC_NEW Logger::ErrorMessenger; \
		std::string s((str)); \
		pErrorMessenger->Show(s, true, __FUNCTION__, __FILE__, __LINE__); \
	} \
	while (0)\

#if 1 //ndef NDEBUG

// Errors are bad and potentially fatal.  They are presented as a dialog with Abort, Retry, and Ignore.  Abort will
// break into the debugger, retry will continue the game, and ignore will continue the game and ignore every subsequent 
// call to this specific error.  They are ignored completely in release mode.
#define GCC_ERROR(str) \
	do \
	{ \
		static Logger::ErrorMessenger* pErrorMessenger = GCC_NEW Logger::ErrorMessenger; \
		std::string s((str)); \
		pErrorMessenger->Show(s, false, __FUNCTION__, __FILE__, __LINE__); \
	} \
	while (0)\

// Warnings are recoverable.  They are just logs with the "WARNING" tag that displays calling information.  The flags
// are initially set to WARNINGFLAG_DEFAULT (defined in debugger.cpp), but they can be overridden normally.
#define GCC_WARNING(str) \
	do \
	{ \
		std::string s((str)); \
		Logger::Log("WARNING", s, __FUNCTION__, __FILE__, __LINE__); \
	}\
	while (0)\

// This is just a convenient macro for logging if you don't feel like dealing with tags.  It calls Log() with a tag
// of "INFO".  The flags are initially set to LOGFLAG_DEFAULT (defined in debugger.cpp), but they can be overridden 
// normally.
#define GCC_INFO(str) \
	do \
	{ \
		std::string s((str)); \
		Logger::Log("INFO", s, NULL, NULL, 0); \
	} \
	while (0) \

// This macro is used for logging and should be the preferred method of "printf debugging".  You can use any tag 
// string you want, just make sure to enabled the ones you want somewhere in your initialization.
#define GCC_LOG(tag, str) \
	do \
	{ \
		std::string s((str)); \
		Logger::Log(tag, s, NULL, NULL, 0); \
	} \
	while (0) \

// This macro replaces GCC_ASSERT().
#define GCC_ASSERT(expr) \
	do \
	{ \
		if (!(expr)) \
		{ \
			static Logger::ErrorMessenger* pErrorMessenger = GCC_NEW Logger::ErrorMessenger; \
			pErrorMessenger->Show(#expr, false, __FUNCTION__, __FILE__, __LINE__); \
		} \
	} \
	while (0) \

#else  // NDEBUG is defined

// These are the release mode definitions for the macros above.  They are all defined in such a way as to be 
// ignored completely by the compiler.
#define GCC_ERROR(str) do { (void)sizeof(str); } while(0) 
#define GCC_WARNING(str) do { (void)sizeof(str); } while(0) 
#define GCC_INFO(str) do { (void)sizeof(str); } while(0) 
#define GCC_LOG(tag, str) do { (void)sizeof(tag); (void)sizeof(str); } while(0) 
#define GCC_ASSERT(expr) do { (void)sizeof(expr); } while(0) 

#endif  // !defined NDEBUG
