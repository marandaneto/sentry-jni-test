package com.marandaneto.myapplication

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import io.sentry.Sentry
import io.sentry.android.AndroidSentryClientFactory
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        Sentry.init("dsn", AndroidSentryClientFactory(applicationContext))

        // stringFromJNI is a c++ method
        sample_text.text = stringFromJNI()
    }

    /**
     * defined in native-lib.cpp, Java_com_marandaneto_myapplication_MainActivity_stringFromJNI
     */
    external fun stringFromJNI(): String

    companion object {

        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("native-lib")
        }
    }
}
