/** ==========================================================================
 * 2011 by KjellKod.cc. This is PUBLIC DOMAIN to use at your own risk and comes
 * with no warranties. This code is yours to share, use and modify with no
 * strings attached and no restrictions or obligations.
 * 
 * For more information see g3log/LICENSE or refer refer to http://unlicense.org
 * ============================================================================
 *
 * Filename:g2log.hpp  Framework for Logging and Design By Contract
 * Created: 2011 by Kjell Hedström
 *
 * PUBLIC DOMAIN and Not copywrited since it was built on public-domain software and influenced
 * at least in "spirit" from the following sources
 * 1. kjellkod.cc ;)
 * 2. Dr.Dobbs, Petru Marginean:  http://drdobbs.com/article/printableArticle.jhtml?articleId=201804215&dept_url=/cpp/
 * 3. Dr.Dobbs, Michael Schulze: http://drdobbs.com/article/printableArticle.jhtml?articleId=225700666&dept_url=/cpp/
 * 4. Google 'glog': http://google-glog.googlecode.com/svn/trunk/doc/glog.html
 * 5. Various Q&A at StackOverflow
 * ********************************************* */


#pragma once

#include <string>
#include <cstdarg>
#include <functional>

#include "g2loglevels.hpp"
#include "g2logmessagecapture.hpp"
#include "g2logmessage.hpp"

#if !(defined(__PRETTY_FUNCTION__))
#define __PRETTY_FUNCTION__   __FUNCTION__
#endif



/** namespace for LOG() and CHECK() frameworks
 * History lesson:   Why the names 'g2' and 'g2log'?:
 * The framework was made in my own free time as PUBLIC DOMAIN but the 
 * first commercial project to use it used 'g2' as an internal denominator for 
 * the current project. g2 as in 'generation 2'. I decided to keep the g2 and g2log names
 * to give credit to the people in that project (you know who you are :) and I guess also
 * for 'sentimental' reasons. That a big influence was google's glog is just a happy 
 *  concidence or subconscious choice. Either way g2log became the name for this logger.
 *
 * --- Thanks for a great 2011 and good luck with 'g2' --- KjellKod 
 */
namespace g2 {
   class LogWorker;
   struct LogMessage;
   struct FatalMessage;

   /** Should be called at very first startup of the software with \ref g2LogWorker
    *  pointer. Ownership of the \ref g2LogWorker is the responsibilkity of the caller */
   void initializeLogging(LogWorker *logger);

   
   //
   // internal namespace is for completely internal or semi-hidden from the g2 namespace due to that it is unlikely
   // that you will use these
   // That said:  
   //    1) setFatalExitHandler() can be used to set your own custom fatal handler. Please see g2log.cpp
   //             so that you shut down and clean up fatal handlers in a good way
   // 
   //    2) setFatalPreLoggingHook() can be used for either an extra step before the fatalExitHandler is called 
   //       examples here could be "cleanup" while keeping the normal default fatal handler
   //       another good example is to put a break point in programatically        
   namespace internal {
      /// @returns true if logger is initialized
      bool isLoggingInitialized();

      // Save the created LogMessage to any existing sinks
      void saveMessage(const char* message, const char* file, int line, const char* function, const LEVELS& level,
              const char* boolean_expression, int fatal_signal, const char* stack_trace);

      // forwards the message to all sinks
      void pushMessageToLogger(LogMessagePtr log_entry);

      
      // forwards a FATAL message to all sinks,. after which the g2logworker
      // will trigger crashhandler / g2::internal::exitWithDefaultSignalHandler
      // 
      // By default the "fatalCall" will forward a Fatalessageptr to this function
      // this behaviour can be changed if you set a different fatal handler through
      // "setFatalExitHandler"
      void pushFatalMessageToLogger(FatalMessagePtr message);


      // Save the created FatalMessage to any existing sinks and exit with 
      // the originating fatal signal,. or SIGABRT if it originated from a broken contract
      // By default forwards to: pushFatalMessageToLogger, see "setFatalExitHandler" to override
      //
      // If you override it then you probably want to call "pushFatalMessageToLogger" after your
      // custom fatal handler is done. This will make sure that the fatal message the pushed
      // to sinks as well as shutting down the process
      void fatalCall(FatalMessagePtr message);

      // Set a function-hook before a fatal message will be sent to the logger
      // i.e. this is a great place to put a break point, either in your debugger
      // or programatically to catch LOG(FATAL), CHECK(...) or an OS fatal event (exception or signal)
      // This will be reset to default (does nothing) at initializeLogging(...);
      //
      // Example usage:
      // Windows: g2::SetPreFatalHook([]{__debugbreak();}); // remember #include <intrin.h>
      // Linux:   g2::SetPreFatalHook([]{ raise(SIGTRAP); }); 
      void setFatalPreLoggingHook(std::function<void(void)>  pre_fatal_hook);

      /** By default the g2log will call g2LogWorker::fatalCall(...) which will
       * abort() the system after flushing the logs to file. This makes unit
       * test of FATAL level cumbersome. A work around is to change the
       * 'fatal call'  which can be done here 
       *  The bool return values in the fatal_call is whether or not the fatal_call should
       *  
       */
      void setFatalExitHandler(std::function<void(FatalMessagePtr)> fatal_call);



      // Shuts down logging. No object cleanup but further LOG(...) calls will be ignored.
      void shutDownLogging();

      // Shutdown logging, but ONLY if the active logger corresponds to the one currently initialized
      bool shutDownLoggingForActiveOnly(LogWorker* active);


      
   } // internal
} // g2

#define INTERNAL_LOG_MESSAGE(level) LogCapture(__FILE__, __LINE__, __PRETTY_FUNCTION__, level)

#define INTERNAL_CONTRACT_MESSAGE(boolean_expression)  \
        LogCapture(__FILE__, __LINE__, __PRETTY_FUNCTION__, g2::internal::CONTRACT, boolean_expression)


// LOG(level) is the API for the stream log
#define LOG(level) if(g2::logLevel(level)) INTERNAL_LOG_MESSAGE(level).stream()


// 'Conditional' stream log
#define LOG_IF(level, boolean_expression)  \
  if(true == boolean_expression)  \
  if(g2::logLevel(level))  INTERNAL_LOG_MESSAGE(level).stream()

// 'Design By Contract' stream API. For Broken Contracts:
//         unit testing: it will throw std::runtime_error when a contract breaks
//         I.R.L : it will exit the application by using fatal signal SIGABRT
#define CHECK(boolean_expression)        \
  if (false == (boolean_expression))  INTERNAL_CONTRACT_MESSAGE(#boolean_expression).stream()


/** For details please see this
 * REFERENCE: http://www.cppreference.com/wiki/io/c/printf_format
 * \verbatim
 *
  There are different %-codes for different variable types, as well as options to
    limit the length of the variables and whatnot.
    Code Format
    %[flags][width][.precision][length]specifier
 SPECIFIERS
 ----------
 %c character
 %d signed integers
 %i signed integers
 %e scientific notation, with a lowercase “e”
 %E scientific notation, with a uppercase “E”
 %f floating point
 %g use %e or %f, whichever is shorter
 %G use %E or %f, whichever is shorter
 %o octal
 %s a string of characters
 %u unsigned integer
 %x unsigned hexadecimal, with lowercase letters
 %X unsigned hexadecimal, with uppercase letters
 %p a pointer
 %n the argument shall be a pointer to an integer into which is placed the number of characters written so far

For flags, width, precision etc please see the above references.
EXAMPLES:
{
   LOGF(INFO, "Characters: %c %c \n", 'a', 65);
   LOGF(INFO, "Decimals: %d %ld\n", 1977, 650000L);      // printing long
   LOGF(INFO, "Preceding with blanks: %10d \n", 1977);
   LOGF(INFO, "Preceding with zeros: %010d \n", 1977);
   LOGF(INFO, "Some different radixes: %d %x %o %#x %#o \n", 100, 100, 100, 100, 100);
   LOGF(INFO, "floats: %4.2f %+.0e %E \n", 3.1416, 3.1416, 3.1416);
   LOGF(INFO, "Width trick: %*d \n", 5, 10);
   LOGF(INFO, "%s \n", "A string");
   return 0;
}
And here is possible output
:      Characters: a A
:      Decimals: 1977 650000
:      Preceding with blanks:       1977
:      Preceding with zeros: 0000001977
:      Some different radixes: 100 64 144 0x64 0144
:      floats: 3.14 +3e+000 3.141600E+000
:      Width trick:    10
:      A string  \endverbatim */
#define LOGF(level, printf_like_message, ...)                 \
    if(g2::logLevel(level)) INTERNAL_LOG_MESSAGE(level).capturef(printf_like_message, ##__VA_ARGS__)

// Conditional log printf syntax
#define LOGF_IF(level,boolean_expression, printf_like_message, ...) \
  if(true == boolean_expression)                                     \
  if(g2::logLevel(level))  INTERNAL_LOG_MESSAGE(level).capturef(printf_like_message, ##__VA_ARGS__)

// Design By Contract, printf-like API syntax with variadic input parameters.
// Throws std::runtime_eror if contract breaks
#define CHECK_F(boolean_expression, printf_like_message, ...)    \
  if (false == (boolean_expression))  INTERNAL_CONTRACT_MESSAGE(#boolean_expression).capturef(printf_like_message, ##__VA_ARGS__)



