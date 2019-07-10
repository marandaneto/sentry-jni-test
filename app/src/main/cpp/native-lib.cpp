#include <jni.h>
#include <string>

std::terminate_handler prev_handler;

static JNIEnv *jni_env = NULL;

void handle_cpp_terminate() {
    // obtain a stacktrace and create a report here
    if (prev_handler != NULL) {
        prev_handler();
    }
}

void init(JNIEnv *env) {
    // save jni env. context
    jni_env = env;

    // set handler to the terminate function, which is the last life cycle of the app before crashing it
    prev_handler = std::set_terminate(handle_cpp_terminate);
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_marandaneto_myapplication_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject) {

    init(env);

    std::string hello = "Hello from C++";

    jstring jstr1 = env->NewStringUTF(hello.c_str());
    jclass clazz = env->FindClass("com/marandaneto/myapplication/Test");
    jmethodID mid = env->GetStaticMethodID(clazz, "capture", "(Ljava/lang/String;)V");
    // will call a static method in the Test java class
    // I've used Java and static method because it's easier than Kotlin's companion object or non static method
    env->CallStaticVoidMethod(clazz, mid, jstr1);


    // both cause a crash
//    throw "Lets see";
//    std::terminate();


// return to the kotlin method
    return env->NewStringUTF(hello.c_str());
}
