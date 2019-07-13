#include <jni.h>
#include <string>
#include <exception>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <exception>
#include <stdexcept>
#include <csignal>
#include <unistd.h>
//#include <execinfo.h> will need to use unwind lib as its not compatible
#include <iostream>
#include <iomanip>

#include <unwind.h>
#include <dlfcn.h>
#include <sstream>
#include <android/log.h>


std::terminate_handler prev_handler;

static JNIEnv *jni_env = nullptr;

namespace {

    struct BacktraceState
    {
        void** current;
        void** end;
    };

    static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context* context, void* arg)
    {
        BacktraceState* state = static_cast<BacktraceState*>(arg);
        uintptr_t pc = _Unwind_GetIP(context);
        if (pc) {
            if (state->current == state->end) {
                return _URC_END_OF_STACK;
            } else {
                *state->current++ = reinterpret_cast<void*>(pc);
            }
        }
        return _URC_NO_REASON;
    }

}

size_t captureBacktrace(void** buffer, size_t max)
{
    BacktraceState state = {buffer, buffer + max};
    _Unwind_Backtrace(unwindCallback, &state);

    return state.current - buffer;
}

void dumpBacktrace(std::ostream& os, void** buffer, size_t count)
{
    for (size_t idx = 0; idx < count; ++idx) {
        const void* addr = buffer[idx];
        const char* symbol = "";

        Dl_info info;
        if (dladdr(addr, &info) && info.dli_sname) {
            symbol = info.dli_sname;
        }

        os << "  #" << std::setw(2) << idx << ": " << addr << "  " << symbol << "\n";
    }
}


void sendToSentry(const char* exception) {
    jstring jstr1 = jni_env->NewStringUTF(exception);
    jclass clazz = jni_env->FindClass("com/marandaneto/myapplication/Test");
    jmethodID mid = jni_env->GetStaticMethodID(clazz, "capture", "(Ljava/lang/String;)V");
    // will call a static method in the Test java class
    // I've used Java and static method because it's easier than Kotlin's companion object or non static method

    jni_env->CallStaticVoidMethod(clazz, mid, jstr1);
}

void handle_cpp_terminate() {
    // obtain a stacktrace and create a report here
    if (prev_handler != nullptr) {


        std::exception_ptr exptr = std::current_exception();
        if (exptr != 0)
        {
            // the only useful feature of std::exception_ptr is that it can be rethrown...
            try
            {
                std::rethrow_exception(exptr);
            }
            catch (std::exception &ex)
            {
                std::fprintf(stderr, "Terminated due to exception: %s\n", ex.what());
                sendToSentry(ex.what());
            }
            catch (...)
            {
                std::fprintf(stderr, "Terminated due to unknown exception\n");
                sendToSentry("Terminated due to unknown exception\n");
            }
        }
        else
        {
            std::fprintf(stderr, "Terminated due to unknown reason :(\n");
            sendToSentry("Terminated due to unknown reason :(\n");
        }


        prev_handler();
        std::abort();
    }
}


void printStacktrace()
{
    const size_t max = 30;
    void* buffer[max];
    std::ostringstream oss;
    dumpBacktrace(oss, buffer, captureBacktrace(buffer, max));
    const char *pStr = oss.str().c_str();
    __android_log_print(ANDROID_LOG_INFO, "app_name", "%s", pStr);


//    not working because pStr is
// UnicodeEncodeError: 'ascii' codec can't encode characters in position
// theres a thing with JNI UTF16 and ascii, so right now its not being reported, but you can have a look at the logs
    sendToSentry(pStr);
}

void signalHandler(int sig)
{
    std::fprintf(stderr, "Error: signal %d\n", sig);
    printStacktrace();
    std::abort();
}

void init(JNIEnv *env) {
    // save jni env. context
    jni_env = env;

    // set handler to the terminate function, which is the last life cycle of the app before crashing it
    signal(SIGSEGV, signalHandler);
    prev_handler = std::set_terminate(handle_cpp_terminate);

    // just a random way of testing normal exceptions and segfault, so execute more times to get into both ways
    srand(time(nullptr));
    if (rand() % 2)
    {
        // segfault
        int *bad = nullptr;
        std::fprintf(stderr, "%d", *bad);
    }
    else
    {
        // exception
        throw std::runtime_error("Hello, world!");
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_marandaneto_myapplication_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject) {

    init(env);

    std::string hello = "Hello from C++";


// return to the kotlin method
    return env->NewStringUTF(hello.c_str());
}
